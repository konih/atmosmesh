// AtmosMesh Room product composition root — ideaspark ESP32-WROOM-32 + 1.14 inch ST7789 TFT.
//
// Scope: VEML7700 ambient light and SHT41 temperature/humidity on one 3.3 V I2C bus, a D-SUN
// PIR on GPIO33, an SDS011 particulate sensor listened to on UART2, a beeper on GPIO25 that
// chirps once at boot and sounds when particulates go high, and Wi-Fi/MQTT publishing every
// reading to Home Assistant under atmosmesh-room-0001.
//
// D-024: the display is SPI and owns GPIO23/18/15/2/4/32. GPIO12 is the flash strap and
// GPIO1/GPIO3 are the CP2102 USB-UART. room_pins.hpp turns that list into static_asserts, so a
// pin collision is a build failure rather than a smoke test.
//
// The SDS011 reports autonomously at 1 Hz, so this only listens. Nothing here depends on the
// control protocol (sleep, duty cycle, query mode), which is still an open document.
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_VEML7700.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "atmosmesh/digital_edge.hpp"
#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/pm_alarm.hpp"
#include "atmosmesh/room_mqtt_runtime.hpp"
#include "atmosmesh/room_pins.hpp"
#include "atmosmesh/sds011_frame.hpp"
#include "atmosmesh/sht4x_frame.hpp"

namespace room = atmosmesh::room;

namespace {

// ---------------------------------------------------------------- palette

constexpr std::uint16_t rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return static_cast<std::uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr std::uint16_t kBackground = rgb(8, 10, 16);
constexpr std::uint16_t kTitleBar = rgb(24, 48, 96);
constexpr std::uint16_t kTitleText = rgb(235, 242, 255);
constexpr std::uint16_t kLabel = rgb(126, 142, 168);
constexpr std::uint16_t kRule = rgb(44, 52, 72);
constexpr std::uint16_t kBarTrack = rgb(34, 40, 56);
constexpr std::uint16_t kMuted = rgb(96, 104, 124);

constexpr std::uint16_t kBlue = rgb(56, 132, 255);
constexpr std::uint16_t kCyan = rgb(0, 205, 220);
constexpr std::uint16_t kGreen = rgb(56, 214, 122);
constexpr std::uint16_t kAmber = rgb(255, 176, 32);
constexpr std::uint16_t kRed = rgb(255, 72, 72);
constexpr std::uint16_t kAlarmBar = rgb(150, 24, 24);
constexpr std::uint16_t kYellow = rgb(255, 226, 92);
constexpr std::uint16_t kWhite = rgb(245, 245, 245);

// ---------------------------------------------------------------- geometry
//
// Six cells, two columns by three rows, under an 18 px title bar. Six readings will not fit as
// full-width rows on a 135 px panel at a legible text size, and shrinking the type to make them
// fit defeats the point of a display you read from across the room.

constexpr int kW = room::kScreenWidthPx;    // 240
constexpr int kH = room::kScreenHeightPx;   // 135
constexpr int kTitleH = 18;
constexpr int kCols = 2;
constexpr int kRows = 3;
constexpr int kCellCount = kCols * kRows;
constexpr int kColW = kW / kCols;              // 120
constexpr int kRowH = (kH - kTitleH) / kRows;  // 39

constexpr int kCellTemp = 0;
constexpr int kCellHum = 1;
constexpr int kCellLux = 2;
constexpr int kCellPm25 = 3;
constexpr int kCellPm10 = 4;
constexpr int kCellPir = 5;

int cell_x(int i) { return (i / kRows) * kColW; }
int cell_y(int i) { return kTitleH + (i % kRows) * kRowH; }

// ---------------------------------------------------------------- state

Adafruit_ST7789 tft(room::kTftCsGpio, room::kTftDcGpio, room::kTftRstGpio);
Adafruit_VEML7700 veml;
atmosmesh::Sds011Stream sds_stream;
atmosmesh::PmAlarm pm_alarm;
atmosmesh::DebouncedLevel pir_level;

bool veml_present = false;
bool sht_present = false;
bool bus_fault = false;

bool sds_have_sample = false;
float sds_pm25 = 0.0F;
float sds_pm10 = 0.0F;
unsigned long sds_last_frame_ms = 0;
unsigned long sds_frame_count = 0;

bool pir_raw_high = false;
bool pir_motion = false;
bool pir_ever_moved = false;
unsigned long pir_last_motion_ms = 0;

unsigned long last_sample_ms = 0;
unsigned long last_publish_ms = 0;
bool published_motion = false;
bool published_alarm = false;
bool link_dots_shown_up = false;
bool link_dots_shown_wifi = false;
unsigned long sht_last_ok_ms = 0;
unsigned long veml_last_ok_ms = 0;
bool heartbeat_on = false;
bool title_alarm_shown = false;
bool title_drawn = false;

struct Cell {
    const char* label;
    char shown[10];
    int shown_bar_px;
    std::uint16_t shown_colour;
    // The motion cell relabels itself, so the label is state rather than chrome drawn once.
    const char* shown_label;
};

Cell cells[kCellCount] = {
    {"TEMP C", "", -1, 0, nullptr}, {"HUM %", "", -1, 0, nullptr},
    {"LUX", "", -1, 0, nullptr},    {"PM2.5", "", -1, 0, nullptr},
    {"PM10", "", -1, 0, nullptr},   {"MOTION", "", -1, 0, nullptr},
};

// ---------------------------------------------------------------- colour rules

std::uint16_t temp_colour(float c) {
    if (c < 16.0F) return kBlue;
    if (c < 20.0F) return kCyan;
    if (c < 24.5F) return kGreen;
    if (c < 27.5F) return kAmber;
    return kRed;
}

std::uint16_t hum_colour(float pct) {
    if (pct < 30.0F) return kAmber;   // dry
    if (pct < 60.0F) return kGreen;   // comfortable
    if (pct < 70.0F) return kCyan;
    return kBlue;                     // damp
}

std::uint16_t lux_colour(float lx) {
    if (lx < 5.0F) return kBlue;      // dark
    if (lx < 60.0F) return kCyan;     // dim
    if (lx < 400.0F) return kAmber;   // lit room
    if (lx < 2000.0F) return kYellow;
    return kWhite;                    // daylight
}

// Red begins exactly at the level that arms the beeper, so the screen and the sound agree.
std::uint16_t pm25_colour(float v) {
    if (v < 12.0F) return kGreen;
    if (v < 25.0F) return kYellow;
    if (v < atmosmesh::kDefaultPmThresholds.pm25_alarm_ug_m3) return kAmber;
    return kRed;
}

std::uint16_t pm10_colour(float v) {
    if (v < 25.0F) return kGreen;
    if (v < 40.0F) return kYellow;
    if (v < atmosmesh::kDefaultPmThresholds.pm10_alarm_ug_m3) return kAmber;
    return kRed;
}

float clamp01(float v) { return v < 0.0F ? 0.0F : (v > 1.0F ? 1.0F : v); }

float temp_fraction(float c) { return clamp01(c / 40.0F); }
float hum_fraction(float pct) { return clamp01(pct / 100.0F); }
// Light is logarithmic to the eye; a linear bar would sit at zero indoors.
float lux_fraction(float lx) { return clamp01(std::log10(lx + 1.0F) / 5.0F); }
float pm25_fraction(float v) { return clamp01(v / 100.0F); }
float pm10_fraction(float v) { return clamp01(v / 150.0F); }

// Read the two I2C lines as plain GPIOs BEFORE handing them to the driver. An idle bus rests
// HIGH on both. If either is held LOW, Wire.begin() can block forever inside its bus-recovery
// loop -- a hang with no panic and no watchdog, the least diagnosable failure this board makes.
struct BusIdle {
    bool sda_high;
    bool scl_high;
    bool healthy() const { return sda_high && scl_high; }
};

BusIdle probe_bus_idle() {
    pinMode(room::kI2cSdaGpio, INPUT_PULLUP);
    pinMode(room::kI2cSclGpio, INPUT_PULLUP);
    delay(5);
    return {digitalRead(room::kI2cSdaGpio) == HIGH, digitalRead(room::kI2cSclGpio) == HIGH};
}

// ---------------------------------------------------------------- beeper
//
// Non-blocking on purpose. A delay()-based pattern would stall the PIR poll and the UART drain
// for the length of the beep, which is exactly when the board is busiest.
struct Beeper {
    int pulses_left = 0;
    bool on = false;
    unsigned long next_change_ms = 0;
    // Nobody reading a serial log can hear the can. Timestamping the rising edge turns "it
    // beeped" into a measured HIGH duration, which is what actually distinguishes a 60 ms chirp
    // from the ~1.8 s drone this used to emit while setup() was busy elsewhere.
    unsigned long on_since_ms = 0;

    void begin() {
        pinMode(room::kBeeperGpio, OUTPUT);
        digitalWrite(room::kBeeperGpio, LOW);
    }

    void start(int pulses, unsigned long now) {
        if (pulses <= 0) return;
        pulses_left = pulses;
        on = true;
        on_since_ms = now;
        digitalWrite(room::kBeeperGpio, HIGH);
        next_change_ms = now + room::kBeeperPulseMs;
    }

    void service(unsigned long now) {
        if (!on && pulses_left == 0) return;
        // Signed difference so a millis() rollover is not a stuck beeper.
        if (static_cast<long>(now - next_change_ms) < 0) return;
        if (on) {
            digitalWrite(room::kBeeperGpio, LOW);
            on = false;
            --pulses_left;
            Serial.printf("beeper: pulse held %lums (target %lums) remaining=%d\n",
                          now - on_since_ms, room::kBeeperPulseMs, pulses_left);
            next_change_ms = now + room::kBeeperGapMs;
        } else if (pulses_left > 0) {
            digitalWrite(room::kBeeperGpio, HIGH);
            on = true;
            on_since_ms = now;
            next_change_ms = now + room::kBeeperPulseMs;
        }
    }
};

Beeper beeper;

// ---------------------------------------------------------------- drawing

void draw_text(int x, int y, const char* s, std::uint8_t size, std::uint16_t colour) {
    tft.setTextSize(size);
    tft.setTextColor(colour);
    tft.setCursor(x, y);
    tft.print(s);
}

void draw_right(int right_x, int y, const char* s, std::uint8_t size, std::uint16_t colour) {
    const int width = static_cast<int>(std::strlen(s)) * 6 * size;
    draw_text(right_x - width, y, s, size, colour);
}

void draw_status_dots() {
    // Guarded here rather than at each call site: while the bus-fault splash is up there is no
    // title bar to draw into, and every caller would otherwise need to remember that.
    if (!title_drawn) {
        return;
    }
    // W: link health at a glance, so "is Home Assistant getting this?" does not need a serial
    // cable. Amber is the honest middle state -- associated to Wi-Fi but not talking to a broker.
    const bool mqtt_up = atmosmesh::room_mqtt_runtime_mqtt_up();
    const bool wifi_up = atmosmesh::room_mqtt_runtime_wifi_up();
    const std::uint16_t link = mqtt_up ? kGreen : (wifi_up ? kAmber : kRed);
    tft.fillCircle(kW - 88, 9, 3, link);
    draw_text(kW - 82, 5, "W", 1, link);
    link_dots_shown_up = mqtt_up;
    link_dots_shown_wifi = wifi_up;
    tft.fillCircle(kW - 66, 9, 3, sht_present ? kGreen : kRed);
    draw_text(kW - 60, 5, "T", 1, sht_present ? kGreen : kRed);
    tft.fillCircle(kW - 44, 9, 3, veml_present ? kGreen : kRed);
    draw_text(kW - 38, 5, "L", 1, veml_present ? kGreen : kRed);
    tft.fillCircle(kW - 22, 9, 3, sds_have_sample ? kGreen : kRed);
    draw_text(kW - 16, 5, "P", 1, sds_have_sample ? kGreen : kRed);
}

// The title bar is the alarm surface. Turning the whole strip red is readable from further away
// than any badge, and it costs no layout space that a reading needs.
void draw_title(bool alarm) {
    tft.fillRect(0, 0, kW, kTitleH, alarm ? kAlarmBar : kTitleBar);
    if (alarm) {
        draw_text(6, 5, "!! PARTICULATES HIGH", 1, kWhite);
    } else {
        draw_text(6, 5, "ATMOSMESH", 1, kTitleText);
        draw_text(66, 5, "ROOM", 1, kCyan);
    }
    title_alarm_shown = alarm;
    title_drawn = true;   // set before the dots, which refuse to draw without a title bar
    draw_status_dots();
}

void draw_chrome() {
    tft.fillScreen(kBackground);
    draw_title(false);
    tft.drawFastVLine(kColW - 1, kTitleH, kH - kTitleH, kRule);
    for (int r = 1; r < kRows; ++r) {
        tft.drawFastHLine(0, kTitleH + r * kRowH, kW, kRule);
    }
    for (int i = 0; i < kCellCount; ++i) {
        const int x = cell_x(i);
        const int y = cell_y(i);
        draw_text(x + 4, y + 3, cells[i].label, 1, kLabel);
        tft.fillRect(x + 4, y + kRowH - 5, kColW - 10, 3, kBarTrack);
        cells[i].shown[0] = '\0';
        cells[i].shown_bar_px = -1;
        cells[i].shown_label = cells[i].label;
    }
}

// Only the value text and the bar move. The chrome is drawn once, so nothing flickers.
void draw_cell(int i, bool valid, const char* text, float fraction, std::uint16_t colour) {
    Cell& c = cells[i];
    const int x = cell_x(i);
    const int y = cell_y(i);
    const std::uint16_t shade = valid ? colour : kMuted;
    const int track_w = kColW - 10;

    if (std::strcmp(c.shown, text) != 0 || c.shown_colour != shade) {
        tft.fillRect(x + 2, y + 11, kColW - 5, 24, kBackground);
        draw_right(x + kColW - 6, y + 11, text, 3, shade);
        std::snprintf(c.shown, sizeof(c.shown), "%s", text);
        c.shown_colour = shade;
    }

    const int fill = valid ? static_cast<int>(fraction * static_cast<float>(track_w)) : 0;
    if (fill != c.shown_bar_px) {
        tft.fillRect(x + 4, y + kRowH - 5, track_w, 3, kBarTrack);
        if (fill > 0) {
            tft.fillRect(x + 4, y + kRowH - 5, fill, 3, shade);
        }
        c.shown_bar_px = fill;
    }
}

// Only the motion cell uses this. Labels are string literals, so comparing the stored pointer's
// text is enough to know whether the strip needs repainting.
void draw_cell_label(int i, const char* label) {
    Cell& c = cells[i];
    if (c.shown_label != nullptr && std::strcmp(c.shown_label, label) == 0) {
        return;
    }
    const int x = cell_x(i);
    const int y = cell_y(i);
    tft.fillRect(x + 2, y + 2, kColW - 5, 9, kBackground);
    draw_text(x + 4, y + 3, label, 1, kLabel);
    c.shown_label = label;
}

// The motion cell.
//
// What this replaces: "MOTION / YES" for the live state, a bare "45s" for time since the last
// movement, and a bare "45s" for the power-on warm-up countdown. Two opposite meanings shared
// one rendering -- a number in that cell could mean "someone moved 45 seconds ago" or "the
// sensor is not trustworthy for another 45 seconds" -- and "YES" answered a question the panel
// never asked.
//
// Now the label states what the number means, so the two never collide, and the live state is a
// word rather than a yes/no. The bar carries recency: full means someone is here or was moments
// ago, empty means the room has been still for ten minutes.
void draw_motion_cell(unsigned long now) {
    char buf[10];

    if (now < room::kPirWarmupMs) {
        const unsigned long remaining_ms = room::kPirWarmupMs - now;
        std::snprintf(buf, sizeof(buf), "%lus", (remaining_ms + 999UL) / 1000UL);
        draw_cell_label(kCellPir, "PIR WARMUP");
        const float done = 1.0F - static_cast<float>(remaining_ms) /
                                      static_cast<float>(room::kPirWarmupMs);
        draw_cell(kCellPir, true, buf, clamp01(done), kCyan);
        return;
    }

    if (pir_motion) {
        draw_cell_label(kCellPir, "MOTION");
        draw_cell(kCellPir, true, "NOW", 1.0F, kAmber);
        return;
    }

    if (!pir_ever_moved) {
        // Never a confirmed movement since boot. That is an absence of evidence, not an empty
        // room, so it stays muted and invalid rather than claiming a quiet reading.
        draw_cell_label(kCellPir, "MOTION");
        draw_cell(kCellPir, false, "--", 0.0F, kMuted);
        return;
    }

    const unsigned long age_s = (now - pir_last_motion_ms) / 1000UL;
    if (age_s < 60UL) {
        std::snprintf(buf, sizeof(buf), "%lus", age_s);
    } else if (age_s < 3600UL) {
        std::snprintf(buf, sizeof(buf), "%lum", age_s / 60UL);
    } else {
        std::snprintf(buf, sizeof(buf), "%luh", age_s / 3600UL);
    }
    draw_cell_label(kCellPir, "LAST SEEN");
    const float fresh = clamp01(1.0F - static_cast<float>(age_s) / 600.0F);
    draw_cell(kCellPir, true, buf, fresh, age_s < 120UL ? kCyan : kMuted);
}

void draw_heartbeat() {
    heartbeat_on = !heartbeat_on;
    tft.fillCircle(kW - 104, 9, 2,
                   heartbeat_on ? kCyan : (title_alarm_shown ? kAlarmBar : kTitleBar));
}

void splash_bus_fault(const BusIdle& idle) {
    tft.fillScreen(kBackground);
    tft.drawRect(0, 0, kW, kH, kRed);
    draw_text(10, 12, "I2C BUS", 3, kRed);
    draw_text(10, 42, "HELD LOW", 3, kRed);
    char line[48];
    std::snprintf(line, sizeof(line), "SDA gpio%d = %s", room::kI2cSdaGpio,
                  idle.sda_high ? "HIGH ok" : "LOW  <-- stuck");
    draw_text(10, 78, line, 1, idle.sda_high ? kGreen : kRed);
    std::snprintf(line, sizeof(line), "SCL gpio%d = %s", room::kI2cSclGpio,
                  idle.scl_high ? "HIGH ok" : "LOW  <-- stuck");
    draw_text(10, 92, line, 1, idle.scl_high ? kGreen : kRed);
    draw_text(10, 112, "check wiring before powering on", 1, kMuted);
}

void splash(const std::uint8_t* found, std::size_t count) {
    tft.fillScreen(kBackground);
    // A 1 px border proves the geometry: if any edge is missing, the rotation offset is wrong.
    tft.drawRect(0, 0, kW, kH, kRule);
    draw_text(10, 10, "ATMOSMESH", 3, kTitleText);
    draw_text(10, 38, "ROOM", 3, kCyan);

    char line[44];
    std::snprintf(line, sizeof(line), "i2c sda=%d scl=%d found %u", room::kI2cSdaGpio,
                  room::kI2cSclGpio, static_cast<unsigned>(count));
    draw_text(10, 68, line, 1, kMuted);

    char addrs[40] = "";
    int used = 0;
    for (std::size_t i = 0; i < count && used < 30; ++i) {
        used += std::snprintf(addrs + used, sizeof(addrs) - used, "0x%02X ", found[i]);
    }
    draw_text(10, 82, count ? addrs : "no i2c devices", 1, count ? kGreen : kRed);

    std::snprintf(line, sizeof(line), "pir gpio%d %s", room::kPirGpio,
                  room::kPirActiveLow ? "active-low" : "active-high");
    draw_text(10, 100, line, 1, kLabel);
    std::snprintf(line, sizeof(line), "sds011 rx=%d tx=%d  beep gpio%d", room::kSdsRxGpio,
                  room::kSdsTxGpio, room::kBeeperGpio);
    draw_text(10, 114, line, 1, kLabel);
}

// ---------------------------------------------------------------- sensors

enum class ShtResult { Ok, NoAck, ShortRead, BadCrc };

ShtResult read_sht41(float* temperature_c, float* humidity_pct) {
    Wire.beginTransmission(atmosmesh::kSht41I2cAddress);
    Wire.write(atmosmesh::kSht41MeasureHighPrecisionCmd);
    if (Wire.endTransmission() != 0) {
        return ShtResult::NoAck;
    }
    delay(atmosmesh::kSht41MeasureDelayMs);
    if (Wire.requestFrom(static_cast<std::uint8_t>(atmosmesh::kSht41I2cAddress),
                         static_cast<std::uint8_t>(atmosmesh::kSht41FrameBytes)) !=
        static_cast<int>(atmosmesh::kSht41FrameBytes)) {
        return ShtResult::ShortRead;
    }
    std::uint8_t bytes[atmosmesh::kSht41FrameBytes];
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(Wire.read());
    }
    const auto sample = atmosmesh::parse_sht41_frame(bytes);
    if (!sample.ok) {
        return ShtResult::BadCrc;   // never report a partially-corrupt frame as a reading
    }
    *temperature_c = sample.temperature_c;
    *humidity_pct = sample.humidity_pct;
    return ShtResult::Ok;
}

bool i2c_present(std::uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool pir_reading_is_motion(bool raw_high) { return room::kPirActiveLow ? !raw_high : raw_high; }

// Polled far more often than the 1 Hz display cadence: a PIR pulse can be shorter than one
// display period, and a missed edge is indistinguishable from a dead sensor.
void poll_pir(unsigned long now) {
    const bool raw_high = digitalRead(room::kPirGpio) == HIGH;
    pir_raw_high = raw_high;

    const bool sample = pir_reading_is_motion(raw_high);
    if (!atmosmesh::update_debounced_level(pir_level, sample, static_cast<std::uint32_t>(now),
                                           room::kPirDebounceMs)) {
        if (pir_motion) {
            pir_last_motion_ms = now;
        }
        return;
    }

    pir_motion = pir_level.stable;
    const bool warm = now < room::kPirWarmupMs;
    Serial.printf("pir: %s raw=gpio%d:%s%s uptime=%lus\n", pir_motion ? "MOTION" : "clear",
                  room::kPirGpio, raw_high ? "HIGH" : "LOW", warm ? " (warm-up, not trusted)" : "",
                  now / 1000UL);
    if (pir_motion && !warm) {
        pir_ever_moved = true;
        pir_last_motion_ms = now;
    }
}

// The SDS011 talks without being asked, so this drains whatever arrived since the last pass.
void drain_sds011(unsigned long now) {
    while (Serial2.available() > 0) {
        const int byte = Serial2.read();
        if (byte < 0) {
            break;
        }
        const auto sample = sds_stream.feed(static_cast<std::uint8_t>(byte));
        if (sample.ok) {
            sds_pm25 = sample.pm25_ug_m3;
            sds_pm10 = sample.pm10_ug_m3;
            sds_last_frame_ms = now;
            ++sds_frame_count;
            if (!sds_have_sample) {
                sds_have_sample = true;
                Serial.println(atmosmesh::format_sds011_listen_log().c_str());
                draw_status_dots();
            }
        }
    }
}

// setup() has slow stretches -- a full I2C scan, a driver probe, the splash hold -- and a bare
// delay() across them is what turned the boot chirp into a ~1.8 s drone: start() drives the pin
// HIGH and only service() lowers it, and service() was reachable from loop() alone. Everything
// that must keep running while setup() waits runs here instead.
void wait_servicing(unsigned long duration_ms) {
    const unsigned long started = millis();
    while (millis() - started < duration_ms) {
        const unsigned long now = millis();
        beeper.service(now);
        drain_sds011(now);
        poll_pir(now);
        delay(1);
    }
}

// The 1 Hz block below does slow things: an SHT41 conversion, a VEML7700 auto-ranging read that
// walks gain and integration time and can take several hundred milliseconds, and a screen full
// of SPI. The beeper is timed in software, so nothing lowers its pin while any of that runs.
// Measured: the first pulse of a particulate alarm held 535 ms against a 60 ms target, because
// the alarm fired at the top of the block and the next service() call came a whole block later.
// Servicing between the slow steps bounds a pulse to the longest single step instead.
void service_beeper() {
    beeper.service(millis());
}

// ---------------------------------------------------------------- home assistant

atmosmesh::MqttReading reading_of(bool valid, float value, unsigned long now,
                                  unsigned long stamp_ms) {
    atmosmesh::MqttReading r;
    r.valid = valid;
    r.value = value;
    // age_ms is "how old is the newest good sample", so a never-read sensor reports 0 rather
    // than the uptime, which would read as a very stale measurement instead of no measurement.
    r.age_ms = stamp_ms == 0UL ? 0UL : now - stamp_ms;
    return r;
}

bool ever_published = false;

void publish_room_state(unsigned long now, bool sht_ok, float t_c, float rh, bool veml_ok,
                        float lux, bool sds_fresh) {
    if (!atmosmesh::room_mqtt_runtime_enabled()) {
        return;
    }

    // A PIR inside its warm-up window and an SDS011 inside its spin-up window are both producing
    // numbers that mean nothing yet. They go out as invalid, which Home Assistant renders as
    // unavailable -- the one state that is not a lie about the room.
    const bool motion_valid = now >= room::kPirWarmupMs;
    const bool pm_valid = sds_fresh && now >= room::kSdsWarmupMs;
    const bool alarm_valid = pm_alarm.level() != atmosmesh::PmLevel::Unknown;
    const bool alarm = pm_alarm.high();

    const bool flipped = (motion_valid && pir_motion != published_motion) ||
                         (alarm_valid && alarm != published_alarm);
    if (ever_published && !flipped && (now - last_publish_ms) < room::kMqttPublishIntervalMs) {
        return;
    }

    atmosmesh::RoomMqttState state;
    state.temperature_c = reading_of(sht_ok, t_c, now, sht_last_ok_ms);
    state.humidity_pct = reading_of(sht_ok, rh, now, sht_last_ok_ms);
    state.illuminance_lx = reading_of(veml_ok, lux, now, veml_last_ok_ms);
    state.pm25 = reading_of(pm_valid, sds_pm25, now, sds_last_frame_ms);
    state.pm10 = reading_of(pm_valid, sds_pm10, now, sds_last_frame_ms);
    state.motion = {pir_motion, motion_valid,
                    pir_last_motion_ms == 0UL ? 0UL : now - pir_last_motion_ms};
    state.pm_alarm = {alarm, alarm_valid,
                      sds_last_frame_ms == 0UL ? 0UL : now - sds_last_frame_ms};

    atmosmesh::room_mqtt_runtime_publish_state(state);
    last_publish_ms = now;
    published_motion = pir_motion;
    published_alarm = alarm;
    ever_published = true;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("room: boot product=atmosmesh-room-v1 board=ideaspark-esp32-1v14-tft");
    Serial.printf("room: tft mosi=%d sclk=%d cs=%d dc=%d rst=%d blk=%d rotation=%d\n",
                  room::kTftMosiGpio, room::kTftSclkGpio, room::kTftCsGpio, room::kTftDcGpio,
                  room::kTftRstGpio, room::kTftBacklightGpio, room::kTftRotation);

    // Backlight first. Without this a perfectly working panel reads as a dead one.
    pinMode(room::kTftBacklightGpio, OUTPUT);
    digitalWrite(room::kTftBacklightGpio, HIGH);

    SPI.begin(room::kTftSclkGpio, -1, room::kTftMosiGpio, -1);
    tft.init(room::kTftNativeWidthPx, room::kTftNativeHeightPx);
    tft.setRotation(room::kTftRotation);
    tft.setTextWrap(false);
    Serial.printf("room: display ready %dx%d\n", kW, kH);

    // A disconnected PIR must read as "clear", never float into phantom motion.
    pinMode(room::kPirGpio, room::kPirActiveLow ? INPUT : INPUT_PULLDOWN);
    Serial.printf("pir: gpio=%d polarity=%s warmup=%lus debounce=%dms\n", room::kPirGpio,
                  room::kPirActiveLow ? "active-low (carrier Q_PIR inverter)"
                                      : "active-high (direct to the module output)",
                  room::kPirWarmupMs / 1000UL, room::kPirDebounceMs);

    Serial2.begin(room::kSdsBaud, SERIAL_8N1, room::kSdsRxGpio, room::kSdsTxGpio);
    Serial.printf("sds011: uart2 rx=gpio%d tx=gpio%d %lu 8N1 warmup=%lus\n", room::kSdsRxGpio,
                  room::kSdsTxGpio, room::kSdsBaud, room::kSdsWarmupMs / 1000UL);

    beeper.begin();
    Serial.printf("beeper: gpio=%d pulse=%lums alarm=%d pulses\n", room::kBeeperGpio,
                  room::kBeeperPulseMs, room::kBeeperAlarmPulses);
    // One boot chirp: proof the beeper is wired, before any alarm can claim credit for it.
    // Serviced to completion right here -- the I2C scan and the splash hold that follow used to
    // run with the pin still HIGH, so what the room actually heard was a two-second tone.
    beeper.start(1, millis());
    wait_servicing(room::kBeeperPulseMs + 20UL);

    // Async: association overlaps the I2C scan and the splash rather than stalling boot.
    atmosmesh::room_mqtt_runtime_begin();

    const BusIdle idle = probe_bus_idle();
    Serial.printf("i2c: idle sda(gpio%d)=%s scl(gpio%d)=%s\n", room::kI2cSdaGpio,
                  idle.sda_high ? "HIGH" : "LOW", room::kI2cSclGpio,
                  idle.scl_high ? "HIGH" : "LOW");
    if (!idle.healthy()) {
        Serial.println("i2c: FAULT a line is held low - refusing to start the driver");
        Serial.println("i2c: an idle bus rests high on both lines; check for a short to GND,");
        Serial.println("i2c: a swapped supply pin, or a module held in reset");
        splash_bus_fault(idle);
        bus_fault = true;
        return;   // never enter Wire.begin() on a stuck bus
    }

    std::uint8_t found[16];
    const std::size_t count =
        atmosmesh::scan_i2c_bus(Wire, {room::kI2cSdaGpio, room::kI2cSclGpio}, found, sizeof(found));
    Serial.printf("i2c: scan sda=%d scl=%d found=%u", room::kI2cSdaGpio, room::kI2cSclGpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count && i < sizeof(found); ++i) {
        Serial.printf(" 0x%02X", found[i]);
    }
    Serial.println();

    Wire.setClock(room::kI2cClockHz);
    sht_present = i2c_present(room::kSht41Address);
    veml_present = veml.begin(&Wire);
    Serial.printf("sht41:    address=0x%02X present=%d\n", room::kSht41Address, sht_present ? 1 : 0);
    Serial.printf("veml7700: address=0x%02X present=%d\n", room::kVeml7700Address,
                  veml_present ? 1 : 0);
    if (veml_present) {
        veml.setGain(VEML7700_GAIN_1);
        veml.setIntegrationTime(VEML7700_IT_100MS);
    }

    splash(found, count);
    wait_servicing(room::kBootSplashHoldMs);
    draw_chrome();
}

void loop() {
    const unsigned long now = millis();

    // These three run every pass, independent of the display cadence and of the I2C bus. A
    // particulate alarm must not be blocked by an unrelated fault on the temperature bus.
    poll_pir(now);
    drain_sds011(now);
    beeper.service(now);
    // Ticked every pass so reconnect backoff runs on time, and so a fault on the I2C bus below
    // does not also take Home Assistant delivery down with it.
    atmosmesh::room_mqtt_runtime_tick(now);

    if (now - last_sample_ms < room::kSampleIntervalMs) {
        // Yield. Without this the loop task spins flat out and the FreeRTOS idle task never
        // runs, which is tolerable on a board with no radio and not on one with Wi-Fi and a
        // TCP stack to feed. 1 ms still polls the PIR ~50 times inside its 50 ms debounce.
        delay(1);
        return;
    }
    last_sample_ms = now;

    // Silence from the SDS011 is missing data, never clean air.
    const bool sds_fresh = sds_have_sample && (now - sds_last_frame_ms) <= room::kSdsStaleMs;
    if (!sds_fresh && sds_have_sample) {
        sds_have_sample = false;
        pm_alarm.mark_no_data();
        Serial.println(atmosmesh::format_sds011_no_frame_log().c_str());
        draw_status_dots();
    }

    bool alarm_beep_due = false;
    const bool sds_warming = now < room::kSdsWarmupMs;
    if (sds_fresh && !sds_warming) {
        // The fan and laser need to spin up; the first frames are not evidence of anything.
        if (pm_alarm.update(sds_pm25, sds_pm10, now)) {
            // Decided here, sounded at the end of the pass. Starting the pattern on this line
            // put a 60 ms pulse in front of the VEML7700's auto-ranging read, which walks gain
            // and integration time and blocks for about half a second; the pin stayed HIGH for
            // all of it and the first pulse measured 517 ms. Started after the slow work, the
            // whole 360 ms pattern runs in the idle remainder of the second, serviced every
            // loop pass.
            alarm_beep_due = true;
            Serial.printf("pm: ALARM pm2.5=%.1f pm10=%.1f -> beeping %d pulses\n",
                          static_cast<double>(sds_pm25), static_cast<double>(sds_pm10),
                          room::kBeeperAlarmPulses);
        }
    }

    if (bus_fault) {
        // Keep the diagnosis on screen rather than drawing readings over it, and re-probe so a
        // corrected wire brings the board up without another flash.
        const BusIdle again = probe_bus_idle();
        Serial.printf("i2c: fault sda=%s scl=%s | pir=%s pm=%s uptime=%lus\n",
                      again.sda_high ? "HIGH" : "LOW", again.scl_high ? "HIGH" : "LOW",
                      pir_motion ? "MOTION" : "clear", sds_fresh ? "ok" : "no-frame",
                      now / 1000UL);
        // The particulate and occupancy channels are independent of this bus, so they keep
        // reaching Home Assistant; temperature and light go out as unavailable.
        publish_room_state(now, false, 0.0F, 0.0F, false, 0.0F, sds_fresh);
        if (alarm_beep_due) {
            beeper.start(room::kBeeperAlarmPulses, millis());
        }
        if (again.healthy()) {
            Serial.println("i2c: bus recovered - restarting to run full init");
            delay(50);
            ESP.restart();
        }
        return;
    }

    float t_c = 0.0F;
    float rh = 0.0F;
    const ShtResult sht = sht_present ? read_sht41(&t_c, &rh) : ShtResult::NoAck;
    const bool sht_ok = sht == ShtResult::Ok;
    if (sht_ok) {
        sht_last_ok_ms = now;
    }
    service_beeper();

    float lux = 0.0F;
    bool veml_ok = false;
    if (veml_present) {
        lux = veml.readLux(VEML_LUX_AUTO);
        veml_ok = !std::isnan(lux);
        if (veml_ok) {
            veml_last_ok_ms = now;
        }
    }
    service_beeper();

    const bool alarm = pm_alarm.high();
    if (alarm != title_alarm_shown) {
        draw_title(alarm);
    }

    char buf[10];
    if (sht_ok) {
        std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(t_c));
        draw_cell(kCellTemp, true, buf, temp_fraction(t_c), temp_colour(t_c));
        std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(rh));
        draw_cell(kCellHum, true, buf, hum_fraction(rh), hum_colour(rh));
    } else {
        draw_cell(kCellTemp, false, "--.-", 0.0F, kMuted);
        draw_cell(kCellHum, false, "--", 0.0F, kMuted);
    }

    if (veml_ok) {
        if (lux < 1000.0F) {
            std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(lux));
        } else {
            std::snprintf(buf, sizeof(buf), "%.1fk", static_cast<double>(lux / 1000.0F));
        }
        draw_cell(kCellLux, true, buf, lux_fraction(lux), lux_colour(lux));
    } else {
        draw_cell(kCellLux, false, "----", 0.0F, kMuted);
    }

    if (sds_fresh) {
        std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(sds_pm25));
        draw_cell(kCellPm25, true, buf, pm25_fraction(sds_pm25), pm25_colour(sds_pm25));
        std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(sds_pm10));
        draw_cell(kCellPm10, true, buf, pm10_fraction(sds_pm10), pm10_colour(sds_pm10));
    } else {
        draw_cell(kCellPm25, false, "--.-", 0.0F, kMuted);
        draw_cell(kCellPm10, false, "--.-", 0.0F, kMuted);
    }

    draw_motion_cell(now);
    service_beeper();

    publish_room_state(now, sht_ok, t_c, rh, veml_ok, lux, sds_fresh);

    if (atmosmesh::room_mqtt_runtime_mqtt_up() != link_dots_shown_up ||
        atmosmesh::room_mqtt_runtime_wifi_up() != link_dots_shown_wifi) {
        draw_status_dots();
    }

    draw_heartbeat();

    const char* sht_note = sht == ShtResult::Ok          ? "ok"
                           : sht == ShtResult::NoAck     ? "no-ack"
                           : sht == ShtResult::ShortRead ? "short-read"
                                                         : "crc-rejected";
    Serial.printf(
        "room: t=%.1fC rh=%.1f%% lux=%.1f pm2.5=%.1f pm10=%.1f frames=%lu pir=%s(raw=%s) "
        "sht=%s veml=%s pm=%s link=%s uptime=%lus\n",
        static_cast<double>(sht_ok ? t_c : 0.0F), static_cast<double>(sht_ok ? rh : 0.0F),
        static_cast<double>(veml_ok ? lux : 0.0F), static_cast<double>(sds_fresh ? sds_pm25 : 0.0F),
        static_cast<double>(sds_fresh ? sds_pm10 : 0.0F), sds_frame_count,
        pir_motion ? "MOTION" : "clear", pir_raw_high ? "HIGH" : "LOW", sht_note,
        veml_ok ? "ok" : "error",
        !sds_fresh ? "no-data" : (sds_warming ? "warming" : (alarm ? "HIGH" : "ok")),
        !atmosmesh::room_mqtt_runtime_enabled() ? "off"
        : atmosmesh::room_mqtt_runtime_mqtt_up() ? "mqtt"
        : atmosmesh::room_mqtt_runtime_wifi_up() ? "wifi-only"
                                                 : "down",
        now / 1000UL);

    if (alarm_beep_due) {
        beeper.start(room::kBeeperAlarmPulses, millis());
    }
}

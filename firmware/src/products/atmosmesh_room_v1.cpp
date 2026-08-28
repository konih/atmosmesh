// AtmosMesh Room product composition root — ideaspark ESP32-WROOM-32 + 1.14 inch ST7789 TFT.
//
// Bring-up scope: VEML7700 ambient light and SHT41 temperature/humidity on one 3.3 V I2C bus,
// a D-SUN PIR on GPIO33, an SDS011 particulate sensor listened to on UART2, and a beeper on
// GPIO25 that sounds when particulates go high. No MQTT and no Wi-Fi yet.
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
bool heartbeat_on = false;
bool title_alarm_shown = false;
bool title_drawn = false;

struct Cell {
    const char* label;
    char shown[10];
    int shown_bar_px;
    std::uint16_t shown_colour;
};

Cell cells[kCellCount] = {
    {"TEMP C", "", -1, 0}, {"HUM %", "", -1, 0},  {"LUX", "", -1, 0},
    {"PM2.5", "", -1, 0},  {"PM10", "", -1, 0},   {"MOTION", "", -1, 0},
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

    void begin() {
        pinMode(room::kBeeperGpio, OUTPUT);
        digitalWrite(room::kBeeperGpio, LOW);
    }

    void start(int pulses, unsigned long now) {
        if (pulses <= 0) return;
        pulses_left = pulses;
        on = true;
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
            next_change_ms = now + room::kBeeperGapMs;
        } else if (pulses_left > 0) {
            digitalWrite(room::kBeeperGpio, HIGH);
            on = true;
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

void draw_heartbeat() {
    heartbeat_on = !heartbeat_on;
    tft.fillCircle(kW - 82, 9, 2,
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
    beeper.start(1, millis());   // one boot chirp: proof the beeper is wired, before any alarm

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
    delay(room::kBootSplashHoldMs);
    draw_chrome();
}

void loop() {
    const unsigned long now = millis();

    // These three run every pass, independent of the display cadence and of the I2C bus. A
    // particulate alarm must not be blocked by an unrelated fault on the temperature bus.
    poll_pir(now);
    drain_sds011(now);
    beeper.service(now);

    if (now - last_sample_ms < room::kSampleIntervalMs) {
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

    const bool sds_warming = now < room::kSdsWarmupMs;
    if (sds_fresh && !sds_warming) {
        // The fan and laser need to spin up; the first frames are not evidence of anything.
        if (pm_alarm.update(sds_pm25, sds_pm10, now)) {
            beeper.start(room::kBeeperAlarmPulses, now);
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

    float lux = 0.0F;
    bool veml_ok = false;
    if (veml_present) {
        lux = veml.readLux(VEML_LUX_AUTO);
        veml_ok = !std::isnan(lux);
    }

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

    if (now < room::kPirWarmupMs) {
        std::snprintf(buf, sizeof(buf), "%lus", (room::kPirWarmupMs - now) / 1000UL);
        draw_cell(kCellPir, true, buf, 0.0F, kCyan);
    } else if (pir_motion) {
        draw_cell(kCellPir, true, "YES", 1.0F, kAmber);
    } else if (pir_ever_moved) {
        const unsigned long age = (now - pir_last_motion_ms) / 1000UL;
        std::snprintf(buf, sizeof(buf), "%lus", age > 999UL ? 999UL : age);
        draw_cell(kCellPir, true, buf, 0.0F, kMuted);
    } else {
        draw_cell(kCellPir, false, "--", 0.0F, kMuted);
    }

    draw_heartbeat();

    const char* sht_note = sht == ShtResult::Ok          ? "ok"
                           : sht == ShtResult::NoAck     ? "no-ack"
                           : sht == ShtResult::ShortRead ? "short-read"
                                                         : "crc-rejected";
    Serial.printf(
        "room: t=%.1fC rh=%.1f%% lux=%.1f pm2.5=%.1f pm10=%.1f frames=%lu pir=%s(raw=%s) "
        "sht=%s veml=%s pm=%s uptime=%lus\n",
        static_cast<double>(sht_ok ? t_c : 0.0F), static_cast<double>(sht_ok ? rh : 0.0F),
        static_cast<double>(veml_ok ? lux : 0.0F), static_cast<double>(sds_fresh ? sds_pm25 : 0.0F),
        static_cast<double>(sds_fresh ? sds_pm10 : 0.0F), sds_frame_count,
        pir_motion ? "MOTION" : "clear", pir_raw_high ? "HIGH" : "LOW", sht_note,
        veml_ok ? "ok" : "error",
        !sds_fresh ? "no-data" : (sds_warming ? "warming" : (alarm ? "HIGH" : "ok")), now / 1000UL);
}

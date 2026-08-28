// AtmosMesh Room product composition root — ideaspark ESP32-WROOM-32 + 1.14 inch ST7789 TFT.
//
// Bring-up scope (ROOM-01): VEML7700 ambient light and SHT41 temperature/humidity on one 3.3 V
// I2C bus, rendered in colour on the on-board display. No MQTT, no Wi-Fi, no 5 V devices —
// the SDS011, the PIR and the buzzer are not part of this build.
//
// D-024: the display is SPI and owns GPIO23/18/15/2/4/32. GPIO12 is the flash strap and
// GPIO1/GPIO3 are the CP2102 USB-UART. None of them are touched here except as the display's own.
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_VEML7700.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/room_pins.hpp"
#include "atmosmesh/sht4x_frame.hpp"

namespace room = atmosmesh::room;

namespace {

// ---------------------------------------------------------------- palette

constexpr std::uint16_t rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return static_cast<std::uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr std::uint16_t kBackground = rgb(8, 10, 16);
constexpr std::uint16_t kPanel = rgb(20, 24, 36);
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
constexpr std::uint16_t kYellow = rgb(255, 226, 92);
constexpr std::uint16_t kWhite = rgb(245, 245, 245);

// ---------------------------------------------------------------- geometry

constexpr int kW = room::kScreenWidthPx;    // 240
constexpr int kH = room::kScreenHeightPx;   // 135
constexpr int kTitleH = 18;
constexpr int kRowCount = 3;
constexpr int kRowH = (kH - kTitleH) / kRowCount;   // 39
constexpr int kValueRightX = 196;
constexpr int kBarX = 6;
constexpr int kBarW = kW - 2 * kBarX - 8;
constexpr int kBarH = 4;

int row_top(int i) { return kTitleH + i * kRowH; }

// ---------------------------------------------------------------- state

Adafruit_ST7789 tft(room::kTftCsGpio, room::kTftDcGpio, room::kTftRstGpio);
Adafruit_VEML7700 veml;

bool veml_present = false;
bool bus_fault = false;
bool sht_present = false;
unsigned long last_sample_ms = 0;
bool heartbeat_on = false;

struct Metric {
    const char* label;
    const char* unit;
    char shown[12];
    int shown_bar_px;
    std::uint16_t shown_colour;
};

Metric metrics[kRowCount] = {
    {"TEMP", "C", "", -1, 0},
    {"HUM", "%", "", -1, 0},
    {"LUX", "lx", "", -1, 0},
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

float clamp01(float v) { return v < 0.0F ? 0.0F : (v > 1.0F ? 1.0F : v); }

float temp_fraction(float c) { return clamp01((c - 0.0F) / 40.0F); }
float hum_fraction(float pct) { return clamp01(pct / 100.0F); }
// Light is logarithmic to the eye; a linear bar would sit at zero indoors.
float lux_fraction(float lx) { return clamp01(std::log10(lx + 1.0F) / 5.0F); }

// Read the two lines as plain GPIOs BEFORE handing them to the I2C driver.
// An idle I2C bus rests HIGH on both. If either is held LOW, Wire.begin() can block forever
// inside its bus-recovery loop -- which is a hang with no panic and no watchdog, i.e. the
// least diagnosable failure this board can produce. This probe turns that into a message.
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

void draw_chrome() {
    tft.fillScreen(kBackground);
    tft.fillRect(0, 0, kW, kTitleH, kTitleBar);
    draw_text(6, 5, "ATMOSMESH", 1, kTitleText);
    draw_text(66, 5, "ROOM", 1, kCyan);

    for (int i = 0; i < kRowCount; ++i) {
        const int t = row_top(i);
        if (i > 0) {
            tft.drawFastHLine(0, t, kW, kRule);
        }
        draw_text(6, t + 6, metrics[i].label, 1, kLabel);
        draw_text(kValueRightX + 6, t + 24, metrics[i].unit, 1, kLabel);
        tft.fillRect(kBarX, t + 33, kBarW, kBarH, kBarTrack);
    }
}

// Only the value text and the bar move, so the chrome is drawn once and never flickers.
void draw_metric(int i, bool valid, const char* text, float fraction, std::uint16_t colour) {
    Metric& m = metrics[i];
    const int t = row_top(i);
    const std::uint16_t shade = valid ? colour : kMuted;

    if (std::strcmp(m.shown, text) != 0 || m.shown_colour != shade) {
        tft.fillRect(70, t + 6, kValueRightX - 70 + 2, 26, kBackground);
        draw_right(kValueRightX, t + 8, text, 3, shade);
        std::snprintf(m.shown, sizeof(m.shown), "%s", text);
        m.shown_colour = shade;
    }

    const int fill = valid ? static_cast<int>(fraction * static_cast<float>(kBarW)) : 0;
    if (fill != m.shown_bar_px) {
        tft.fillRect(kBarX, t + 33, kBarW, kBarH, kBarTrack);
        if (fill > 0) {
            tft.fillRect(kBarX, t + 33, fill, kBarH, shade);
        }
        m.shown_bar_px = fill;
    }
}

void draw_sensor_dots() {
    tft.fillCircle(kW - 46, 9, 3, sht_present ? kGreen : kRed);
    draw_text(kW - 40, 5, "T", 1, sht_present ? kGreen : kRed);
    tft.fillCircle(kW - 24, 9, 3, veml_present ? kGreen : kRed);
    draw_text(kW - 18, 5, "L", 1, veml_present ? kGreen : kRed);
}

void draw_heartbeat() {
    heartbeat_on = !heartbeat_on;
    tft.fillCircle(kW - 62, 9, 2, heartbeat_on ? kCyan : kTitleBar);
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
    draw_text(10, 14, "ATMOSMESH", 3, kTitleText);
    draw_text(10, 44, "ROOM", 3, kCyan);
    draw_text(10, 76, "ideaspark ESP32 1.14 TFT", 1, kLabel);

    char line[40];
    std::snprintf(line, sizeof(line), "I2C sda=%d scl=%d  found %u",
                  room::kI2cSdaGpio, room::kI2cSclGpio, static_cast<unsigned>(count));
    draw_text(10, 92, line, 1, kMuted);

    char addrs[40] = "";
    int used = 0;
    for (std::size_t i = 0; i < count && used < 30; ++i) {
        used += std::snprintf(addrs + used, sizeof(addrs) - used, "0x%02X ", found[i]);
    }
    draw_text(10, 106, count ? addrs : "no devices", 1, count ? kGreen : kRed);
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
    draw_sensor_dots();
}

void loop() {
    const unsigned long now = millis();
    if (now - last_sample_ms < room::kSampleIntervalMs) {
        return;
    }
    last_sample_ms = now;

    if (bus_fault) {
        // Keep the diagnosis on screen rather than drawing readings over it, and re-probe so a
        // corrected wire brings the board up without another flash.
        const BusIdle again = probe_bus_idle();
        Serial.printf("i2c: fault sda=%s scl=%s uptime=%lus\n", again.sda_high ? "HIGH" : "LOW",
                      again.scl_high ? "HIGH" : "LOW", now / 1000UL);
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

    char buf[12];
    if (sht_ok) {
        std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(t_c));
        draw_metric(0, true, buf, temp_fraction(t_c), temp_colour(t_c));
        std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(rh));
        draw_metric(1, true, buf, hum_fraction(rh), hum_colour(rh));
    } else {
        draw_metric(0, false, "--.-", 0.0F, kMuted);
        draw_metric(1, false, "--", 0.0F, kMuted);
    }

    if (veml_ok) {
        if (lux < 1000.0F) {
            std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(lux));
        } else {
            std::snprintf(buf, sizeof(buf), "%.1fk", static_cast<double>(lux / 1000.0F));
        }
        draw_metric(2, true, buf, lux_fraction(lux), lux_colour(lux));
    } else {
        draw_metric(2, false, "----", 0.0F, kMuted);
    }

    draw_heartbeat();

    const char* sht_note = sht == ShtResult::Ok        ? "ok"
                           : sht == ShtResult::NoAck   ? "no-ack"
                           : sht == ShtResult::ShortRead ? "short-read"
                                                         : "crc-rejected";
    Serial.printf("room: t=%.1fC rh=%.1f%% lux=%.1f sht=%s veml=%s uptime=%lus\n",
                  static_cast<double>(sht_ok ? t_c : 0.0F),
                  static_cast<double>(sht_ok ? rh : 0.0F),
                  static_cast<double>(veml_ok ? lux : 0.0F), sht_note, veml_ok ? "ok" : "error",
                  now / 1000UL);
}

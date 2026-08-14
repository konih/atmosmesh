#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>

#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/oled_address.hpp"
#include "atmosmesh/oled_profile.hpp"
#include "atmosmesh/pins.hpp"
#include "atmosmesh/sds011_frame.hpp"

namespace {

DHT am2302(atmosmesh::kAm2302DataGpio, DHT22);
Adafruit_SSD1306* ssd1306 = nullptr;
Adafruit_SH1106G* sh1106 = nullptr;
Adafruit_GFX* panel = nullptr;
void (*oled_clear)() = nullptr;
void (*oled_flush)() = nullptr;
atmosmesh::OledProfile oled_profile{};
bool oled_ready = false;
int bmp_address = -1;
atmosmesh::Sds011Stream sds011{};
bool pm_ok = false;
float pm25_ug_m3 = 0.0F;
float pm10_ug_m3 = 0.0F;
unsigned long last_pm_ms = 0;
constexpr unsigned long kPmStaleMs = 5000;

void log_scan(const char* label, const atmosmesh::I2cBusMap& pins, const std::uint8_t* found,
              std::size_t count) {
    Serial.printf("i2c scan %s sda=%d scl=%d devices=%u\n", label, pins.sda_gpio, pins.scl_gpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        Serial.printf("  addr=0x%02X\n", found[i]);
    }
}

void release_oled() {
    delete ssd1306;
    ssd1306 = nullptr;
    delete sh1106;
    sh1106 = nullptr;
    panel = nullptr;
    oled_clear = nullptr;
    oled_flush = nullptr;
}

void show_lines(const std::string* lines, std::size_t count) {
    if (!oled_ready || panel == nullptr || oled_clear == nullptr || oled_flush == nullptr) {
        return;
    }
    oled_clear();
    panel->setTextSize(1);
    panel->setTextColor(1);
    panel->setCursor(oled_profile.column_offset_px, 0);
    for (std::size_t i = 0; i < count; ++i) {
        panel->setCursor(oled_profile.column_offset_px, static_cast<int16_t>(i * 8));
        panel->print(lines[i].c_str());
    }
    oled_flush();
}

bool try_oled_bus(const char* label, const atmosmesh::I2cBusMap& pins, atmosmesh::I2cDevice* out) {
    std::uint8_t found[16];
    const std::size_t count = atmosmesh::scan_i2c_bus(Wire, pins, found, sizeof(found));
    log_scan(label, pins, found, count > sizeof(found) ? sizeof(found) : count);
    const int address = atmosmesh::pick_oled_address(found, count);
    if (address < 0) {
        return false;
    }
    out->address = static_cast<std::uint8_t>(address);
    out->pins = pins;
    return true;
}

bool begin_oled(std::uint8_t address, const atmosmesh::OledProfile& profile) {
    release_oled();
    Wire.setClock(atmosmesh::kOledI2cHz);

    if (profile.controller == atmosmesh::OledController::Sh1106) {
        sh1106 = new Adafruit_SH1106G(profile.width_px, profile.height_px, &Wire, -1);
        if (!sh1106->begin(address, true)) {
            Serial.printf("oled: init error controller=SH1106 addr=0x%02X height=%d\n", address,
                          profile.height_px);
            release_oled();
            return false;
        }
        panel = sh1106;
        oled_clear = []() { sh1106->clearDisplay(); };
        oled_flush = []() { sh1106->display(); };
    } else {
        ssd1306 = new Adafruit_SSD1306(profile.width_px, profile.height_px, &Wire, -1,
                                       atmosmesh::kOledI2cHz, atmosmesh::kOledI2cHz);
        if (!ssd1306->begin(SSD1306_SWITCHCAPVCC, address)) {
            Serial.printf("oled: init error controller=SSD1306 addr=0x%02X height=%d\n", address,
                          profile.height_px);
            release_oled();
            return false;
        }
        ssd1306->ssd1306_command(SSD1306_SETCOMPINS);
        ssd1306->ssd1306_command(atmosmesh::oled_compins_arg(profile.com_pins));
        Wire.setClock(atmosmesh::kOledI2cHz);
        panel = ssd1306;
        oled_clear = []() { ssd1306->clearDisplay(); };
        oled_flush = []() { ssd1306->display(); };
    }

    oled_profile = profile;
    panel->setRotation(0);
    Serial.println(atmosmesh::format_oled_init_log(profile, address).c_str());
    Serial.printf("oled: sda=%d scl=%d column_offset=%d\n", atmosmesh::kOledSdaGpio,
                  atmosmesh::kOledSclGpio, profile.column_offset_px);
    return true;
}

void draw_probe_pattern() {
    if (panel == nullptr || oled_clear == nullptr || oled_flush == nullptr) {
        return;
    }
    oled_clear();
    panel->fillRect(oled_profile.column_offset_px, 0, oled_profile.width_px, 8, 1);
    panel->setTextSize(1);
    panel->setTextColor(1);
    panel->setCursor(oled_profile.column_offset_px, 10);
    panel->print("AtmosMesh");
    oled_flush();
    Serial.println("oled: probe bar=row0 text=line1");
}

void setup_oled() {
    atmosmesh::I2cDevice device{};
    const atmosmesh::I2cBusMap primary{atmosmesh::kOledSdaGpio, atmosmesh::kOledSclGpio};
    const atmosmesh::I2cBusMap swapped{atmosmesh::kOledSclGpio, atmosmesh::kOledSdaGpio};

    bool found = try_oled_bus("oled d5=sda d4=scl", primary, &device);
    if (!found) {
        found = try_oled_bus("oled swapped", swapped, &device);
    }
    if (!found) {
        Serial.println("oled: no i2c device at 0x3C/0x3D on GPIO5/GPIO4");
        return;
    }

    Serial.printf("oled address=0x%02X sda=%d scl=%d\n", device.address, device.pins.sda_gpio,
                  device.pins.scl_gpio);

    const auto compiled = atmosmesh::compiled_oled_profile();
    const atmosmesh::OledProfile candidates[] = {
        compiled,
        atmosmesh::resolve_oled_profile(compiled.controller, compiled.height_px == 64 ? 32 : 64),
        atmosmesh::resolve_oled_profile(compiled.controller == atmosmesh::OledController::Sh1106
                                            ? atmosmesh::OledController::Ssd1306
                                            : atmosmesh::OledController::Sh1106,
                                        64),
    };

    bool started = false;
    for (const auto& candidate : candidates) {
        if (begin_oled(device.address, candidate)) {
            started = true;
            break;
        }
    }
    if (!started) {
        Serial.println("oled: init failed");
        return;
    }

    oled_ready = true;
    draw_probe_pattern();
    delay(400);
    const auto lines = atmosmesh::dummy_banner();
    show_lines(lines.data(), lines.size());
}

void setup_bmp280() {
    const atmosmesh::I2cBusMap pins{atmosmesh::kSensorSdaGpio, atmosmesh::kSensorSclGpio};
    std::uint8_t found[16];
    const std::size_t count = atmosmesh::scan_i2c_bus(Wire1, pins, found, sizeof(found));
    log_scan("bmp sda=21 scl=19", pins, found, count > sizeof(found) ? sizeof(found) : count);
    bmp_address = atmosmesh::pick_bmp_address(found, count);
    if (bmp_address < 0) {
        Serial.println("bmp280: no i2c device on GPIO21/GPIO19");
        Serial.println("bmp280: CSB must be 3V3, SDO to GND for 0x76, VCC to 3V3");
        return;
    }

    Wire1.beginTransmission(static_cast<std::uint8_t>(bmp_address));
    Wire1.write(0xD0);
    if (Wire1.endTransmission() != 0) {
        Serial.printf("bmp280: failed to select chip-id register at 0x%02X\n", bmp_address);
        bmp_address = -1;
        return;
    }
    if (Wire1.requestFrom(static_cast<std::uint8_t>(bmp_address), static_cast<std::uint8_t>(1)) != 1) {
        Serial.println("bmp280: chip-id read failed");
        bmp_address = -1;
        return;
    }
    const std::uint8_t chip_id = static_cast<std::uint8_t>(Wire1.read());
    Serial.printf("bmp280: addr=0x%02X chip_id=0x%02X\n", bmp_address, chip_id);
    if (!atmosmesh::is_bmp_family_id(chip_id)) {
        Serial.println("bmp280: unexpected chip id (0x58=BMP280, 0x60=BME280)");
    }
}

void poll_sds011() {
    while (Serial2.available() > 0) {
        const auto sample = sds011.feed(static_cast<std::uint8_t>(Serial2.read()));
        if (!sample.ok) {
            continue;
        }
        pm_ok = true;
        pm25_ug_m3 = sample.pm25_ug_m3;
        pm10_ug_m3 = sample.pm10_ug_m3;
        last_pm_ms = millis();
        Serial.printf("sds011: pm25=%.1f ug/m3 pm10=%.1f ug/m3\n", static_cast<double>(pm25_ug_m3),
                      static_cast<double>(pm10_ug_m3));
    }
    if (pm_ok && (millis() - last_pm_ms) > kPmStaleMs) {
        pm_ok = false;
        Serial.println("sds011: no AA C0 frame (MQ135 analog is not UART; check sensor TX->GPIO16)");
    }
}

}  // namespace

void setup() {
    Serial.begin(atmosmesh::kSerialBaud);
    delay(200);
    Serial.println("atmosmesh bench bring-up");
    Serial.println("OLED VCC=3V3; BMP280 VCC=3V3 CSB=3V3 SDO=GND; AM2302 VDD=3V3 data=GPIO18");
    Serial.println("sds011: VCC=5V only; UART 3.3V; sensor TX -> GPIO16 RX2; ESP TX2 GPIO17 -> sensor RX");
    Serial.println("mq135: analog via divider to GPIO34, not UART. Do not put 5V on GPIO16/17");

    setup_oled();
    setup_bmp280();
    am2302.begin();
    Serial.printf("am2302: data=GPIO%d\n", atmosmesh::kAm2302DataGpio);
    Serial2.begin(atmosmesh::kSds011Baud, SERIAL_8N1, atmosmesh::kSds011RxGpio,
                  atmosmesh::kSds011TxGpio);
    Serial.printf("sds011: uart2 rx=GPIO%d tx=GPIO%d baud=%d\n", atmosmesh::kSds011RxGpio,
                  atmosmesh::kSds011TxGpio, atmosmesh::kSds011Baud);
}

void loop() {
    delay(atmosmesh::kAm2302MinIntervalMs);
    poll_sds011();
    const float humidity = am2302.readHumidity();
    const float temperature = am2302.readTemperature();
    const bool am_ok = !isnan(humidity) && !isnan(temperature);

    if (am_ok) {
        Serial.printf("am2302: t=%.1fC rh=%.1f%%\n", temperature, humidity);
    } else {
        Serial.println("am2302: read failed (need 3V3, 10k pull-up to 3V3 if the module has none)");
    }

    const auto lines = atmosmesh::live_sensor_lines(am_ok, temperature, humidity, bmp_address >= 0,
                                                    bmp_address, pm_ok, pm25_ug_m3, pm10_ug_m3);
    show_lines(lines.data(), lines.size());
}

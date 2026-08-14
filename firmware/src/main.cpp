#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>

#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/oled_address.hpp"
#include "atmosmesh/pins.hpp"

namespace {

DHT am2302(atmosmesh::kAm2302DataGpio, DHT22);
Adafruit_SSD1306* oled = nullptr;
bool oled_ready = false;
int bmp_address = -1;

void log_scan(const char* label, const atmosmesh::I2cBusMap& pins, const std::uint8_t* found,
              std::size_t count) {
    Serial.printf("i2c scan %s sda=%d scl=%d devices=%u\n", label, pins.sda_gpio, pins.scl_gpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        Serial.printf("  addr=0x%02X\n", found[i]);
    }
}

void show_lines(const std::string* lines, std::size_t count) {
    if (!oled_ready || oled == nullptr) {
        return;
    }
    oled->clearDisplay();
    oled->setTextSize(1);
    oled->setTextColor(SSD1306_WHITE);
    oled->setCursor(0, 0);
    for (std::size_t i = 0; i < count; ++i) {
        oled->println(lines[i].c_str());
    }
    oled->display();
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

bool begin_oled(std::uint8_t address, int height_px) {
    delete oled;
    oled = new Adafruit_SSD1306(atmosmesh::kOledWidthPx, height_px, &Wire, -1);
    if (!oled->begin(SSD1306_SWITCHCAPVCC, address)) {
        Serial.printf("oled: init error addr=0x%02X height=%d\n", address, height_px);
        delete oled;
        oled = nullptr;
        return false;
    }
    Serial.printf("oled: init ok addr=0x%02X height=%d sda=%d scl=%d\n", address, height_px,
                  atmosmesh::kOledSdaGpio, atmosmesh::kOledSclGpio);
    return true;
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

    if (!begin_oled(device.address, atmosmesh::kOledHeightPx) &&
        !begin_oled(device.address, atmosmesh::kOledHeightPxAlt)) {
        Serial.println("oled: init failed");
        return;
    }

    oled_ready = true;
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

}  // namespace

void setup() {
    Serial.begin(atmosmesh::kSerialBaud);
    delay(200);
    Serial.println("atmosmesh bench bring-up");
    Serial.println("OLED VCC=3V3; BMP280 VCC=3V3 CSB=3V3 SDO=GND; AM2302 VDD=3V3 data=GPIO18");

    setup_oled();
    setup_bmp280();
    am2302.begin();
    Serial.printf("am2302: data=GPIO%d\n", atmosmesh::kAm2302DataGpio);
}

void loop() {
    delay(atmosmesh::kAm2302MinIntervalMs);
    const float humidity = am2302.readHumidity();
    const float temperature = am2302.readTemperature();
    const bool am_ok = !isnan(humidity) && !isnan(temperature);

    if (am_ok) {
        Serial.printf("am2302: t=%.1fC rh=%.1f%%\n", temperature, humidity);
    } else {
        Serial.println("am2302: read failed (need 3V3, 10k pull-up to 3V3 if the module has none)");
    }

    const auto lines =
        atmosmesh::live_sensor_lines(am_ok, temperature, humidity, bmp_address >= 0, bmp_address);
    show_lines(lines.data(), lines.size());
}

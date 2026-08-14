#include <Arduino.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/lcd_address.hpp"
#include "atmosmesh/lcd_bus.hpp"
#include "atmosmesh/pins.hpp"

namespace {

DHT am2302(atmosmesh::kAm2302DataGpio, DHT22);
LiquidCrystal_I2C* lcd = nullptr;
bool lcd_ready = false;
int bmp_address = -1;

void log_scan(const char* label, const atmosmesh::I2cBusMap& pins, const std::uint8_t* found,
              std::size_t count) {
    Serial.printf("i2c scan %s sda=%d scl=%d devices=%u\n", label, pins.sda_gpio, pins.scl_gpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        Serial.printf("  addr=0x%02X\n", found[i]);
    }
}

bool try_lcd_bus(const char* label, const atmosmesh::I2cBusMap& pins, atmosmesh::I2cDevice* out) {
    std::uint8_t found[16];
    const std::size_t count = atmosmesh::scan_i2c_bus(Wire, pins, found, sizeof(found));
    log_scan(label, pins, found, count > sizeof(found) ? sizeof(found) : count);
    const int address = atmosmesh::pick_lcd_address(found, count);
    if (address < 0) {
        return false;
    }
    out->address = static_cast<std::uint8_t>(address);
    out->pins = pins;
    return true;
}

void setup_lcd() {
    atmosmesh::I2cDevice device{};
    const atmosmesh::I2cBusMap primary{atmosmesh::kLcdSdaGpio, atmosmesh::kLcdSclGpio};
    const atmosmesh::I2cBusMap swapped{atmosmesh::kLcdSclGpio, atmosmesh::kLcdSdaGpio};

    bool found = try_lcd_bus("lcd d5=sda d4=scl", primary, &device);
    if (!found) {
        found = try_lcd_bus("lcd swapped", swapped, &device);
    }
    if (!found) {
        Serial.println("lcd: no i2c device on GPIO5/GPIO4");
        return;
    }

    Serial.printf("lcd address=0x%02X sda=%d scl=%d\n", device.address, device.pins.sda_gpio,
                  device.pins.scl_gpio);
    lcd = new LiquidCrystal_I2C(device.address, atmosmesh::kLcdColumns, atmosmesh::kLcdRows);
    lcd->init();
    lcd->backlight();
    lcd->clear();
    const auto lines = atmosmesh::dummy_banner();
    lcd->setCursor(0, 0);
    lcd->print(lines[0].c_str());
    lcd->setCursor(0, 1);
    lcd->print(lines[1].c_str());
    lcd_ready = true;
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
    Serial.println("LCD VCC=3V3; BMP280 VCC=3V3 CSB=3V3 SDO=GND; AM2302 VDD=3V3 data=GPIO18");

    setup_lcd();
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

    if (!lcd_ready || lcd == nullptr) {
        return;
    }
    lcd->clear();
    lcd->setCursor(0, 0);
    char line0[17];
    if (am_ok) {
        snprintf(line0, sizeof(line0), "T%5.1f H%4.1f", temperature, humidity);
    } else {
        snprintf(line0, sizeof(line0), "AM2302 missing");
    }
    lcd->print(line0);
    lcd->setCursor(0, 1);
    char line1[17];
    if (bmp_address >= 0) {
        snprintf(line1, sizeof(line1), "BMP I2C 0x%02X", static_cast<unsigned>(bmp_address) & 0xFFU);
    } else {
        snprintf(line1, sizeof(line1), "BMP280 missing");
    }
    lcd->print(line1);
}

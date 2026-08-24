// AtmosMesh Grove v1.5 product composition root.
#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cmath>

#include "atmosmesh/grove_mqtt_runtime.hpp"
#include "atmosmesh/grove_status.hpp"
#include "atmosmesh/product_profile.hpp"
#include "atmosmesh/rc_light.hpp"

namespace {

constexpr unsigned long kSampleIntervalMs = 2500;
constexpr std::uint32_t kI2cClockHz = 100000;
constexpr std::uint8_t kOledAddresses[] = {0x3C, 0x3D};
constexpr std::uint8_t kBmp180Address = 0x77;

const auto& profile = atmosmesh::grove_profile();
DHT dht(profile.dht_data_gpio, DHT11);
Adafruit_BMP085 bmp180;
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE,
                                             profile.i2c_scl_gpio, profile.i2c_sda_gpio);
atmosmesh::GroveReadings readings{};
atmosmesh::RcLightState light_state{};
bool oled_ready = false;
bool bmp_ready = false;
unsigned long last_sample_ms = 0;

bool i2c_address_present(std::uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

int find_oled_address() {
    for (const auto address : kOledAddresses) {
        if (i2c_address_present(address)) {
            return address;
        }
    }
    return -1;
}

void render() {
    if (!oled_ready) {
        return;
    }
    const auto lines = atmosmesh::grove_oled_lines(readings);
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x7_tf);
    constexpr int kBaselines[] = {7, 15, 23, 31};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        oled.drawStr(0, kBaselines[i], lines[i].c_str());
    }
    oled.sendBuffer();
}

void apply_light_pin_action(atmosmesh::RcLightPinAction action) {
    switch (action) {
        case atmosmesh::RcLightPinAction::DriveLow:
            pinMode(profile.light_rc_gpio, OUTPUT);
            digitalWrite(profile.light_rc_gpio, LOW);
            break;
        case atmosmesh::RcLightPinAction::ReleaseInput:
            pinMode(profile.light_rc_gpio, INPUT);
            break;
        case atmosmesh::RcLightPinAction::None:
            break;
    }
}

void publish_grove_state() {
    atmosmesh::GroveMqttState state{};
    state.temperature_c = {readings.dht_temperature.value, readings.dht_temperature.valid, 0};
    state.humidity_pct = {readings.humidity.value, readings.humidity.valid, 0};
    state.pressure_hpa = {readings.pressure.value, readings.pressure.valid, 0};
    state.light_charge_us = {static_cast<float>(readings.light.charge_us), readings.light.valid, 0};
    atmosmesh::grove_mqtt_runtime_publish_state(state);
}

void service_light_measurement() {
    if (!atmosmesh::rc_light_active(light_state)) {
        return;
    }
    const bool input_high = light_state.phase == atmosmesh::RcLightPhase::Charging &&
                            digitalRead(profile.light_rc_gpio) == HIGH;
    const auto step = atmosmesh::rc_light_tick(light_state, micros(), input_high);
    apply_light_pin_action(step.pin_action);
    if (!step.completed) {
        return;
    }
    readings.light = light_state.measurement;
    Serial.println(atmosmesh::rc_light_serial_text(light_state).c_str());
    Serial.printf("health: %s light=%s\n", atmosmesh::grove_health_text(readings).c_str(),
                  readings.light.valid ? "ok" : "unavailable");
    render();
    publish_grove_state();
}

void setup_oled() {
    const int address = find_oled_address();
    if (address < 0) {
        Serial.println("oled: error not found at 0x3C/0x3D");
        return;
    }
    oled.setI2CAddress(static_cast<std::uint8_t>(address << 1));
    oled.setBusClock(kI2cClockHz);
    oled_ready = oled.begin();
    Serial.printf("oled: %s addr=0x%02X geometry=%dx%d\n", oled_ready ? "ok" : "error",
                  address, profile.oled_width_px, profile.oled_height_px);
}

void setup_bmp180() {
    if (!i2c_address_present(kBmp180Address)) {
        Serial.println("bmp180: error not found at 0x77");
        return;
    }
    bmp_ready = bmp180.begin(BMP085_STANDARD, &Wire);
    // Keep the operator's non-default bus pins explicit even if a library calls Wire.begin().
    Wire.begin(profile.i2c_sda_gpio, profile.i2c_scl_gpio);
    Wire.setClock(kI2cClockHz);
    Serial.printf("bmp180: %s addr=0x77\n", bmp_ready ? "ok" : "error");
}

bool prepare_bmp180_sample() {
    const bool address_present = i2c_address_present(kBmp180Address);
    switch (atmosmesh::grove_bmp_action(address_present, bmp_ready)) {
        case atmosmesh::GroveBmpAction::Unavailable:
            if (bmp_ready) {
                Serial.println("bmp180: lost addr=0x77; prior reading invalidated");
            }
            bmp_ready = false;
            atmosmesh::invalidate_grove_bmp(readings);
            return false;
        case atmosmesh::GroveBmpAction::Initialize:
            bmp_ready = bmp180.begin(BMP085_STANDARD, &Wire);
            // Adafruit_BMP085 may call Wire.begin(); restore the wired D2/D3 bus explicitly.
            Wire.begin(profile.i2c_sda_gpio, profile.i2c_scl_gpio);
            Wire.setClock(kI2cClockHz);
            Serial.printf("bmp180: %s addr=0x77 after probe\n",
                          bmp_ready ? "recovered" : "initialize error");
            if (!bmp_ready) {
                atmosmesh::invalidate_grove_bmp(readings);
            }
            return bmp_ready;
        case atmosmesh::GroveBmpAction::Read:
            return true;
    }
    return false;
}

void sample_sensors() {
    const float humidity = dht.readHumidity();
    const float dht_temperature = dht.readTemperature();
    const bool dht_valid = std::isfinite(humidity) && std::isfinite(dht_temperature);
    readings.humidity = {dht_valid, dht_valid ? humidity : 0.0F};
    readings.dht_temperature = {dht_valid, dht_valid ? dht_temperature : 0.0F};

    bool bmp_valid = false;
    float bmp_temperature = 0.0F;
    float pressure_hpa = 0.0F;
    if (prepare_bmp180_sample()) {
        bmp_temperature = bmp180.readTemperature();
        pressure_hpa = bmp180.readPressure() / 100.0F;
        bmp_valid = std::isfinite(bmp_temperature) && std::isfinite(pressure_hpa) &&
                    pressure_hpa >= 300.0F && pressure_hpa <= 1100.0F;
    }
    if (bmp_valid) {
        readings.bmp_temperature = {true, bmp_temperature};
        readings.pressure = {true, pressure_hpa};
    } else {
        atmosmesh::invalidate_grove_bmp(readings);
    }

    if (dht_valid) {
        Serial.printf("dht11: ok temperature=%.1f C humidity=%.1f %%RH\n",
                      static_cast<double>(dht_temperature), static_cast<double>(humidity));
    } else {
        Serial.println("dht11: error reading unavailable");
    }
    if (bmp_valid) {
        Serial.printf("bmp180: ok temperature=%.1f C pressure=%.1f hPa\n",
                      static_cast<double>(bmp_temperature), static_cast<double>(pressure_hpa));
    } else {
        Serial.println("bmp180: error reading unavailable");
    }
    readings.light = {};
    apply_light_pin_action(atmosmesh::rc_light_begin(light_state, micros()).pin_action);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("product=%s product_id=%s variant=%s station_id=%s\n", profile.product_name,
                  profile.product_id, profile.product_variant, profile.station_id);
    Serial.printf("i2c: SDA=D2/GPIO%d SCL=D3/GPIO%d clock=%lu Hz\n", profile.i2c_sda_gpio,
                  profile.i2c_scl_gpio, static_cast<unsigned long>(kI2cClockHz));
    Serial.println("boot-warning: D3/GPIO0 must remain HIGH during reset");
    Serial.printf("dht11: DATA=D5/GPIO%d (communication observed; values uncalibrated)\n",
                  profile.dht_data_gpio);
    Serial.printf("light: D7/GPIO%d RC raw microseconds; lower=brighter; A0 unused\n",
                  profile.light_rc_gpio);
    Serial.println("power: OLED, BMP180, DHT11 and LDR RC are 3V3 only");

    Wire.begin(profile.i2c_sda_gpio, profile.i2c_scl_gpio);
    Wire.setClock(kI2cClockHz);
    setup_oled();
    setup_bmp180();
    dht.begin();
    pinMode(profile.light_rc_gpio, INPUT);
    Serial.println("dht11: initialized; first read follows minimum interval");
    atmosmesh::grove_mqtt_runtime_begin();
    render();
    last_sample_ms = millis();
}

void loop() {
    const unsigned long now = millis();
    service_light_measurement();
    if (!atmosmesh::rc_light_active(light_state) && now - last_sample_ms >= kSampleIntervalMs) {
        last_sample_ms = now;
        sample_sensors();
    }
    // Defer socket work for at most the bounded RC window so connect latency cannot distort timing.
    if (!atmosmesh::rc_light_active(light_state)) {
        atmosmesh::grove_mqtt_runtime_tick(now);
    }
    yield();
}

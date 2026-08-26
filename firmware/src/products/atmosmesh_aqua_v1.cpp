// AtmosMesh Aqua product composition root. See story AQ-01 and D-019/ADR-0002.
// Board and water-probe hardware identity are unconfirmed (AQ-01 open questions 1-2): pin values
// below are provisional until photographed and reviewed.
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cstdio>
#include <string>

#include "atmosmesh/aqua_mqtt_runtime.hpp"
#include "atmosmesh/aqua_status.hpp"
#include "atmosmesh/product_profile.hpp"
#include "atmosmesh/sht4x_frame.hpp"
#include "atmosmesh/soil_sampler.hpp"

namespace {

constexpr unsigned long kSampleIntervalMs = 2500;
constexpr std::uint32_t kI2cClockHz = 100000;
constexpr std::uint8_t kOledAddresses[] = {0x3C, 0x3D};

const auto& profile = atmosmesh::aqua_profile();
U8G2_SSD1306_128X64_ALT0_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, profile.i2c_scl_gpio,
                                       profile.i2c_sda_gpio);
atmosmesh::AquaReadings readings{};
atmosmesh::SoilSamplerState water_state{};
bool oled_ready = false;
bool sht41_ready = false;
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
    const auto lines = atmosmesh::aqua_oled_lines(readings);
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tf);
    constexpr int kBaselines[] = {14, 28, 42, 56};
    for (std::size_t i = 0; i < lines.size(); ++i) {
        oled.drawStr(0, kBaselines[i], lines[i].c_str());
    }
    oled.sendBuffer();
}

void write_pin_level(int gpio, bool high) {
    digitalWrite(gpio, high ? HIGH : LOW);
}

// This product has no soil sensor: report the reused sampler's state under the "water:" name so
// serial diagnostics never call an ADC reading "soil" for a probe that isn't one (AQ-01 scope).
std::string water_sampler_serial_text(const atmosmesh::SoilSamplerState& state) {
    if (state.status == atmosmesh::SoilAcquisitionStatus::Valid && state.measurement.valid) {
        char text[64];
        std::snprintf(text, sizeof(text), "water: ok adc_raw=%u samples=%u power=off",
                      static_cast<unsigned>(state.measurement.raw),
                      static_cast<unsigned>(atmosmesh::kSoilSampleCount));
        return text;
    }
    if (state.status == atmosmesh::SoilAcquisitionStatus::Timeout) {
        return "water: unavailable acquisition-timeout power=off";
    }
    if (state.status == atmosmesh::SoilAcquisitionStatus::Failed) {
        return "water: unavailable acquisition-failed power=off";
    }
    return "water: unavailable not-sampled power=off";
}

void apply_water_power_action(atmosmesh::SoilPowerAction action) {
    if (action != atmosmesh::SoilPowerAction::None) {
        write_pin_level(profile.water_power_control_gpio, atmosmesh::soil_power_pin_high(action));
    }
}

void setup_water_power_fail_safe() {
    // Set the latch HIGH before OUTPUT. The external 100k base-emitter pull-up is still mandatory
    // to hold the PNP switch off before firmware takes control (mirrors D-015 for Grove's soil
    // probe; see story AQ-01).
    digitalWrite(profile.water_power_control_gpio, HIGH);
    pinMode(profile.water_power_control_gpio, OUTPUT);
    apply_water_power_action(atmosmesh::soil_sampler_begin(water_state, millis()).power_action);
}

void publish_aqua_state() {
    atmosmesh::AquaMqttState state{};
    state.temperature_c = {readings.temperature.value, readings.temperature.valid, 0};
    state.humidity_pct = {readings.humidity.value, readings.humidity.valid, 0};
    state.water_adc_raw = {static_cast<float>(readings.water.raw), readings.water.valid, 0};
    atmosmesh::aqua_mqtt_runtime_publish_state(state);
}

void service_water_measurement(unsigned long now_ms) {
    auto step = atmosmesh::soil_sampler_tick(water_state, now_ms);
    apply_water_power_action(step.power_action);
    if (step.sample_adc) {
        step = atmosmesh::soil_sampler_record_sample(water_state, now_ms, analogRead(A0));
        apply_water_power_action(step.power_action);
    }
    if (!step.completed) {
        return;
    }
    readings.water = water_state.measurement;
    Serial.println(water_sampler_serial_text(water_state).c_str());
    render();
    publish_aqua_state();
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
    Serial.printf("oled: %s addr=0x%02X geometry=%dx%d\n", oled_ready ? "ok" : "error", address,
                  profile.oled_width_px, profile.oled_height_px);
}

bool read_sht41(float* temperature_c, float* humidity_pct) {
    Wire.beginTransmission(atmosmesh::kSht41I2cAddress);
    Wire.write(atmosmesh::kSht41MeasureHighPrecisionCmd);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    delay(atmosmesh::kSht41MeasureDelayMs);
    if (Wire.requestFrom(static_cast<std::uint8_t>(atmosmesh::kSht41I2cAddress),
                         static_cast<std::uint8_t>(atmosmesh::kSht41FrameBytes)) !=
        static_cast<int>(atmosmesh::kSht41FrameBytes)) {
        return false;
    }
    std::uint8_t bytes[atmosmesh::kSht41FrameBytes];
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(Wire.read());
    }
    const auto sample = atmosmesh::parse_sht41_frame(bytes);
    if (!sample.ok) {
        return false;
    }
    *temperature_c = sample.temperature_c;
    *humidity_pct = sample.humidity_pct;
    return true;
}

void sample_sensors() {
    sht41_ready = i2c_address_present(atmosmesh::kSht41I2cAddress);
    float temperature_c = 0.0F;
    float humidity_pct = 0.0F;
    const bool ok = sht41_ready && read_sht41(&temperature_c, &humidity_pct);
    readings.temperature = {ok, ok ? temperature_c : 0.0F};
    readings.humidity = {ok, ok ? humidity_pct : 0.0F};
    if (ok) {
        Serial.printf("sht41: ok temperature=%.1f C humidity=%.1f %%RH\n",
                      static_cast<double>(temperature_c), static_cast<double>(humidity_pct));
    } else {
        Serial.println("sht41: error reading unavailable");
    }
}

}  // namespace

void setup() {
    setup_water_power_fail_safe();
    Serial.begin(115200);
    delay(200);
    Serial.printf("product=%s product_id=%s variant=%s station_id=%s\n", profile.product_name,
                  profile.product_id, profile.product_variant, profile.station_id);
    Serial.printf("i2c: SDA=D2/GPIO%d SCL=D1/GPIO%d clock=%lu Hz (non-bootstrap pins)\n",
                  profile.i2c_sda_gpio, profile.i2c_scl_gpio,
                  static_cast<unsigned long>(kI2cClockHz));
    Serial.printf("sht41: addr=0x%02X on shared I2C bus (no calibration, Sensirion default)\n",
                  atmosmesh::kSht41I2cAddress);
    Serial.printf("water: control=D5/GPIO%d active=LOW interval=%lums settle=%lums samples=%u\n",
                  profile.water_power_control_gpio,
                  static_cast<unsigned long>(atmosmesh::kSoilSampleIntervalMs),
                  static_cast<unsigned long>(atmosmesh::kSoilSettleMs),
                  static_cast<unsigned>(atmosmesh::kSoilSampleCount));
    Serial.println("water: high-side 2N3906 PNP emitter=3V3 collector=probe-VCC "
                   "base=D5-via-2.2k pullup=100k-base-emitter");
    Serial.println("water: AOUT->A0 raw-only; probe identity/divider unconfirmed (AQ-01 OQ2)");
    Serial.println("water-warning: confirm board A0 divider before connecting probe AOUT (bare "
                   "module TOUT is 0-1.0V; NodeMCU-style A0 header is 0-3.3V)");
    Serial.println("power: OLED and SHT41 are 3V3 only");

    Wire.begin(profile.i2c_sda_gpio, profile.i2c_scl_gpio);
    Wire.setClock(kI2cClockHz);
    setup_oled();
    atmosmesh::aqua_mqtt_runtime_begin();
    render();
    last_sample_ms = millis();
}

void loop() {
    const unsigned long now = millis();
    service_water_measurement(now);
    if (!atmosmesh::soil_sampler_power_active(water_state) &&
        now - last_sample_ms >= kSampleIntervalMs) {
        last_sample_ms = now;
        sample_sensors();
        render();
        publish_aqua_state();
    }
    // Defer socket work while the water probe's power switch is active, mirroring Grove's rule
    // that the PNP switch cannot stay on through a DNS/TCP/MQTT reconnect attempt.
    if (atmosmesh::aqua_network_work_allowed(atmosmesh::soil_sampler_power_active(water_state))) {
        atmosmesh::aqua_mqtt_runtime_tick(now);
    }
    readings.mqtt_up = atmosmesh::aqua_mqtt_runtime_mqtt_up();
    yield();
}

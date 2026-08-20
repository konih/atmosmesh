#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_VEML7700.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "atmosmesh/am2302_frame.hpp"
#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/digital_edge.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/mq135_scale.hpp"
#include "atmosmesh/oled_address.hpp"
#include "atmosmesh/oled_profile.hpp"
#include "atmosmesh/pins.hpp"
#include "atmosmesh/sds011_frame.hpp"
#include "atmosmesh/veml7700_text.hpp"

namespace {

DHT am2302(atmosmesh::kAm2302DataGpio, DHT22);
Adafruit_BMP280 bmp280(&Wire1);
Adafruit_VEML7700 veml7700;
U8G2* oled = nullptr;
atmosmesh::OledProfile oled_profile{};
bool oled_ready = false;
int bmp_address = -1;
bool bmp_ok = false;
atmosmesh::Sds011Stream sds011{};
bool pm_ok = false;
float pm25_ug_m3 = 0.0F;
float pm10_ug_m3 = 0.0F;
unsigned long last_pm_ms = 0;
constexpr unsigned long kPmStaleMs = 5000;
constexpr unsigned long kLoopSliceMs = 10;
atmosmesh::DebouncedLevel pir_edge{};
bool pir_motion = false;
bool veml_ok = false;
float veml_lux = 0.0F;
bool am_ok = false;
float humidity = 0.0F;
float temperature = 0.0F;
atmosmesh::Am2302Hold am_hold{};
float bmp_t_c = 0.0F;
float bmp_p_hpa = 0.0F;
bool bmp_read_ok = false;
int mq_raw = 0;
unsigned long last_env_ms = 0;

void log_scan(const char* label, const atmosmesh::I2cBusMap& pins, const std::uint8_t* found,
              std::size_t count) {
    Serial.printf("i2c scan %s sda=%d scl=%d devices=%u\n", label, pins.sda_gpio, pins.scl_gpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        Serial.printf("  addr=0x%02X\n", found[i]);
    }
}

void release_oled() {
    delete oled;
    oled = nullptr;
}

U8G2* make_u8g2(const atmosmesh::OledProfile& profile) {
    const int scl = atmosmesh::kOledSclGpio;
    const int sda = atmosmesh::kOledSdaGpio;
    if (profile.controller == atmosmesh::OledController::Sh1106) {
        return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl, sda);
    }
    if (profile.height_px == atmosmesh::kOledHeightPxAlt) {
        return new U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl, sda);
    }
    // 128×48 uses 64-row full buffer then SET MUX 0x2F (U8g2 has no 128×48 ctor).
    return new U8G2_SSD1306_128X64_ALT0_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl, sda);
}

void show_lines(const std::string* lines, std::size_t count) {
    if (!oled_ready || oled == nullptr) {
        return;
    }
    oled->clearBuffer();
    oled->setDrawColor(1);
    oled->setFont(u8g2_font_6x10_tf);
    const int baseline_adjust = oled->getAscent();
    for (std::size_t i = 0; i < count; ++i) {
        int y = atmosmesh::oled_live_row_y_px(static_cast<int>(i)) + baseline_adjust;
        if (y > oled_profile.clip_max_y) {
            y = oled_profile.clip_max_y;
        }
        oled->drawStr(oled_profile.column_offset_px, y, lines[i].c_str());
    }
    oled->sendBuffer();
}

void pulse_beeper() {
    digitalWrite(atmosmesh::kBeeperGpio, HIGH);
    delay(atmosmesh::kBeeperPulseMs);
    digitalWrite(atmosmesh::kBeeperGpio, LOW);
}

void refresh_oled() {
    const auto lines = atmosmesh::live_sensor_lines(
        am_hold.show, am_hold.temperature_c, am_hold.humidity_rh, bmp_read_ok, bmp_p_hpa, pm_ok,
        pm25_ug_m3, pm10_ug_m3, mq_raw, pir_motion, veml_ok, veml_lux, bmp_t_c,
        atmosmesh::oled_right_cell_for_ms(millis()));
    show_lines(lines.data(), lines.size());
}

void setup_extras() {
    pinMode(atmosmesh::kBeeperGpio, OUTPUT);
    digitalWrite(atmosmesh::kBeeperGpio, LOW);
    pinMode(atmosmesh::kPirGpio, INPUT_PULLDOWN);
    Serial.printf("beeper: GPIO%d output, %d ms boot pulse\n", atmosmesh::kBeeperGpio,
                  atmosmesh::kBeeperPulseMs);
    Serial.printf("pir: GPIO%d digital INPUT_PULLDOWN\n", atmosmesh::kPirGpio);
    pulse_beeper();
    Serial.println(atmosmesh::format_beep_boot_log().c_str());
}

void poll_extras() {
    const unsigned long now = millis();
    const bool pir_sample = digitalRead(atmosmesh::kPirGpio) == HIGH;
    if (atmosmesh::update_debounced_level(pir_edge, pir_sample, now, atmosmesh::kDigitalDebounceMs)) {
        const bool rose = pir_edge.stable && !pir_motion;
        pir_motion = pir_edge.stable;
        Serial.println(atmosmesh::format_pir_log(pir_motion).c_str());
        if (rose) {
            pulse_beeper();
        }
        refresh_oled();
    }
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
    oled = make_u8g2(profile);
    oled->setI2CAddress(static_cast<uint8_t>(address << 1));
    oled->setBusClock(atmosmesh::kOledI2cHz);
    Serial.printf("oled: begin renderer=u8g2 constructor=%s addr=0x%02X\n",
                  atmosmesh::u8g2_hw_i2c_constructor_name(profile), address);
    if (!oled->begin()) {
        Serial.printf("oled: init error controller=%s addr=0x%02X height=%d\n",
                      atmosmesh::oled_controller_name(profile.controller), address,
                      profile.height_px);
        release_oled();
        return false;
    }
    oled_profile = profile;
    oled->setFlipMode(static_cast<uint8_t>(atmosmesh::compiled_oled_flip_mode()));
    Serial.println(atmosmesh::format_oled_flip_log().c_str());
    if (atmosmesh::oled_should_set_mux(profile)) {
        oled->sendF("ca", atmosmesh::kSsd1306SetMultiplex, atmosmesh::kSsd1306MuxRatio48);
        oled->sendF("ca", atmosmesh::kSsd1306SetDisplayOffset, atmosmesh::kSsd1306DisplayOffset0);
        oled->setClipWindow(0, 0, profile.width_px - 1, profile.clip_max_y);
        Serial.println(atmosmesh::format_oled_mux48_log().c_str());
    }
    Serial.println(atmosmesh::format_oled_init_log(profile, address).c_str());
    Serial.printf("oled: sda=%d scl=%d column_offset=%d vcc=3V3 (never 5V if pull-ups are on VCC)\n",
                  atmosmesh::kOledSdaGpio, atmosmesh::kOledSclGpio, profile.column_offset_px);
    return true;
}

void prove_oled_glass() {
    if (oled == nullptr) {
        return;
    }
    oled->setPowerSave(0);
    Serial.println(atmosmesh::format_oled_display_on_log().c_str());
    oled->setContrast(255);
    Serial.println(atmosmesh::format_oled_contrast_log().c_str());
    oled->sendF("c", 0x0a6);
    Serial.println(atmosmesh::format_oled_invert_off_log().c_str());

    oled->clearBuffer();
    oled->setDrawColor(1);
    for (int i = 0; i < atmosmesh::oled_boot_bar_count(); ++i) {
        oled->drawBox(0, atmosmesh::oled_boot_bar_y_px(i), atmosmesh::kOledWidthPx,
                      atmosmesh::oled_telltale_bar_height_px());
    }
    oled->sendBuffer();
    Serial.println(atmosmesh::format_oled_boot_bars_log().c_str());
    delay(atmosmesh::oled_boot_bar_hold_ms());
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
        atmosmesh::resolve_oled_profile(atmosmesh::OledController::Ssd1306, 64),
        atmosmesh::resolve_oled_profile(atmosmesh::OledController::Sh1106, 64),
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
    prove_oled_glass();
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
        bmp_address = -1;
        return;
    }

    // Adafruit_BMP280::begin() calls Wire1.begin() with no pins; restore GPIO21/19.
    if (!bmp280.begin(static_cast<std::uint8_t>(bmp_address), chip_id)) {
        Serial.printf("bmp280: begin failed at 0x%02X\n", bmp_address);
        bmp_address = -1;
        return;
    }
    Wire1.begin(atmosmesh::kSensorSdaGpio, atmosmesh::kSensorSclGpio);
    Wire1.setClock(atmosmesh::kOledI2cHz);
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2,
                       Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16,
                       Adafruit_BMP280::STANDBY_MS_500);
    bmp_ok = true;
}

void setup_veml7700() {
    Serial.printf("veml7700: Wire1 SDA=GPIO%d SCL=GPIO%d addr=0x%02X VCC=3V3 (shared with BMP280)\n",
                  atmosmesh::kSensorSdaGpio, atmosmesh::kSensorSclGpio,
                  atmosmesh::kVeml7700Address);
    // Adafruit_I2CDevice::begin() may call Wire1.begin() with no pins; restore 21/19.
    veml_ok = veml7700.begin(&Wire1);
    Wire1.begin(atmosmesh::kSensorSdaGpio, atmosmesh::kSensorSclGpio);
    Wire1.setClock(atmosmesh::kOledI2cHz);
    if (!veml_ok) {
        Serial.println(atmosmesh::format_veml7700_serial(false, 0.0F).c_str());
        return;
    }
    veml7700.setGain(VEML7700_GAIN_1);
    veml7700.setIntegrationTime(VEML7700_IT_100MS);
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
    if (!pm_ok) {
        Serial.println(atmosmesh::format_sds011_no_frame_log().c_str());
        return;
    }
    if ((millis() - last_pm_ms) > kPmStaleMs) {
        pm_ok = false;
        Serial.println(atmosmesh::format_sds011_no_frame_log().c_str());
    }
}

}  // namespace

void setup() {
    Serial.begin(atmosmesh::kSerialBaud);
    delay(200);
    Serial.println("atmosmesh bench bring-up");
    Serial.println("OLED VCC=3V3; BMP280 VCC=3V3 CSB=3V3 SDO=GND; AM2302 VDD=3V3 data=GPIO18");
    Serial.println(atmosmesh::format_sds011_listen_log().c_str());
    Serial.println("mq135: analog GPIO34 ADC1 atten~11dB. Divider 10k series + 20k to GND.");
    Serial.println("mq135: GPIO sees 2/3 of AOUT. 5V AOUT => 3.33V on GPIO34 (no headroom). Never apply 5V to GPIO.");

    analogReadResolution(12);
    analogSetPinAttenuation(atmosmesh::kMq135AdcGpio, ADC_11db);

    setup_oled();
    setup_bmp280();
    setup_veml7700();
    am2302.begin();
    last_env_ms = millis();
    Serial.printf("am2302: data=GPIO%d min_interval_ms=%d\n", atmosmesh::kAm2302DataGpio,
                  atmosmesh::kAm2302MinIntervalMs);
    Serial2.begin(atmosmesh::kSds011Baud, SERIAL_8N1, atmosmesh::kSds011RxGpio,
                  atmosmesh::kSds011TxGpio);
    Serial.printf("sds011: uart2 rx=GPIO%d tx=GPIO%d baud=%d\n", atmosmesh::kSds011RxGpio,
                  atmosmesh::kSds011TxGpio, atmosmesh::kSds011Baud);
    Serial.println(atmosmesh::format_sds011_listen_log().c_str());
    setup_extras();
}

void loop() {
    poll_sds011();
    poll_extras();
    const unsigned long now = millis();
    if ((now - last_env_ms) < static_cast<unsigned long>(atmosmesh::kAm2302MinIntervalMs)) {
        delay(kLoopSliceMs);
        return;
    }
    last_env_ms = now;
    // Adafruit DHT caches for MIN_INTERVAL (2 s): humidity does the bus read, temperature reuses it.
    humidity = am2302.readHumidity();
    temperature = am2302.readTemperature();
    am_ok = !isnan(humidity) && !isnan(temperature);
    atmosmesh::update_am2302_hold(am_hold, am_ok, temperature, humidity,
                                  atmosmesh::kAm2302HoldMisses);

    if (am_ok) {
        Serial.printf("am2302: t=%.1fC rh=%.1f%%\n", temperature, humidity);
    } else {
        Serial.println("am2302: read failed (need 3V3, 10k pull-up to 3V3 if the module has none)");
    }

    mq_raw = analogRead(atmosmesh::kMq135AdcGpio);
    Serial.println(atmosmesh::format_mq135_serial(mq_raw).c_str());

    bmp_t_c = 0.0F;
    bmp_p_hpa = 0.0F;
    bmp_read_ok = false;
    if (bmp_ok) {
        bmp_t_c = bmp280.readTemperature();
        bmp_p_hpa = bmp280.readPressure() / 100.0F;
        bmp_read_ok = !isnan(bmp_t_c) && (bmp_p_hpa > 300.0F) && (bmp_p_hpa < 1100.0F);
        if (bmp_read_ok) {
            Serial.println(atmosmesh::format_bmp280_serial(bmp_t_c, bmp_p_hpa).c_str());
        } else {
            Serial.println("bmp280: read failed");
        }
    }

    if (veml_ok) {
        veml_lux = veml7700.readLux();
        Serial.println(atmosmesh::format_veml7700_serial(true, veml_lux).c_str());
    }

    refresh_oled();
    delay(kLoopSliceMs);
}

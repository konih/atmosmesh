// AtmosMesh Spot product composition root — ESP32-C3 SuperMini with the 0.42 inch OLED (SP-01).
//
// Scope: SHT41 temperature/humidity and VEML7700 light on the OLED's own I2C bus, an HLK-LD2410S
// presence radar (OT2 pin plus its UART report on UART0), a DS18B20 probe on 1-Wire, and
// Wi-Fi/MQTT to Home Assistant as atmosmesh-spot-0001. One 3.3 V domain, USB powered.
//
// The 72 x 40 panel shows one value at a time and rotates pages every few seconds; the BOOT
// button forces the next page. A presence glyph sits in the top-right corner of every page and
// the link state in the bottom-right, so "is somebody here" and "is Home Assistant getting this"
// never need a serial cable.
//
// Every reading carries valid/age: a breakout pulled at runtime goes unavailable within one
// publish interval rather than repeating its last number.
#include <Adafruit_VEML7700.h>
#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "atmosmesh/esp32_mqtt_runtime.hpp"
#include "atmosmesh/i2c_bus.hpp"
#include "atmosmesh/ld2410s_frame.hpp"
#include "atmosmesh/mqtt_contract.hpp"
#include "atmosmesh/presence_hold.hpp"
#include "atmosmesh/sht4x_frame.hpp"
#include "atmosmesh/spot_pins.hpp"

namespace spot = atmosmesh::spot;

namespace {

constexpr const char* kProductId = "atmosmesh-spot-v1";
constexpr const char* kBoardId = "esp32-c3-supermini-oled";

const u8g2_cb_t* oled_rotation() {
    switch (spot::kOledRotation) {
        case 1: return U8G2_R1;
        case 2: return U8G2_R2;
        case 3: return U8G2_R3;
        default: return U8G2_R0;
    }
}

// ---------------------------------------------------------------- state

// No pin numbers here on purpose. Given pins, U8g2's GPIO init calls pinMode() on SDA and SCL,
// and on Arduino core 3.x that hands the pins back from the I2C peripheral to plain GPIO (the
// peripheral manager), after which the bus is gone and oled.begin() never returns. Observed on
// the first unit on 2026-09-05: both sensors answered before oled.begin() and nothing after.
// The bus is started by scan_i2c_bus() on the real pins; U8g2 then finds Wire already running.
U8G2_SSD1306_72X40_ER_F_HW_I2C oled(oled_rotation(), U8X8_PIN_NONE);
Adafruit_VEML7700 veml;
OneWire one_wire(spot::kOneWireGpio);
DallasTemperature probe_bus(&one_wire);
DeviceAddress probe_addr{};
atmosmesh::Ld2410sStream radar_stream;
atmosmesh::PresenceHold presence{};

bool bus_fault = false;
bool oled_present = false;
bool sht_present = false;
bool veml_present = false;
bool probe_present = false;

float sht_t_c = 0.0F;
float sht_rh = 0.0F;
float veml_lux = 0.0F;
float probe_c = 0.0F;
unsigned long sht_last_ok_ms = 0;
unsigned long veml_last_ok_ms = 0;
unsigned long probe_last_ok_ms = 0;

bool probe_conversion_pending = false;
bool probe_first_read = true;
int probe_misses = 0;
unsigned long probe_requested_ms = 0;
unsigned long probe_last_search_ms = 0;

bool radar_frame_seen = false;
bool radar_standard = false;
bool radar_ot2_ever_high = false;
std::uint8_t radar_state = 0;
std::uint16_t radar_distance_cm = 0;
unsigned long radar_last_frame_ms = 0;
unsigned long radar_frames = 0;
// Raw UART evidence, for the case "OT2 works but no frames": a byte count separates "nothing on
// the wire" (joint, wrong pin) from "bytes that are not the format expected" (decoder).
unsigned long radar_rx_bytes = 0;
std::uint8_t radar_first_bytes[16] = {};
std::size_t radar_first_count = 0;
bool radar_first_dumped = false;

int page = 0;
constexpr int kPageCount = 5;
unsigned long page_changed_ms = 0;
bool button_pressed = false;
unsigned long button_edge_ms = 0;
bool button_raw_last = false;
bool redraw_due = true;

unsigned long last_sample_ms = 0;
unsigned long last_publish_ms = 0;
unsigned long last_log_ms = 0;
unsigned long last_heartbeat_ms = 0;
bool heartbeat_on = false;
bool ever_published = false;
bool published_presence = false;
bool publish_now = false;

// ---------------------------------------------------------------- validity

bool fresh(unsigned long stamp_ms, unsigned long now) {
    return stamp_ms != 0UL && now - stamp_ms < spot::kReadingStaleMs;
}

bool sht_valid(unsigned long now) { return sht_present && fresh(sht_last_ok_ms, now); }
bool veml_valid(unsigned long now) { return veml_present && fresh(veml_last_ok_ms, now); }
bool probe_valid(unsigned long now) { return probe_present && fresh(probe_last_ok_ms, now); }
bool radar_frames_fresh(unsigned long now) {
    return radar_frame_seen && now - radar_last_frame_ms < spot::kRadarStaleMs;
}
// A radar is known to be attached when its UART talks or its OT2 pin has ever gone high. Until
// one of those happens, a low OT2 is "no radar", not "nobody", and presence stays unavailable.
bool radar_attached(unsigned long now) {
    return radar_frames_fresh(now) || radar_ot2_ever_high;
}
bool presence_valid(unsigned long now) {
    return now >= spot::kRadarWarmupMs && radar_attached(now);
}

// ---------------------------------------------------------------- sensors

bool i2c_present(std::uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

enum class ShtResult { Ok, NoAck, ShortRead, BadCrc };

std::uint8_t last_i2c_err = 0;

ShtResult read_sht41(float* temperature_c, float* humidity_pct) {
    Wire.beginTransmission(atmosmesh::kSht41I2cAddress);
    Wire.write(atmosmesh::kSht41MeasureHighPrecisionCmd);
    last_i2c_err = Wire.endTransmission();
    if (last_i2c_err != 0) {
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
        return ShtResult::BadCrc;
    }
    *temperature_c = sample.temperature_c;
    *humidity_pct = sample.humidity_pct;
    return ShtResult::Ok;
}

void sample_sht41(unsigned long now) {
    float t = 0.0F;
    float rh = 0.0F;
    const ShtResult r = read_sht41(&t, &rh);
    const bool was_present = sht_present;
    if (r == ShtResult::Ok) {
        sht_t_c = t;
        sht_rh = rh;
        sht_last_ok_ms = now;
        sht_present = true;
    } else if (r == ShtResult::NoAck) {
        sht_present = false;
    }
    if (was_present != sht_present) {
        Serial.printf("sht41: %s (0x%02X, wire err=%u)\n", sht_present ? "found" : "LOST - no ack",
                      spot::kSht41Address, static_cast<unsigned>(last_i2c_err));
        redraw_due = true;
    } else if (r == ShtResult::BadCrc || r == ShtResult::ShortRead) {
        Serial.printf("sht41: frame rejected (%s)\n", r == ShtResult::BadCrc ? "crc" : "short");
    }
}

void configure_veml() {
    veml.setGain(VEML7700_GAIN_1);
    veml.setIntegrationTime(VEML7700_IT_100MS);
}

void sample_veml(unsigned long now) {
    const bool was_present = veml_present;
    const bool acks = i2c_present(spot::kVeml7700Address);
    if (acks && !veml_present) {
        // Came back (or first seen): the driver has to re-run its init sequence.
        veml_present = veml.begin(&Wire);
        if (veml_present) {
            configure_veml();
        }
    } else if (!acks) {
        veml_present = false;
    }
    if (veml_present) {
        const float lux = veml.readLux(VEML_LUX_AUTO);
        if (!std::isnan(lux) && lux >= 0.0F && lux < 200000.0F) {
            veml_lux = lux;
            veml_last_ok_ms = now;
        }
    }
    if (was_present != veml_present) {
        Serial.printf("veml7700: %s (0x%02X)\n", veml_present ? "found" : "LOST - no ack",
                      spot::kVeml7700Address);
        redraw_due = true;
    }
}

void log_probe_rom() {
    Serial.print("ds18b20: probe rom=");
    for (int i = 0; i < 8; ++i) {
        Serial.printf("%02X", probe_addr[i]);
    }
    Serial.printf(" on gpio%d, 12-bit, powered mode\n", spot::kOneWireGpio);
}

// Non-blocking: ask for a conversion on one pass, read it on a later one. A missing probe is
// searched for again every few seconds, so plugging it in at runtime just works.
void service_probe(unsigned long now) {
    if (!probe_present) {
        if (probe_last_search_ms != 0UL &&
            now - probe_last_search_ms < spot::kProbeSearchIntervalMs) {
            return;
        }
        probe_last_search_ms = now;
        probe_bus.begin();
        if (probe_bus.getDeviceCount() > 0 && probe_bus.getAddress(probe_addr, 0) &&
            probe_bus.validAddress(probe_addr) && probe_addr[0] == 0x28) {
            probe_present = true;
            probe_first_read = true;
            probe_misses = 0;
            probe_conversion_pending = false;
            probe_requested_ms = 0;
            probe_bus.setResolution(probe_addr, 12);
            log_probe_rom();
            redraw_due = true;
        }
        return;
    }

    if (!probe_conversion_pending) {
        if (probe_requested_ms != 0UL && now - probe_requested_ms < spot::kSampleIntervalMs) {
            return;
        }
        probe_bus.requestTemperaturesByAddress(probe_addr);
        probe_requested_ms = now;
        probe_conversion_pending = true;
        return;
    }
    if (now - probe_requested_ms < spot::kProbeConversionMs) {
        return;
    }
    probe_conversion_pending = false;

    const float t = probe_bus.getTempC(probe_addr);
    // 85.0 C is the power-on value of a DS18B20 that has never converted; the first read after a
    // (re)discovery can return it if the request was lost, and it is not a measurement.
    const bool plausible = t != DEVICE_DISCONNECTED_C && t > -55.0F && t < 125.0F &&
                           !(probe_first_read && t == 85.0F);
    probe_first_read = false;
    if (plausible) {
        probe_c = t;
        probe_last_ok_ms = now;
        probe_misses = 0;
        return;
    }
    ++probe_misses;
    if (probe_misses >= 3) {
        Serial.println("ds18b20: probe LOST - no valid scratchpad three times; searching again");
        probe_present = false;
        probe_last_search_ms = now;
        redraw_due = true;
    }
}

void drain_radar_uart(unsigned long now) {
    while (Serial0.available() > 0) {
        const int byte = Serial0.read();
        if (byte < 0) {
            break;
        }
        ++radar_rx_bytes;
        if (radar_first_count < sizeof(radar_first_bytes)) {
            radar_first_bytes[radar_first_count++] = static_cast<std::uint8_t>(byte);
        } else if (!radar_first_dumped) {
            radar_first_dumped = true;
            Serial.print("radar: first bytes on RX:");
            for (std::size_t i = 0; i < radar_first_count; ++i) {
                Serial.printf(" %02X", radar_first_bytes[i]);
            }
            Serial.println();
        }
        const auto report = radar_stream.feed(static_cast<std::uint8_t>(byte));
        if (!report.ok) {
            continue;
        }
        radar_state = report.state;
        radar_distance_cm = report.distance_cm;
        radar_standard = report.standard;
        radar_last_frame_ms = now;
        ++radar_frames;
        if (!radar_frame_seen) {
            radar_frame_seen = true;
            Serial.printf("radar: first %s frame on gpio%d: state=%u distance=%ucm\n",
                          report.standard ? "standard" : "minimal", spot::kRadarRxGpio,
                          static_cast<unsigned>(report.state),
                          static_cast<unsigned>(report.distance_cm));
            redraw_due = true;
        }
    }
}

void poll_presence(unsigned long now) {
    const bool raw_high = digitalRead(spot::kRadarPresenceGpio) == HIGH;
    if (raw_high && !radar_ot2_ever_high) {
        radar_ot2_ever_high = true;
        Serial.printf("radar: OT2 (gpio%d) seen HIGH - a radar is attached\n",
                      spot::kRadarPresenceGpio);
    }
    if (atmosmesh::presence_hold_update(presence, raw_high, now, spot::kPresenceDebounceMs,
                                        spot::kPresenceHoldMs)) {
        Serial.printf("presence: %s ot2=%s uart_state=%u distance=%ucm%s uptime=%lus\n",
                      presence.occupied ? "OCCUPIED" : "clear", raw_high ? "HIGH" : "LOW",
                      static_cast<unsigned>(radar_state), static_cast<unsigned>(radar_distance_cm),
                      now < spot::kRadarWarmupMs ? " (warm-up, not trusted)" : "",
                      now / 1000UL);
        redraw_due = true;
        publish_now = true;
    }
}

// ---------------------------------------------------------------- button + heartbeat

void poll_button(unsigned long now) {
    const bool raw = digitalRead(spot::kBootButtonGpio) == LOW;   // active low, on-board pull-up
    if (raw != button_raw_last) {
        button_raw_last = raw;
        button_edge_ms = now;
        return;
    }
    if (now - button_edge_ms < spot::kButtonDebounceMs) {
        return;
    }
    if (raw && !button_pressed) {
        button_pressed = true;
        page = (page + 1) % kPageCount;
        page_changed_ms = now;
        redraw_due = true;
        Serial.printf("button: next page -> %d\n", page);
    } else if (!raw && button_pressed) {
        button_pressed = false;
    }
}

void service_heartbeat(unsigned long now) {
    if (now - last_heartbeat_ms < 1000UL) {
        return;
    }
    last_heartbeat_ms = now;
    heartbeat_on = !heartbeat_on;
    digitalWrite(spot::kStatusLedGpio, heartbeat_on ? HIGH : LOW);
}

// ---------------------------------------------------------------- display
//
// 72 x 40: a 7 px label row, one value in an 18 px face with a small unit after it, and two
// corner glyphs. Nothing else fits legibly, and nothing else is needed at arm's length.

void draw_corner_glyphs(unsigned long now) {
    const int w = oled.getDisplayWidth();
    const int h = oled.getDisplayHeight();
    // Presence, top-right: filled disc = somebody, ring = nobody, dot = radar not trusted yet.
    if (presence_valid(now)) {
        if (presence.occupied) {
            oled.drawDisc(w - 4, 3, 3);
        } else {
            oled.drawCircle(w - 4, 3, 3);
        }
    } else {
        oled.drawPixel(w - 4, 3);
    }
    // Link, bottom-right: filled box = MQTT up, frame = Wi-Fi only, nothing = no network.
    if (atmosmesh::esp32_mqtt_runtime_mqtt_up()) {
        oled.drawBox(w - 5, h - 5, 5, 5);
    } else if (atmosmesh::esp32_mqtt_runtime_wifi_up()) {
        oled.drawFrame(w - 5, h - 5, 5, 5);
    }
    // Page ticks, bottom-left.
    for (int i = 0; i < kPageCount; ++i) {
        if (i == page) {
            oled.drawBox(i * 4, h - 2, 3, 2);
        } else {
            oled.drawPixel(i * 4 + 1, h - 1);
        }
    }
}

void format_value(char* out, std::size_t cap, bool valid, float value, int decimals) {
    if (!valid) {
        std::snprintf(out, cap, "--");
    } else if (decimals == 0) {
        std::snprintf(out, cap, "%.0f", static_cast<double>(value));
    } else {
        std::snprintf(out, cap, "%.1f", static_cast<double>(value));
    }
}

void draw_page(unsigned long now) {
    const char* label = "";
    const char* unit = "";
    char big[12] = "";
    char sub[16] = "";

    switch (page) {
        case 0:
            label = "TEMP";
            unit = "C";
            format_value(big, sizeof(big), sht_valid(now), sht_t_c, 1);
            break;
        case 1:
            label = "HUMIDITY";
            unit = "%";
            format_value(big, sizeof(big), sht_valid(now), sht_rh, 0);
            break;
        case 2:
            label = "LIGHT";
            unit = "lx";
            format_value(big, sizeof(big), veml_valid(now), veml_lux, veml_lux < 100.0F ? 1 : 0);
            break;
        case 3:
            label = "PROBE";
            unit = "C";
            format_value(big, sizeof(big), probe_valid(now), probe_c, 1);
            break;
        default:
            label = "PRESENCE";
            if (!presence_valid(now)) {
                std::snprintf(big, sizeof(big), "--");
                std::snprintf(sub, sizeof(sub), now < spot::kRadarWarmupMs ? "warm-up" : "no radar");
            } else {
                std::snprintf(big, sizeof(big), presence.occupied ? "HERE" : "AWAY");
                if (radar_frames_fresh(now)) {
                    // Distance and the raw state byte from the UART report.
                    std::snprintf(sub, sizeof(sub), "%ucm  s%u",
                                  static_cast<unsigned>(radar_distance_cm),
                                  static_cast<unsigned>(radar_state));
                } else {
                    std::snprintf(sub, sizeof(sub), "OT2 only");
                }
            }
            break;
    }

    oled.clearBuffer();
    oled.setFont(u8g2_font_5x7_tf);
    oled.drawStr(0, 7, label);
    oled.setFont(u8g2_font_logisoso18_tr);
    oled.drawStr(0, 33, big);
    const int big_w = oled.getStrWidth(big);
    oled.setFont(u8g2_font_5x7_tf);
    if (unit[0] != '\0') {
        oled.drawStr(big_w + 3, 33, unit);
    }
    if (sub[0] != '\0') {
        oled.drawStr(0, 40, sub);
    }
    draw_corner_glyphs(now);
    oled.sendBuffer();
}

void splash(const std::uint8_t* found, std::size_t count) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(0, 10, "ATMOSMESH");
    oled.drawStr(0, 21, "SPOT 0001");
    char line[24];
    std::snprintf(line, sizeof(line), "i2c %u dev", static_cast<unsigned>(count));
    oled.setFont(u8g2_font_5x7_tf);
    oled.drawStr(0, 31, line);
    char addrs[24] = "";
    int used = 0;
    for (std::size_t i = 0; i < count && used < 18; ++i) {
        used += std::snprintf(addrs + used, sizeof(addrs) - used, "%02X ", found[i]);
    }
    oled.drawStr(0, 39, addrs);
    oled.sendBuffer();
}

// ---------------------------------------------------------------- home assistant

atmosmesh::MqttReading reading_of(bool valid, float value, unsigned long now,
                                  unsigned long stamp_ms) {
    atmosmesh::MqttReading r;
    r.valid = valid;
    r.value = value;
    r.age_ms = stamp_ms == 0UL ? 0UL : now - stamp_ms;
    return r;
}

void publish_state(unsigned long now) {
    if (!atmosmesh::esp32_mqtt_runtime_enabled()) {
        return;
    }
    const bool p_valid = presence_valid(now);
    const bool flipped = p_valid && presence.occupied != published_presence;
    if (ever_published && !flipped && !publish_now &&
        now - last_publish_ms < spot::kMqttPublishIntervalMs) {
        return;
    }
    publish_now = false;

    atmosmesh::SpotMqttState state;
    state.temperature_c = reading_of(sht_valid(now), sht_t_c, now, sht_last_ok_ms);
    state.humidity_pct = reading_of(sht_valid(now), sht_rh, now, sht_last_ok_ms);
    state.illuminance_lx = reading_of(veml_valid(now), veml_lux, now, veml_last_ok_ms);
    state.probe_temperature_c = reading_of(probe_valid(now), probe_c, now, probe_last_ok_ms);
    const bool uart_valid = radar_frames_fresh(now) && now >= spot::kRadarWarmupMs;
    state.presence_distance_cm = reading_of(uart_valid, static_cast<float>(radar_distance_cm), now,
                                            radar_last_frame_ms);
    state.presence_state = reading_of(uart_valid, static_cast<float>(radar_state), now,
                                      radar_last_frame_ms);
    const bool wifi = atmosmesh::esp32_mqtt_runtime_wifi_up();
    state.wifi_rssi_dbm = reading_of(
        wifi, static_cast<float>(atmosmesh::esp32_mqtt_runtime_rssi_dbm()), now, wifi ? now : 0UL);
    state.presence = {presence.occupied, p_valid,
                      presence.last_high_ms == 0UL ? 0UL : now - presence.last_high_ms};

    atmosmesh::esp32_mqtt_runtime_publish_payload(atmosmesh::spot_mqtt_state_json(state));
    last_publish_ms = now;
    published_presence = presence.occupied;
    ever_published = true;
}

// The slowest loop stage since the last log line goes into that line: a VEML7700 auto-ranging
// read in the dark takes ~700 ms, and that number is the ceiling on how late a radar edge or a
// button press can be noticed. If it ever grows, this says where.
unsigned long g_slowest_pass_ms = 0;
const char* g_slowest_stage = "";

void note_stage(const char* stage, unsigned long started) {
    const unsigned long took = millis() - started;
    if (took > g_slowest_pass_ms) {
        g_slowest_pass_ms = took;
        g_slowest_stage = stage;
    }
}

void log_state(unsigned long now) {
    if (now - last_log_ms < spot::kLogIntervalMs) {
        return;
    }
    last_log_ms = now;
    char t[12], rh[12], lux[12], probe[12];
    format_value(t, sizeof(t), sht_valid(now), sht_t_c, 1);
    format_value(rh, sizeof(rh), sht_valid(now), sht_rh, 0);
    format_value(lux, sizeof(lux), veml_valid(now), veml_lux, 0);
    format_value(probe, sizeof(probe), probe_valid(now), probe_c, 1);
    const bool wifi = atmosmesh::esp32_mqtt_runtime_wifi_up();
    char dist[12] = "--";
    if (radar_frames_fresh(now)) {
        std::snprintf(dist, sizeof(dist), "%ucm/s%u", static_cast<unsigned>(radar_distance_cm),
                      static_cast<unsigned>(radar_state));
    }
    Serial.printf("spot: t=%sC rh=%s%% lux=%s probe=%sC presence=%s radar=%s frames=%lu wifi=%s"
                  " rssi=%ddBm mqtt=%s slow=%s:%lums uptime=%lus\n",
                  t, rh, lux, probe,
                  presence_valid(now) ? (presence.occupied ? "occupied" : "clear") : "n/a", dist,
                  radar_frames, wifi ? "up" : "down", atmosmesh::esp32_mqtt_runtime_rssi_dbm(),
                  atmosmesh::esp32_mqtt_runtime_mqtt_up() ? "up" : "down", g_slowest_stage,
                  g_slowest_pass_ms, now / 1000UL);
    g_slowest_pass_ms = 0;
    if (radar_frames == 0) {
        Serial.printf("radar: no frame yet, %lu raw byte(s) on gpio%d\n", radar_rx_bytes,
                      spot::kRadarRxGpio);
    }
}

// Everything that must keep running while setup() waits.
void wait_servicing(unsigned long duration_ms) {
    const unsigned long started = millis();
    while (millis() - started < duration_ms) {
        const unsigned long now = millis();
        drain_radar_uart(now);
        poll_presence(now);
        poll_button(now);
        service_heartbeat(now);
        atmosmesh::esp32_mqtt_runtime_tick(now);
        delay(1);
    }
}

struct BusIdle {
    bool sda_high;
    bool scl_high;
    bool healthy() const { return sda_high && scl_high; }
};

// Read both lines as plain GPIOs before handing them to a driver: a bus held low can hang
// Wire.begin() in its recovery loop with no panic and no watchdog.
BusIdle probe_bus_idle() {
    pinMode(spot::kI2cSdaGpio, INPUT_PULLUP);
    pinMode(spot::kI2cSclGpio, INPUT_PULLUP);
    delay(5);
    return {digitalRead(spot::kI2cSdaGpio) == HIGH, digitalRead(spot::kI2cSclGpio) == HIGH};
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1500);   // native USB: give the host a moment to attach before the hello
    Serial.println();
    Serial.printf("spot: boot product=%s station=%s board=%s\n", kProductId,
                  atmosmesh::mqtt_spot_contract().station_id, kBoardId);
    Serial.printf("spot: chip=%s rev%d flash=%uMB mac=%012llX\n", ESP.getChipModel(),
                  ESP.getChipRevision(), ESP.getFlashChipSize() / (1024U * 1024U),
                  ESP.getEfuseMac());

    pinMode(spot::kStatusLedGpio, OUTPUT);
    // A disconnected radar must read "clear", never float into phantom presence.
    pinMode(spot::kRadarPresenceGpio, INPUT_PULLDOWN);
    pinMode(spot::kBootButtonGpio, INPUT_PULLUP);
    Serial.printf("radar: OT2=gpio%d warmup=%lus hold=%lums debounce=%lums\n",
                  spot::kRadarPresenceGpio, spot::kRadarWarmupMs / 1000UL, spot::kPresenceHoldMs,
                  spot::kPresenceDebounceMs);

    // A UART transmitter idles HIGH. Read both lines against a pull-down before the driver takes
    // them: the radar's OT1 should hold the RX pin HIGH. RX LOW is an open joint or the wrong
    // pin -- the one wiring fault OT2 cannot reveal. RX LOW with TX HIGH is OT1 landed on the TX
    // pin: the UART is then run crossed for this boot so the radar still reports, and the pin
    // that carries the radar's push-pull output is never driven as an output against it.
    // First unit, 2026-09-05: RX idled LOW with OT2 working, which is how this got written.
    pinMode(spot::kRadarRxGpio, INPUT_PULLDOWN);
    pinMode(spot::kRadarTxGpio, INPUT_PULLDOWN);
    delay(2);
    const bool rx_idle_high = digitalRead(spot::kRadarRxGpio) == HIGH;
    const bool tx_idle_high = digitalRead(spot::kRadarTxGpio) == HIGH;
    int uart_rx = spot::kRadarRxGpio;
    int uart_tx = spot::kRadarTxGpio;
    Serial.printf("radar: OT1 line (gpio%d) idles %s; gpio%d idles %s\n", spot::kRadarRxGpio,
                  rx_idle_high ? "HIGH - a transmitter is attached" : "LOW - nothing driving it",
                  spot::kRadarTxGpio, tx_idle_high ? "HIGH" : "LOW");
    if (!rx_idle_high && tx_idle_high) {
        uart_rx = spot::kRadarTxGpio;
        uart_tx = spot::kRadarRxGpio;
        Serial.printf("radar: WIRING: OT1 appears to be on the TX pin (gpio%d) - running the UART "
                      "crossed for this boot; move OT1 to the RX pin and the radar RX to TX\n",
                      spot::kRadarTxGpio);
    }
    Serial0.setRxBufferSize(512);
    Serial0.begin(spot::kRadarBaud, SERIAL_8N1, uart_rx, uart_tx);
    Serial.printf("radar: uart0 rx=gpio%d(<-OT1) tx=gpio%d(->RX) %lu 8N1, listening for 6E..62\n",
                  spot::kRadarRxGpio, spot::kRadarTxGpio, spot::kRadarBaud);

    probe_bus.begin();
    probe_bus.setWaitForConversion(false);
    service_probe(millis());
    if (!probe_present) {
        Serial.printf("ds18b20: no probe on gpio%d (searched again every %lus)\n",
                      spot::kOneWireGpio, spot::kProbeSearchIntervalMs / 1000UL);
    }

    // Async: association overlaps the I2C scan and the splash.
    atmosmesh::Esp32MqttRuntimeConfig net;
    net.contract = &atmosmesh::mqtt_spot_contract();
    net.limit_tx_power = true;
    atmosmesh::esp32_mqtt_runtime_begin(net);

    const BusIdle idle = probe_bus_idle();
    Serial.printf("i2c: idle sda(gpio%d)=%s scl(gpio%d)=%s\n", spot::kI2cSdaGpio,
                  idle.sda_high ? "HIGH" : "LOW", spot::kI2cSclGpio, idle.scl_high ? "HIGH" : "LOW");
    if (!idle.healthy()) {
        Serial.println("i2c: FAULT a line is held low - refusing to start the drivers");
        Serial.println("i2c: check for a short to GND, a swapped supply pin, or a breakout in reset");
        bus_fault = true;
        return;
    }

    std::uint8_t found[16];
    const std::size_t count =
        atmosmesh::scan_i2c_bus(Wire, {spot::kI2cSdaGpio, spot::kI2cSclGpio}, found, sizeof(found));
    Serial.printf("i2c: scan sda=%d scl=%d found=%u", spot::kI2cSdaGpio, spot::kI2cSclGpio,
                  static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count && i < sizeof(found); ++i) {
        Serial.printf(" 0x%02X", found[i]);
    }
    Serial.println();

    oled_present = false;
    for (std::size_t i = 0; i < count && i < sizeof(found); ++i) {
        if (found[i] == spot::kOledAddress) {
            oled_present = true;
        }
    }
    // Wire is up on the real pins from the scan; U8g2 (constructed without pins) joins it.
    unsigned long t0 = millis();
    oled.setBusClock(spot::kI2cClockHz);
    oled.begin();
    oled.setContrast(160);
    Serial.printf("oled: ssd1306 72x40 at 0x%02X %s rotation=%d init took %lums\n",
                  spot::kOledAddress, oled_present ? "present" : "NOT on the bus",
                  spot::kOledRotation, millis() - t0);
    Wire.setClock(spot::kI2cClockHz);

    sample_sht41(millis());
    sample_veml(millis());
    Serial.printf("sht41:    address=0x%02X present=%d (wire err=%u)\n", spot::kSht41Address,
                  sht_present ? 1 : 0, static_cast<unsigned>(last_i2c_err));
    Serial.printf("veml7700: address=0x%02X present=%d\n", spot::kVeml7700Address,
                  veml_present ? 1 : 0);

    splash(found, count);
    wait_servicing(spot::kBootSplashHoldMs);
    Serial.println("spot: setup done, entering loop");
    page_changed_ms = millis();
    redraw_due = true;
}


void loop() {
    const unsigned long now = millis();

    drain_radar_uart(now);
    poll_presence(now);
    poll_button(now);
    service_heartbeat(now);
    atmosmesh::esp32_mqtt_runtime_tick(now);

    if (bus_fault) {
        // Sensors and display are off the table, but presence and the probe still work and the
        // log still says why the bus is down.
        service_probe(now);
        publish_state(now);
        log_state(now);
        delay(5);
        return;
    }

    unsigned long t0 = millis();
    service_probe(now);
    note_stage("probe", t0);

    if (now - last_sample_ms >= spot::kSampleIntervalMs) {
        last_sample_ms = now;
        t0 = millis();
        sample_sht41(now);
        note_stage("sht41", t0);
        drain_radar_uart(now);
        t0 = millis();
        sample_veml(now);
        note_stage("veml", t0);
        drain_radar_uart(now);
        redraw_due = true;
    }

    if (now - page_changed_ms >= spot::kPageDwellMs) {
        page = (page + 1) % kPageCount;
        page_changed_ms = now;
        redraw_due = true;
    }

    if (redraw_due) {
        redraw_due = false;
        t0 = millis();
        draw_page(now);
        note_stage("oled", t0);
    }

    t0 = millis();
    publish_state(now);
    note_stage("publish", t0);
    log_state(now);
    delay(2);
}

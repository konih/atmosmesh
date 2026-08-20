#include "atmosmesh/mqtt_contract.hpp"

#include <cstdio>
#include <string>

namespace atmosmesh {
namespace {

struct DiscoverySpec {
    const char* component;
    const char* object_id;
    const char* name;
    const char* device_class;  // nullable
    const char* unit;          // nullable for binary_sensor
    const char* value_template;
};

// value_template returns HA `none` when valid is false so entities go unavailable,
// not zero.
constexpr DiscoverySpec kDiscoverySpecs[] = {
    {"sensor", "temperature", "Temperature", "temperature", "\u00B0C",
     "{{ value_json.temperature.value if value_json.temperature.valid else none }}"},
    {"sensor", "humidity", "Humidity", "humidity", "%",
     "{{ value_json.humidity.value if value_json.humidity.valid else none }}"},
    {"sensor", "pressure", "Pressure", "pressure", "hPa",
     "{{ value_json.pressure.value if value_json.pressure.valid else none }}"},
    {"sensor", "bmp_temperature", "BMP Temperature", "temperature", "\u00B0C",
     "{{ value_json.bmp_temperature.value if value_json.bmp_temperature.valid else none }}"},
    {"sensor", "pm25", "PM2.5", "pm25", "\u00B5g/m\u00B3",
     "{{ value_json.pm25.value if value_json.pm25.valid else none }}"},
    {"sensor", "pm10", "PM10", "pm10", "\u00B5g/m\u00B3",
     "{{ value_json.pm10.value if value_json.pm10.valid else none }}"},
    {"sensor", "gas_index", "Gas Index", nullptr, "index",
     "{{ value_json.gas_index.value if value_json.gas_index.valid else none }}"},
    {"binary_sensor", "motion", "Motion", "occupancy", nullptr,
     "{{ 'ON' if value_json.motion.valid and value_json.motion.value else "
     "('OFF' if value_json.motion.valid else none) }}"},
};

void append_reading(std::string& out, const char* key, const MqttReading& reading,
                    const char* unit, bool with_comma) {
    if (with_comma) {
        out += ',';
    }
    out += '"';
    out += key;
    out += "\":{";
    if (reading.valid) {
        char value_buf[32];
        std::snprintf(value_buf, sizeof(value_buf), "%.1f", static_cast<double>(reading.value));
        out += "\"value\":";
        out += value_buf;
        out += ',';
    }
    out += "\"unit\":\"";
    out += unit;
    out += "\",\"valid\":";
    out += reading.valid ? "true" : "false";
    out += ",\"age_ms\":";
    char age_buf[32];
    std::snprintf(age_buf, sizeof(age_buf), "%lu", static_cast<unsigned long>(reading.age_ms));
    out += age_buf;
    out += '}';
}

void append_bool_reading(std::string& out, const char* key, const MqttBoolReading& reading,
                         bool with_comma) {
    if (with_comma) {
        out += ',';
    }
    out += '"';
    out += key;
    out += "\":{";
    if (reading.valid) {
        out += "\"value\":";
        out += reading.value ? "true" : "false";
        out += ',';
    }
    out += "\"valid\":";
    out += reading.valid ? "true" : "false";
    out += ",\"age_ms\":";
    char age_buf[32];
    std::snprintf(age_buf, sizeof(age_buf), "%lu", static_cast<unsigned long>(reading.age_ms));
    out += age_buf;
    out += '}';
}

std::string json_escape(const char* text) {
    std::string out;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '"' || *p == '\\') {
            out += '\\';
        }
        out += *p;
    }
    return out;
}

}  // namespace

std::string mqtt_state_json(const MqttStationState& state) {
    std::string out = "{\"id\":\"atmosmesh-0001\",\"device\":\"atmosmesh-v1\"";
    append_reading(out, "temperature", state.temperature, "C", true);
    append_reading(out, "humidity", state.humidity, "%", true);
    append_reading(out, "pressure", state.pressure, "hPa", true);
    append_reading(out, "bmp_temperature", state.bmp_temperature, "C", true);
    append_reading(out, "pm25", state.pm25, "ug/m3", true);
    append_reading(out, "pm10", state.pm10, "ug/m3", true);
    append_reading(out, "gas_index", state.gas_index, "index", true);
    if (state.mq135_raw_valid) {
        out += ",\"mq135_raw\":";
        char raw_buf[16];
        std::snprintf(raw_buf, sizeof(raw_buf), "%d", state.mq135_raw);
        out += raw_buf;
    }
    append_bool_reading(out, "motion", state.motion, true);
    out += '}';
    return out;
}

std::string mqtt_discovery_device_json() {
    return "{\"identifiers\":[\"atmosmesh-v1\",\"atmosmesh-0001\"],"
           "\"name\":\"AtmosMesh 0001\",\"model\":\"atmosmesh-v1\","
           "\"manufacturer\":\"AtmosMesh\"}";
}

std::size_t mqtt_discovery_config_count() {
    return sizeof(kDiscoverySpecs) / sizeof(kDiscoverySpecs[0]);
}

MqttDiscoveryConfig mqtt_discovery_config_at(std::size_t index) {
    MqttDiscoveryConfig cfg{};
    if (index >= mqtt_discovery_config_count()) {
        cfg.component = "";
        cfg.object_id = "";
        return cfg;
    }
    const DiscoverySpec& spec = kDiscoverySpecs[index];
    cfg.component = spec.component;
    cfg.object_id = spec.object_id;
    cfg.topic = std::string(kMqttDiscoveryPrefix) + "/" + spec.component + "/" +
                kMqttDiscoveryNode + "/" + spec.object_id + "/config";

    std::string payload = "{";
    payload += "\"name\":\"";
    payload += spec.name;
    payload += "\",\"state_topic\":\"";
    payload += kMqttStateTopic;
    payload += "\",\"availability_topic\":\"";
    payload += kMqttAvailabilityTopic;
    payload += "\",\"unique_id\":\"atmosmesh-0001_";
    payload += spec.object_id;
    payload += "\",\"expire_after\":90";
    payload += ",\"value_template\":\"";
    payload += json_escape(spec.value_template);
    payload += "\"";
    if (spec.device_class != nullptr) {
        payload += ",\"device_class\":\"";
        payload += spec.device_class;
        payload += "\"";
    }
    if (spec.unit != nullptr) {
        payload += ",\"unit_of_measurement\":\"";
        payload += spec.unit;
        payload += "\"";
    }
    payload += ",\"device\":";
    payload += mqtt_discovery_device_json();
    payload += '}';
    cfg.payload = std::move(payload);
    return cfg;
}

bool mqtt_payload_mentions_forbidden_room(std::string_view text) {
    return text.find("wohnzimmer") != std::string_view::npos ||
           text.find("office") != std::string_view::npos;
}

bool mqtt_payload_mentions_forbidden_gas_label(std::string_view text) {
    return text.find("co2") != std::string_view::npos ||
           text.find("CO2") != std::string_view::npos ||
           text.find("ppm") != std::string_view::npos;
}

}  // namespace atmosmesh

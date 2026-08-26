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
constexpr DiscoverySpec kV1DiscoverySpecs[] = {
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

constexpr DiscoverySpec kGroveDiscoverySpecs[] = {
    {"sensor", "temperature_c", "Temperature", "temperature", "\u00B0C",
     "{{ value_json.temperature_c | default(none) }}"},
    {"sensor", "humidity_pct", "Humidity", "humidity", "%",
     "{{ value_json.humidity_pct | default(none) }}"},
    {"sensor", "pressure_hpa", "Pressure", "pressure", "hPa",
     "{{ value_json.pressure_hpa | default(none) }}"},
    {"sensor", "light_charge_us", "Uncalibrated RC Light Charge Time", nullptr, "µs",
     "{{ value_json.light_charge_us | default(none) }}"},
    {"sensor", "soil_adc_raw", "Soil Probe ADC Raw", nullptr, "ADC count",
     "{{ value_json.soil_adc_raw | default(none) }}"},
};

// No pressure entity: SHT41 does not measure pressure (D-019/ADR-0002, story AQ-01).
constexpr DiscoverySpec kAquaDiscoverySpecs[] = {
    {"sensor", "temperature_c", "Temperature", "temperature", "\u00B0C",
     "{{ value_json.temperature_c | default(none) }}"},
    {"sensor", "humidity_pct", "Humidity", "humidity", "%",
     "{{ value_json.humidity_pct | default(none) }}"},
    {"sensor", "water_adc_raw", "Water Probe ADC Raw", nullptr, "ADC count",
     "{{ value_json.water_adc_raw | default(none) }}"},
};

constexpr MqttProductContract kV1Contract{
    MqttProductKind::AtmosMeshV1,
    kMqttDeviceId,
    kMqttStationId,
    kMqttDiscoveryNode,
    "AtmosMesh 0001",
    kMqttStateTopic,
    kMqttAvailabilityTopic,
};

constexpr MqttProductContract kGroveContract{
    MqttProductKind::AtmosMeshGroveV1_5,
    "atmosmesh-grove-v1.5",
    "atmosmesh-grove-0001",
    "atmosmesh_grove_0001",
    "AtmosMesh Grove 0001",
    "home/air/atmosmesh-grove-0001/state",
    "home/air/atmosmesh-grove-0001/availability",
};

constexpr MqttProductContract kAquaContract{
    MqttProductKind::AtmosMeshAquaV1,
    "atmosmesh-aqua-v1",
    "atmosmesh-aqua-0001",
    "atmosmesh_aqua_0001",
    "AtmosMesh Aqua 0001",
    "home/air/atmosmesh-aqua-0001/state",
    "home/air/atmosmesh-aqua-0001/availability",
};

struct DiscoverySpecRange {
    const DiscoverySpec* specs;
    std::size_t count;
};

DiscoverySpecRange discovery_specs(const MqttProductContract& contract) {
    if (contract.kind == MqttProductKind::AtmosMeshGroveV1_5) {
        return {kGroveDiscoverySpecs,
                sizeof(kGroveDiscoverySpecs) / sizeof(kGroveDiscoverySpecs[0])};
    }
    if (contract.kind == MqttProductKind::AtmosMeshAquaV1) {
        return {kAquaDiscoverySpecs, sizeof(kAquaDiscoverySpecs) / sizeof(kAquaDiscoverySpecs[0])};
    }
    return {kV1DiscoverySpecs, sizeof(kV1DiscoverySpecs) / sizeof(kV1DiscoverySpecs[0])};
}

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

void append_optional_number(std::string& out, const char* key, const MqttReading& reading,
                            const char* format) {
    if (!reading.valid) {
        return;
    }
    char value_buf[32];
    std::snprintf(value_buf, sizeof(value_buf), format, static_cast<double>(reading.value));
    out += ",\"";
    out += key;
    out += "\":";
    out += value_buf;
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

std::string grove_mqtt_state_json(const GroveMqttState& state) {
    std::string out = "{\"station_id\":\"atmosmesh-grove-0001\",";
    out += "\"product_id\":\"atmosmesh-grove-v1.5\"";
    append_optional_number(out, "temperature_c", state.temperature_c, "%.1f");
    append_optional_number(out, "humidity_pct", state.humidity_pct, "%.1f");
    append_optional_number(out, "pressure_hpa", state.pressure_hpa, "%.1f");
    append_optional_number(out, "light_charge_us", state.light_charge_us, "%.0f");
    append_optional_number(out, "soil_adc_raw", state.soil_adc_raw, "%.0f");
    out += '}';
    return out;
}

std::string aqua_mqtt_state_json(const AquaMqttState& state) {
    std::string out = "{\"station_id\":\"atmosmesh-aqua-0001\",";
    out += "\"product_id\":\"atmosmesh-aqua-v1\"";
    append_optional_number(out, "temperature_c", state.temperature_c, "%.1f");
    append_optional_number(out, "humidity_pct", state.humidity_pct, "%.1f");
    append_optional_number(out, "water_adc_raw", state.water_adc_raw, "%.0f");
    out += '}';
    return out;
}

const MqttProductContract& mqtt_v1_contract() {
    return kV1Contract;
}

const MqttProductContract& mqtt_grove_contract() {
    return kGroveContract;
}

const MqttProductContract& mqtt_aqua_contract() {
    return kAquaContract;
}

MqttWillConfig mqtt_will_config(const MqttProductContract& contract) {
    return {contract.availability_topic, kMqttAvailabilityOffline, 0, true};
}

std::string mqtt_discovery_device_json() {
    return mqtt_discovery_device_json(kV1Contract);
}

std::string mqtt_discovery_device_json(const MqttProductContract& contract) {
    std::string out = "{\"identifiers\":[\"";
    out += contract.product_id;
    out += "\",\"";
    out += contract.station_id;
    out += "\"],\"name\":\"";
    out += contract.display_name;
    out += "\",\"model\":\"";
    out += contract.product_id;
    out += "\",\"manufacturer\":\"AtmosMesh\"}";
    return out;
}

std::size_t mqtt_discovery_config_count() {
    return mqtt_discovery_config_count(kV1Contract);
}

std::size_t mqtt_discovery_config_count(const MqttProductContract& contract) {
    return discovery_specs(contract).count;
}

MqttDiscoveryConfig mqtt_discovery_config_at(std::size_t index) {
    return mqtt_discovery_config_at(kV1Contract, index);
}

MqttDiscoveryConfig mqtt_discovery_config_at(const MqttProductContract& contract,
                                             std::size_t index) {
    MqttDiscoveryConfig cfg{};
    const auto range = discovery_specs(contract);
    if (index >= range.count) {
        cfg.component = "";
        cfg.object_id = "";
        return cfg;
    }
    const DiscoverySpec& spec = range.specs[index];
    cfg.component = spec.component;
    cfg.object_id = spec.object_id;
    cfg.topic = std::string(kMqttDiscoveryPrefix) + "/" + spec.component + "/" +
                contract.discovery_node + "/" + spec.object_id + "/config";

    std::string payload = "{";
    payload += "\"name\":\"";
    payload += spec.name;
    payload += "\",\"state_topic\":\"";
    payload += contract.state_topic;
    payload += "\",\"availability_topic\":\"";
    payload += contract.availability_topic;
    payload += "\",\"unique_id\":\"";
    payload += contract.station_id;
    payload += '_';
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
    payload += mqtt_discovery_device_json(contract);
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

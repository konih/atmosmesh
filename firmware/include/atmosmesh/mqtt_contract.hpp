#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace atmosmesh {

inline constexpr const char* kMqttDeviceId = "atmosmesh-v1";
inline constexpr const char* kMqttStationId = "atmosmesh-0001";
inline constexpr const char* kMqttDiscoveryNode = "atmosmesh_0001";
inline constexpr const char* kMqttDiscoveryPrefix = "homeassistant";
inline constexpr const char* kMqttStateTopic = "home/air/atmosmesh-0001/state";
inline constexpr const char* kMqttAvailabilityTopic = "home/air/atmosmesh-0001/availability";
inline constexpr const char* kMqttAvailabilityOnline = "online";
inline constexpr const char* kMqttAvailabilityOffline = "offline";
inline constexpr int kMqttKeepaliveSec = 15;
inline constexpr int kMqttExpireAfterSec = 90;

struct MqttReading {
    float value = 0.0F;
    bool valid = false;
    unsigned long age_ms = 0;
};

struct MqttBoolReading {
    bool value = false;
    bool valid = false;
    unsigned long age_ms = 0;
};

struct MqttStationState {
    MqttReading temperature;
    MqttReading humidity;
    MqttReading pressure;
    MqttReading bmp_temperature;
    MqttReading pm25;
    MqttReading pm10;
    MqttReading gas_index;
    int mq135_raw = -1;
    bool mq135_raw_valid = false;
    MqttBoolReading motion;
};

struct GroveMqttState {
    MqttReading temperature_c;
    MqttReading humidity_pct;
    MqttReading pressure_hpa;
    MqttReading light_charge_us;
    MqttReading soil_adc_raw;
};

// No pressure entity: SHT41 does not measure pressure. See D-019/ADR-0002 and story AQ-01.
struct AquaMqttState {
    MqttReading temperature_c;
    MqttReading humidity_pct;
    MqttReading water_adc_raw;
};

// AtmosMesh Room. Every reading carries valid/age_ms rather than a bare number: an SDS011 that
// has stopped talking must reach Home Assistant as "unavailable", never as clean air, and a PIR
// still inside its warm-up window must not publish "no motion" as though it were a measurement.
struct RoomMqttState {
    MqttReading temperature_c;
    MqttReading humidity_pct;
    MqttReading illuminance_lx;
    MqttReading pm25;
    MqttReading pm10;
    MqttBoolReading motion;
    // The same latch that drives the beeper, so an automation can react to what the room heard.
    MqttBoolReading pm_alarm;
};

enum class MqttProductKind {
    AtmosMeshV1,
    AtmosMeshGroveV1_5,
    AtmosMeshAquaV1,
    AtmosMeshRoomV1,
};

struct MqttProductContract {
    MqttProductKind kind;
    const char* product_id;
    const char* station_id;
    const char* discovery_node;
    const char* display_name;
    const char* state_topic;
    const char* availability_topic;
};

struct MqttDiscoveryConfig {
    const char* component;  // "sensor" or "binary_sensor"
    const char* object_id;
    std::string topic;
    std::string payload;
};

struct MqttWillConfig {
    const char* topic;
    const char* payload;
    int qos;
    bool retained;
};

std::string mqtt_state_json(const MqttStationState& state);
std::string grove_mqtt_state_json(const GroveMqttState& state);
std::string aqua_mqtt_state_json(const AquaMqttState& state);
std::string room_mqtt_state_json(const RoomMqttState& state);
const MqttProductContract& mqtt_v1_contract();
const MqttProductContract& mqtt_grove_contract();
const MqttProductContract& mqtt_aqua_contract();
const MqttProductContract& mqtt_room_contract();
MqttWillConfig mqtt_will_config(const MqttProductContract& contract);
std::string mqtt_discovery_device_json();
std::string mqtt_discovery_device_json(const MqttProductContract& contract);
std::size_t mqtt_discovery_config_count();
std::size_t mqtt_discovery_config_count(const MqttProductContract& contract);
MqttDiscoveryConfig mqtt_discovery_config_at(std::size_t index);
MqttDiscoveryConfig mqtt_discovery_config_at(const MqttProductContract& contract,
                                             std::size_t index);

bool mqtt_payload_mentions_forbidden_room(std::string_view text);
bool mqtt_payload_mentions_forbidden_gas_label(std::string_view text);

}  // namespace atmosmesh

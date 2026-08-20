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

struct MqttDiscoveryConfig {
    const char* component;  // "sensor" or "binary_sensor"
    const char* object_id;
    std::string topic;
    std::string payload;
};

std::string mqtt_state_json(const MqttStationState& state);
std::string mqtt_discovery_device_json();
std::size_t mqtt_discovery_config_count();
MqttDiscoveryConfig mqtt_discovery_config_at(std::size_t index);

bool mqtt_payload_mentions_forbidden_room(std::string_view text);
bool mqtt_payload_mentions_forbidden_gas_label(std::string_view text);

}  // namespace atmosmesh

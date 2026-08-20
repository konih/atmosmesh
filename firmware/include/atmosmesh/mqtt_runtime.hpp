#pragma once

#include "atmosmesh/mqtt_contract.hpp"
#include "atmosmesh/mqtt_session.hpp"

namespace atmosmesh {

// ESP32-only Wi-Fi + MQTT client. No-op stubs are not provided on native builds:
// this translation unit is excluded from the native env via platformio.ini.

bool mqtt_runtime_begin();
void mqtt_runtime_tick(unsigned long now_ms);
void mqtt_runtime_publish_state(const MqttStationState& state);

bool mqtt_runtime_enabled();
bool mqtt_runtime_wifi_up();
bool mqtt_runtime_mqtt_up();

}  // namespace atmosmesh

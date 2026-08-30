#pragma once

#include "atmosmesh/mqtt_contract.hpp"
#include "atmosmesh/mqtt_session.hpp"

namespace atmosmesh {

// ESP32-only Wi-Fi + MQTT client for the AtmosMesh Room product. No native stubs: this
// translation unit is excluded from the native env via platformio.ini.
//
// Separate from mqtt_runtime.cpp rather than a parameter on it, matching how grove and aqua each
// own their client. The v1 runtime hardcodes the v1 availability topic into the MQTT last-will,
// which is set once at client construction and cannot be re-pointed per call.

bool room_mqtt_runtime_begin();
void room_mqtt_runtime_tick(unsigned long now_ms);
void room_mqtt_runtime_publish_state(const RoomMqttState& state);

bool room_mqtt_runtime_enabled();
bool room_mqtt_runtime_wifi_up();
bool room_mqtt_runtime_mqtt_up();

}  // namespace atmosmesh

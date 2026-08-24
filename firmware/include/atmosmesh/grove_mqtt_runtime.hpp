#pragma once

#include "atmosmesh/mqtt_contract.hpp"

namespace atmosmesh {

// ESP8266-only transport. The shared contract/session remains host-testable; this translation unit
// is selected only by the Grove PlatformIO environment.
bool grove_mqtt_runtime_begin();
void grove_mqtt_runtime_tick(unsigned long now_ms);
void grove_mqtt_runtime_publish_state(const GroveMqttState& state);
bool grove_mqtt_runtime_enabled();
bool grove_mqtt_runtime_wifi_up();
bool grove_mqtt_runtime_mqtt_up();

}  // namespace atmosmesh

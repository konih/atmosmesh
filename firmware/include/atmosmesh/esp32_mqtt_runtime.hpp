#pragma once

#include <string>

#include "atmosmesh/mqtt_contract.hpp"
#include "atmosmesh/mqtt_session.hpp"

namespace atmosmesh {

// ESP32-family Wi-Fi + esp-mqtt client bound to a product contract at begin() time. Excluded
// from the native env via platformio.ini.
//
// This is room_mqtt_runtime.cpp with the contract as a parameter instead of a hardcoded call to
// mqtt_room_contract(): the last-will, client id and topics all come from the contract passed
// in, so a new product cannot publish on another station's topics by forgetting to edit a copy.
// The Room image still runs its own file; moving it over is a separate change, made once this
// one has run on a board for a while. The reasoning behind the Wi-Fi retry, the keepalive and
// the publish-from-the-loop-task rule is written up in room_mqtt_runtime.cpp and holds here.
struct Esp32MqttRuntimeConfig {
    const MqttProductContract* contract = nullptr;
    int keepalive_sec = 45;
    int network_timeout_ms = 30000;
    // ESP32-C3 SuperMini: the ceramic antenna couples badly and the module drops off the network
    // at full TX power; SP-01 makes the ~8.5 dBm limit part of the product.
    bool limit_tx_power = false;
};

bool esp32_mqtt_runtime_begin(const Esp32MqttRuntimeConfig& config);
void esp32_mqtt_runtime_tick(unsigned long now_ms);
// Queued only; the next tick drains it from the loop task.
void esp32_mqtt_runtime_publish_payload(std::string payload);

bool esp32_mqtt_runtime_enabled();
bool esp32_mqtt_runtime_wifi_up();
bool esp32_mqtt_runtime_mqtt_up();
int esp32_mqtt_runtime_rssi_dbm();

}  // namespace atmosmesh

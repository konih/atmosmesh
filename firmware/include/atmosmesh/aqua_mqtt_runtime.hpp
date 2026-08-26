#pragma once

#include <cstdint>

#include "atmosmesh/mqtt_contract.hpp"

namespace atmosmesh {

inline constexpr std::uint32_t kAquaMqttTransportConnectBudgetMs = 1000U;

inline std::uint32_t aqua_mqtt_connect_budget_remaining_ms(std::uint32_t started_ms,
                                                           std::uint32_t now_ms) {
    const std::uint32_t elapsed_ms = now_ms - started_ms;
    return elapsed_ms >= kAquaMqttTransportConnectBudgetMs
               ? 0U
               : kAquaMqttTransportConnectBudgetMs - elapsed_ms;
}

inline bool aqua_network_work_allowed(bool water_power_active) {
    return !water_power_active;
}

// ESP8266-only transport. The shared contract/session remains host-testable; this translation unit
// is selected only by the Aqua PlatformIO environment.
bool aqua_mqtt_runtime_begin();
void aqua_mqtt_runtime_tick(unsigned long now_ms);
void aqua_mqtt_runtime_publish_state(const AquaMqttState& state);
bool aqua_mqtt_runtime_enabled();
bool aqua_mqtt_runtime_wifi_up();
bool aqua_mqtt_runtime_mqtt_up();

}  // namespace atmosmesh

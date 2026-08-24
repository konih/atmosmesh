#pragma once

#include <cstdint>

#include "atmosmesh/mqtt_contract.hpp"

namespace atmosmesh {

inline constexpr std::uint32_t kGroveMqttTransportConnectBudgetMs = 1000U;

inline std::uint32_t grove_mqtt_connect_budget_remaining_ms(std::uint32_t started_ms,
                                                            std::uint32_t now_ms) {
    const std::uint32_t elapsed_ms = now_ms - started_ms;
    return elapsed_ms >= kGroveMqttTransportConnectBudgetMs
               ? 0U
               : kGroveMqttTransportConnectBudgetMs - elapsed_ms;
}

// ESP8266-only transport. The shared contract/session remains host-testable; this translation unit
// is selected only by the Grove PlatformIO environment.
bool grove_mqtt_runtime_begin();
void grove_mqtt_runtime_tick(unsigned long now_ms);
void grove_mqtt_runtime_publish_state(const GroveMqttState& state);
bool grove_mqtt_runtime_enabled();
bool grove_mqtt_runtime_wifi_up();
bool grove_mqtt_runtime_mqtt_up();

}  // namespace atmosmesh

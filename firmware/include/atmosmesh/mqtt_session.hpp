#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "atmosmesh/mqtt_contract.hpp"

namespace atmosmesh {

enum class MqttSessionActionKind {
    None,
    PublishDiscovery,
    PublishAvailabilityOnline,
    PublishState,
};

struct MqttPublishAction {
    MqttSessionActionKind kind = MqttSessionActionKind::None;
    std::string topic;
    std::string payload;
    bool retained = false;
};

struct MqttSession {
    const MqttProductContract* contract = nullptr;
    bool wifi_up = false;
    bool mqtt_connected = false;
    bool discovery_sent = false;
    bool online_sent = false;
    bool state_pending = false;
    MqttStationState pending_state{};
    std::string pending_state_payload;
    unsigned long backoff_ms = 1000;
    unsigned long next_attempt_ms = 0;
    bool reconnect_due = false;
};

inline constexpr unsigned long kMqttBackoffInitialMs = 1000;
inline constexpr unsigned long kMqttBackoffCapMs = 30000;

void mqtt_session_reset_backoff(MqttSession& session);
unsigned long mqtt_session_next_backoff_ms(unsigned long current_ms);
void mqtt_session_note_disconnect(MqttSession& session, unsigned long now_ms);
void mqtt_session_note_connect(MqttSession& session);
void mqtt_session_use_contract(MqttSession& session, const MqttProductContract& contract);
void mqtt_session_queue_state(MqttSession& session, const MqttStationState& state);
void mqtt_session_queue_payload(MqttSession& session, std::string payload);

// Pure tick: returns ordered publish actions for this slice. Does not block.
// When disconnected, returns empty — never invents publish work for a dead link.
std::vector<MqttPublishAction> mqtt_session_tick(MqttSession& session, unsigned long now_ms);

bool mqtt_session_should_attempt_reconnect(const MqttSession& session, unsigned long now_ms);

}  // namespace atmosmesh

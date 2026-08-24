#include "atmosmesh/mqtt_session.hpp"

namespace atmosmesh {
namespace {

const MqttProductContract& session_contract(const MqttSession& session) {
    return session.contract == nullptr ? mqtt_v1_contract() : *session.contract;
}

}  // namespace

void mqtt_session_reset_backoff(MqttSession& session) {
    session.backoff_ms = kMqttBackoffInitialMs;
    session.next_attempt_ms = 0;
    session.reconnect_due = false;
}

unsigned long mqtt_session_next_backoff_ms(unsigned long current_ms) {
    if (current_ms >= kMqttBackoffCapMs) {
        return kMqttBackoffCapMs;
    }
    const unsigned long doubled = current_ms * 2UL;
    return doubled > kMqttBackoffCapMs ? kMqttBackoffCapMs : doubled;
}

void mqtt_session_note_disconnect(MqttSession& session, unsigned long now_ms) {
    session.mqtt_connected = false;
    session.discovery_sent = false;
    session.online_sent = false;
    if (session.backoff_ms == 0) {
        session.backoff_ms = kMqttBackoffInitialMs;
    }
    session.next_attempt_ms = now_ms + session.backoff_ms;
    session.reconnect_due = true;
    session.backoff_ms = mqtt_session_next_backoff_ms(session.backoff_ms);
}

void mqtt_session_note_connect(MqttSession& session) {
    session.mqtt_connected = true;
    session.discovery_sent = false;
    session.online_sent = false;
    mqtt_session_reset_backoff(session);
}

void mqtt_session_use_contract(MqttSession& session, const MqttProductContract& contract) {
    session.contract = &contract;
}

void mqtt_session_queue_state(MqttSession& session, const MqttStationState& state) {
    session.pending_state = state;
    session.pending_state_payload = mqtt_state_json(state);
    session.state_pending = true;
}

void mqtt_session_queue_payload(MqttSession& session, std::string payload) {
    session.pending_state_payload = std::move(payload);
    session.state_pending = true;
}

void mqtt_session_note_publish_success(MqttSession& session, MqttSessionActionKind kind) {
    if (kind != MqttSessionActionKind::PublishState) {
        return;
    }
    session.state_pending = false;
    session.pending_state_payload.clear();
}

bool mqtt_session_should_attempt_reconnect(const MqttSession& session, unsigned long now_ms) {
    if (session.mqtt_connected) {
        return false;
    }
    if (!session.reconnect_due && session.next_attempt_ms == 0) {
        return true;
    }
    return now_ms >= session.next_attempt_ms;
}

std::vector<MqttPublishAction> mqtt_session_tick(MqttSession& session, unsigned long /*now_ms*/) {
    std::vector<MqttPublishAction> actions;
    if (!session.mqtt_connected) {
        return actions;
    }

    if (!session.discovery_sent) {
        const auto& contract = session_contract(session);
        const std::size_t count = mqtt_discovery_config_count(contract);
        for (std::size_t i = 0; i < count; ++i) {
            const auto cfg = mqtt_discovery_config_at(contract, i);
            MqttPublishAction action;
            action.kind = MqttSessionActionKind::PublishDiscovery;
            action.topic = cfg.topic;
            action.payload = cfg.payload;
            action.retained = true;
            actions.push_back(std::move(action));
        }
        session.discovery_sent = true;
    }

    if (!session.online_sent) {
        const auto& contract = session_contract(session);
        MqttPublishAction action;
        action.kind = MqttSessionActionKind::PublishAvailabilityOnline;
        action.topic = contract.availability_topic;
        action.payload = kMqttAvailabilityOnline;
        action.retained = true;
        actions.push_back(std::move(action));
        session.online_sent = true;
    }

    if (session.state_pending) {
        const auto& contract = session_contract(session);
        MqttPublishAction action;
        action.kind = MqttSessionActionKind::PublishState;
        action.topic = contract.state_topic;
        action.payload = session.pending_state_payload.empty()
                             ? mqtt_state_json(session.pending_state)
                             : session.pending_state_payload;
        action.retained = false;
        actions.push_back(std::move(action));
    }

    return actions;
}

}  // namespace atmosmesh

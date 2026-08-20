#include "atmosmesh/mqtt_session.hpp"

namespace atmosmesh {

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

void mqtt_session_queue_state(MqttSession& session, const MqttStationState& state) {
    session.pending_state = state;
    session.state_pending = true;
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
        const std::size_t count = mqtt_discovery_config_count();
        for (std::size_t i = 0; i < count; ++i) {
            const auto cfg = mqtt_discovery_config_at(i);
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
        MqttPublishAction action;
        action.kind = MqttSessionActionKind::PublishAvailabilityOnline;
        action.topic = kMqttAvailabilityTopic;
        action.payload = kMqttAvailabilityOnline;
        action.retained = true;
        actions.push_back(std::move(action));
        session.online_sent = true;
    }

    if (session.state_pending) {
        MqttPublishAction action;
        action.kind = MqttSessionActionKind::PublishState;
        action.topic = kMqttStateTopic;
        action.payload = mqtt_state_json(session.pending_state);
        action.retained = false;
        actions.push_back(std::move(action));
        session.state_pending = false;
    }

    return actions;
}

}  // namespace atmosmesh

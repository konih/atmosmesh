#include "atmosmesh/aqua_mqtt_runtime.hpp"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "atmosmesh/mqtt_session.hpp"

#if __has_include("atmosmesh/secrets.hpp")
#include "atmosmesh/secrets.hpp"
#define ATMOSMESH_AQUA_HAS_SECRETS 1
#else
#define ATMOSMESH_AQUA_HAS_SECRETS 0
#endif

namespace atmosmesh {
namespace {

class BoundedWiFiClient final : public WiFiClient {
   public:
    int connect(const char* host, std::uint16_t port) override {
        const std::uint32_t started_ms = millis();
        IPAddress remote_address;
        if (!WiFi.hostByName(host, remote_address, kAquaMqttTransportConnectBudgetMs)) {
            return 0;
        }
        const std::uint32_t remaining_ms =
            aqua_mqtt_connect_budget_remaining_ms(started_ms, millis());
        if (remaining_ms == 0U) {
            return 0;
        }
        setTimeout(remaining_ms);
        const int result = WiFiClient::connect(remote_address, port);
        setTimeout(kAquaMqttTransportConnectBudgetMs);
        return result;
    }

    int connect(IPAddress address, std::uint16_t port) override {
        setTimeout(kAquaMqttTransportConnectBudgetMs);
        return WiFiClient::connect(address, port);
    }
};

BoundedWiFiClient network_client;
PubSubClient mqtt_client(network_client);
MqttSession session{};
bool enabled = false;

bool apply_actions(const std::vector<MqttPublishAction>& actions, unsigned long now_ms) {
    for (const auto& action : actions) {
        if (action.kind == MqttSessionActionKind::None || action.topic.empty()) {
            continue;
        }
        if (!mqtt_client.publish(action.topic.c_str(), action.payload.c_str(), action.retained)) {
            Serial.println("mqtt: publish failed; reconnect will replay discovery/availability");
            if (action.kind == MqttSessionActionKind::PublishState) {
                mqtt_session_queue_payload(session, action.payload);
            }
            mqtt_client.disconnect();
            mqtt_session_note_disconnect(session, now_ms);
            return false;
        }
        mqtt_session_note_publish_success(session, action.kind);
    }
    return true;
}

void attempt_mqtt_connect(unsigned long now_ms) {
#if ATMOSMESH_AQUA_HAS_SECRETS
    const auto& contract = mqtt_aqua_contract();
    const auto will = mqtt_will_config(contract);
    const bool connected = mqtt_client.connect(
        contract.station_id, ATMOSMESH_MQTT_USER, ATMOSMESH_MQTT_PASSWORD,
        will.topic, will.qos, will.retained, will.payload);
    if (!connected) {
        Serial.printf("mqtt: connect failed state=%d; retry is backed off\n", mqtt_client.state());
        mqtt_session_note_disconnect(session, now_ms);
        return;
    }
    Serial.println("mqtt: connected");
    mqtt_session_note_connect(session);
    apply_actions(mqtt_session_tick(session, now_ms), now_ms);
#else
    (void)now_ms;
#endif
}

}  // namespace

bool aqua_mqtt_runtime_enabled() {
    return enabled;
}

bool aqua_mqtt_runtime_wifi_up() {
    return enabled && WiFi.status() == WL_CONNECTED;
}

bool aqua_mqtt_runtime_mqtt_up() {
    return enabled && session.mqtt_connected && mqtt_client.connected();
}

bool aqua_mqtt_runtime_begin() {
    mqtt_session_use_contract(session, mqtt_aqua_contract());
#if !ATMOSMESH_AQUA_HAS_SECRETS
    Serial.println("mqtt: credentials missing; networking disabled (sensors+OLED continue)");
    enabled = false;
    return false;
#else
    enabled = true;
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ATMOSMESH_WIFI_SSID, ATMOSMESH_WIFI_PASSWORD);
    mqtt_client.setServer(ATMOSMESH_MQTT_HOST, ATMOSMESH_MQTT_PORT);
    mqtt_client.setKeepAlive(kMqttKeepaliveSec);
    mqtt_client.setSocketTimeout(1);
    mqtt_client.setBufferSize(768);
    Serial.println("wifi: asynchronous connection started");
    Serial.println("mqtt: reconnect backoff=1..30s DNS+TCP-budget=1000ms response-timeout=1s");
    return true;
#endif
}

void aqua_mqtt_runtime_tick(unsigned long now_ms) {
    if (!enabled) {
        return;
    }

    session.wifi_up = aqua_mqtt_runtime_wifi_up();
    if (!session.wifi_up) {
        if (session.mqtt_connected) {
            mqtt_client.disconnect();
            mqtt_session_note_disconnect(session, now_ms);
        }
        return;
    }

    if (mqtt_client.connected()) {
        mqtt_client.loop();
    }
    if (!mqtt_client.connected() && session.mqtt_connected) {
        mqtt_session_note_disconnect(session, now_ms);
    }
    if (!mqtt_client.connected() && mqtt_session_should_attempt_reconnect(session, now_ms)) {
        attempt_mqtt_connect(now_ms);
    }
    if (mqtt_client.connected()) {
        apply_actions(mqtt_session_tick(session, now_ms), now_ms);
    }
}

void aqua_mqtt_runtime_publish_state(const AquaMqttState& state) {
    if (!enabled) {
        return;
    }
    mqtt_session_queue_payload(session, aqua_mqtt_state_json(state));
}

}  // namespace atmosmesh

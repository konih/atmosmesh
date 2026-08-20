#include "atmosmesh/mqtt_runtime.hpp"

#include <WiFi.h>
#include <esp_event.h>
#include <mqtt_client.h>

#include <cstdio>
#include <cstring>

// Optional local credentials. Copy secrets.hpp.example → secrets.hpp (gitignored).
#if __has_include("atmosmesh/secrets.hpp")
#include "atmosmesh/secrets.hpp"
#define ATMOSMESH_HAS_SECRETS 1
#else
#define ATMOSMESH_HAS_SECRETS 0
#endif

namespace atmosmesh {
namespace {

MqttSession g_session{};
esp_mqtt_client_handle_t g_client = nullptr;
bool g_wifi_started = false;
bool g_mqtt_started = false;
bool g_enabled = false;

void apply_actions(const std::vector<MqttPublishAction>& actions) {
    if (g_client == nullptr) {
        return;
    }
    for (const auto& action : actions) {
        if (action.kind == MqttSessionActionKind::None || action.topic.empty()) {
            continue;
        }
        const int qos = 0;
        const int retain = action.retained ? 1 : 0;
        esp_mqtt_client_publish(g_client, action.topic.c_str(), action.payload.c_str(),
                                static_cast<int>(action.payload.size()), qos, retain);
    }
}

void on_mqtt_event(void* /*handler_args*/, esp_event_base_t /*base*/, int32_t event_id,
                   void* event_data) {
    const auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            Serial.println("mqtt: connected");
            mqtt_session_note_connect(g_session);
            apply_actions(mqtt_session_tick(g_session, millis()));
            break;
        case MQTT_EVENT_DISCONNECTED:
            Serial.println("mqtt: disconnected");
            mqtt_session_note_disconnect(g_session, millis());
            break;
        case MQTT_EVENT_ERROR:
            Serial.println("mqtt: error");
            break;
        default:
            break;
    }
    (void)event;
}

void ensure_mqtt_client() {
    if (g_mqtt_started || !g_enabled) {
        return;
    }
#if ATMOSMESH_HAS_SECRETS
    char uri[128];
    std::snprintf(uri, sizeof(uri), "mqtt://%s:%d", ATMOSMESH_MQTT_HOST, ATMOSMESH_MQTT_PORT);

    // Arduino-ESP32 on this board uses the IDF 4.x flat mqtt config (not IDF 5 nested).
    esp_mqtt_client_config_t cfg = {};
    cfg.uri = uri;
    cfg.username = ATMOSMESH_MQTT_USER;
    cfg.password = ATMOSMESH_MQTT_PASSWORD;
    cfg.keepalive = kMqttKeepaliveSec;
    cfg.lwt_topic = kMqttAvailabilityTopic;
    cfg.lwt_msg = kMqttAvailabilityOffline;
    cfg.lwt_msg_len = static_cast<int>(strlen(kMqttAvailabilityOffline));
    cfg.lwt_qos = 0;
    cfg.lwt_retain = 1;

    g_client = esp_mqtt_client_init(&cfg);
    if (g_client == nullptr) {
        Serial.println("mqtt: client init failed");
        return;
    }
    esp_mqtt_client_register_event(g_client, MQTT_EVENT_ANY, on_mqtt_event, nullptr);
    const esp_err_t err = esp_mqtt_client_start(g_client);
    if (err != ESP_OK) {
        Serial.printf("mqtt: start failed err=%d\n", static_cast<int>(err));
        return;
    }
    g_mqtt_started = true;
    Serial.printf("mqtt: starting broker=%s:%d user=%s\n", ATMOSMESH_MQTT_HOST,
                  ATMOSMESH_MQTT_PORT, ATMOSMESH_MQTT_USER);
#else
    (void)0;
#endif
}

}  // namespace

bool mqtt_runtime_enabled() {
    return g_enabled;
}

bool mqtt_runtime_wifi_up() {
    return WiFi.status() == WL_CONNECTED;
}

bool mqtt_runtime_mqtt_up() {
    return g_session.mqtt_connected;
}

bool mqtt_runtime_begin() {
#if !ATMOSMESH_HAS_SECRETS
    Serial.println("mqtt: secrets.hpp missing — Wi-Fi/MQTT disabled (sensors+OLED only)");
    g_enabled = false;
    return false;
#else
    g_enabled = true;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ATMOSMESH_WIFI_SSID, ATMOSMESH_WIFI_PASSWORD);
    g_wifi_started = true;
    Serial.printf("wifi: connecting ssid=%s (async)\n", ATMOSMESH_WIFI_SSID);
    return true;
#endif
}

void mqtt_runtime_tick(unsigned long now_ms) {
    if (!g_enabled) {
        return;
    }

    g_session.wifi_up = mqtt_runtime_wifi_up();
    if (!g_session.wifi_up) {
        if (g_session.mqtt_connected) {
            mqtt_session_note_disconnect(g_session, now_ms);
        }
        return;
    }

    if (!g_mqtt_started) {
        if (mqtt_session_should_attempt_reconnect(g_session, now_ms) || !g_session.reconnect_due) {
            ensure_mqtt_client();
        }
    }

    // Discovery/state publishes are driven from the CONNECT event and queue_state.
    apply_actions(mqtt_session_tick(g_session, now_ms));
}

void mqtt_runtime_publish_state(const MqttStationState& state) {
    if (!g_enabled) {
        return;
    }
    mqtt_session_queue_state(g_session, state);
    apply_actions(mqtt_session_tick(g_session, millis()));
}

}  // namespace atmosmesh

#include "atmosmesh/esp32_mqtt_runtime.hpp"

#include <WiFi.h>
#include <esp_event.h>
#include <esp_idf_version.h>
#include <mqtt_client.h>

#include <cstdio>
#include <cstring>
#include <vector>

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
Esp32MqttRuntimeConfig g_config{};
esp_mqtt_client_handle_t g_client = nullptr;
bool g_mqtt_started = false;
bool g_enabled = false;
unsigned long g_wifi_attempt_ms = 0;
int g_wifi_status_shown = -1;
bool g_rssi_logged = false;

// A single WiFi.begin() is not an association strategy (see room_mqtt_runtime.cpp): re-issue it
// every 30 s while unassociated, and never power the radio down to do so.
constexpr unsigned long kWifiRetryMs = 30000UL;

const char* wifi_status_name(int status) {
    switch (status) {
        case WL_IDLE_STATUS: return "idle";
        case WL_NO_SSID_AVAIL: return "no-ssid";
        case WL_SCAN_COMPLETED: return "scan-done";
        case WL_CONNECTED: return "connected";
        case WL_CONNECT_FAILED: return "connect-failed";
        case WL_CONNECTION_LOST: return "connection-lost";
        case WL_DISCONNECTED: return "disconnected";
        default: return "unknown";
    }
}

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
        const int message_id =
            esp_mqtt_client_publish(g_client, action.topic.c_str(), action.payload.c_str(),
                                    static_cast<int>(action.payload.size()), qos, retain);
        mqtt_session_note_publish_result(g_session, action.kind, message_id);
    }
}

void on_mqtt_event(void* /*handler_args*/, esp_event_base_t /*base*/, int32_t event_id,
                   void* event_data) {
    const auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            // Note it and nothing else: this runs on the esp-mqtt task, and publishing from here
            // deadlocks the client. The publishes happen in esp32_mqtt_runtime_tick().
            Serial.println("mqtt: connected");
            mqtt_session_note_connect(g_session);
            break;
        case MQTT_EVENT_DISCONNECTED:
            Serial.println("mqtt: disconnected");
            mqtt_session_note_disconnect(g_session, millis());
            break;
        case MQTT_EVENT_ERROR:
            if (event != nullptr && event->error_handle != nullptr) {
                Serial.printf(
                    "mqtt: error type=%d connect_rc=%d sock_errno=%d transport_esp_err=0x%x\n",
                    static_cast<int>(event->error_handle->error_type),
                    static_cast<int>(event->error_handle->connect_return_code),
                    event->error_handle->esp_transport_sock_errno,
                    static_cast<unsigned>(event->error_handle->esp_tls_last_esp_err));
            } else {
                Serial.println("mqtt: error (no detail)");
            }
            break;
        default:
            break;
    }
}

void ensure_mqtt_client() {
    if (g_mqtt_started || !g_enabled || g_config.contract == nullptr) {
        return;
    }
#if ATMOSMESH_HAS_SECRETS
    const MqttProductContract& contract = *g_config.contract;
    const MqttWillConfig will = mqtt_will_config(contract);

    static char uri[128];
    std::snprintf(uri, sizeof(uri), "mqtt://%s:%d", ATMOSMESH_MQTT_HOST, ATMOSMESH_MQTT_PORT);

    esp_mqtt_client_config_t cfg = {};
#if ESP_IDF_VERSION_MAJOR >= 5
    // Arduino core 3.x sits on IDF 5, whose config is nested; the Room image (core 2.x, IDF 4)
    // uses the flat form below.
    cfg.broker.address.uri = uri;
    cfg.credentials.username = ATMOSMESH_MQTT_USER;
    cfg.credentials.authentication.password = ATMOSMESH_MQTT_PASSWORD;
    cfg.credentials.client_id = contract.station_id;
    cfg.session.keepalive = g_config.keepalive_sec;
    cfg.network.timeout_ms = g_config.network_timeout_ms;
    cfg.session.last_will.topic = will.topic;
    cfg.session.last_will.msg = will.payload;
    cfg.session.last_will.msg_len = static_cast<int>(strlen(will.payload));
    cfg.session.last_will.qos = will.qos;
    cfg.session.last_will.retain = will.retained ? 1 : 0;
#else
    cfg.uri = uri;
    cfg.username = ATMOSMESH_MQTT_USER;
    cfg.password = ATMOSMESH_MQTT_PASSWORD;
    cfg.client_id = contract.station_id;
    cfg.keepalive = g_config.keepalive_sec;
    cfg.network_timeout_ms = g_config.network_timeout_ms;
    cfg.lwt_topic = will.topic;
    cfg.lwt_msg = will.payload;
    cfg.lwt_msg_len = static_cast<int>(strlen(will.payload));
    cfg.lwt_qos = will.qos;
    cfg.lwt_retain = will.retained ? 1 : 0;
#endif

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
    Serial.printf("mqtt: starting broker=%s:%d user=%s client=%s state=%s\n", ATMOSMESH_MQTT_HOST,
                  ATMOSMESH_MQTT_PORT, ATMOSMESH_MQTT_USER, contract.station_id,
                  contract.state_topic);
#else
    (void)0;
#endif
}

}  // namespace

bool esp32_mqtt_runtime_enabled() {
    return g_enabled;
}

bool esp32_mqtt_runtime_wifi_up() {
    return WiFi.status() == WL_CONNECTED;
}

bool esp32_mqtt_runtime_mqtt_up() {
    return g_session.mqtt_connected;
}

int esp32_mqtt_runtime_rssi_dbm() {
    return esp32_mqtt_runtime_wifi_up() ? static_cast<int>(WiFi.RSSI()) : 0;
}

bool esp32_mqtt_runtime_begin(const Esp32MqttRuntimeConfig& config) {
    g_config = config;
    if (g_config.contract == nullptr) {
        Serial.println("mqtt: no contract given - networking disabled");
        g_enabled = false;
        return false;
    }
    // Bound before anything can tick: a session with a null contract answers with v1's topics.
    mqtt_session_use_contract(g_session, *g_config.contract);
#if !ATMOSMESH_HAS_SECRETS
    Serial.println("mqtt: secrets.hpp missing — Wi-Fi/MQTT disabled (sensors+display only)");
    g_enabled = false;
    return false;
#else
    g_enabled = true;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ATMOSMESH_WIFI_SSID, ATMOSMESH_WIFI_PASSWORD);
    if (g_config.limit_tx_power) {
        const bool set = WiFi.setTxPower(WIFI_POWER_8_5dBm);
        Serial.printf("wifi: tx power limit 8.5dBm %s (now %d/4 dBm)\n", set ? "set" : "REFUSED",
                      static_cast<int>(WiFi.getTxPower()));
    }
    g_wifi_attempt_ms = millis();
    Serial.printf("wifi: connecting ssid=%s (async, retry every %lus)\n", ATMOSMESH_WIFI_SSID,
                  kWifiRetryMs / 1000UL);
    return true;
#endif
}

void esp32_mqtt_runtime_tick(unsigned long now_ms) {
    if (!g_enabled) {
        return;
    }

#if ATMOSMESH_HAS_SECRETS
    const int status = WiFi.status();
    if (status != g_wifi_status_shown) {
        Serial.printf("wifi: %s\n", wifi_status_name(status));
        g_wifi_status_shown = status;
        if (status != WL_CONNECTED) {
            g_rssi_logged = false;
        }
    }
#endif

    g_session.wifi_up = esp32_mqtt_runtime_wifi_up();
    if (!g_session.wifi_up) {
        if (g_session.mqtt_connected) {
            mqtt_session_note_disconnect(g_session, now_ms);
        }
#if ATMOSMESH_HAS_SECRETS
        if (now_ms - g_wifi_attempt_ms >= kWifiRetryMs) {
            Serial.println("wifi: still not associated - re-issuing begin()");
            WiFi.begin(ATMOSMESH_WIFI_SSID, ATMOSMESH_WIFI_PASSWORD);
            g_wifi_attempt_ms = now_ms;
        }
#endif
        return;
    }
    if (g_wifi_status_shown == WL_CONNECTED && !g_rssi_logged) {
        Serial.printf("wifi: ip=%s rssi=%ddBm ch=%d\n", WiFi.localIP().toString().c_str(),
                      static_cast<int>(WiFi.RSSI()), static_cast<int>(WiFi.channel()));
        g_rssi_logged = true;
    }
    g_wifi_attempt_ms = now_ms;

    if (!g_mqtt_started) {
        if (mqtt_session_should_attempt_reconnect(g_session, now_ms) || !g_session.reconnect_due) {
            ensure_mqtt_client();
        }
    }

    // The only place actions are applied: every esp_mqtt_client_publish() runs on the loop task.
    apply_actions(mqtt_session_tick(g_session, now_ms));
}

void esp32_mqtt_runtime_publish_payload(std::string payload) {
    if (!g_enabled) {
        return;
    }
    mqtt_session_queue_payload(g_session, std::move(payload));
}

}  // namespace atmosmesh

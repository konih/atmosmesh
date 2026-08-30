#include "atmosmesh/room_mqtt_runtime.hpp"

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
bool g_mqtt_started = false;
bool g_enabled = false;
unsigned long g_wifi_attempt_ms = 0;
int g_wifi_status_shown = -1;
bool g_rssi_logged = false;

// A single WiFi.begin() is not an association strategy. Observed on this board against a
// consumer access point: the first attempt can sit in WL_DISCONNECTED indefinitely -- three
// minutes with no progress and no error -- while a re-issued begin() associates within seconds.
// Retrying is the difference between "Home Assistant gets readings" and a board that looks fine
// on its own display and silently never publishes.
//
// 30 s, and the radio is never powered down to do it. An earlier 20 s retry called
// WiFi.disconnect(true), whose first argument is "turn the radio off" rather than "forget the
// AP"; that tore down association attempts that were still in progress and made the board worse
// at connecting than doing nothing at all. Give an attempt time to finish before replacing it.
constexpr unsigned long kWifiRetryMs = 30000UL;

constexpr int kRoomMqttKeepaliveSec = 45;
constexpr int kRoomMqttNetworkTimeoutMs = 30000;

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
            // Note the connection and nothing else. This handler runs on the esp-mqtt client's
            // own task, and esp_mqtt_client_publish() must not be called from there -- it can
            // block waiting on the very task that is currently inside this callback. Publishing
            // the discovery burst here made the client error out and drop the link a moment
            // after every connect, so the board looped connect -> error -> backoff and only the
            // first retained config ever reached the broker. The publishes now happen in
            // room_mqtt_runtime_tick(), which runs on the Arduino loop task.
            Serial.println("mqtt: connected");
            mqtt_session_note_connect(g_session);
            break;
        case MQTT_EVENT_DISCONNECTED:
            Serial.println("mqtt: disconnected");
            mqtt_session_note_disconnect(g_session, millis());
            break;
        case MQTT_EVENT_ERROR:
            // "mqtt: error" on its own is not a diagnosis. error_type separates a refused
            // CONNECT (bad credentials, duplicate client id) from a transport failure, and
            // sock_errno names the latter exactly.
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
    if (g_mqtt_started || !g_enabled) {
        return;
    }
#if ATMOSMESH_HAS_SECRETS
    const MqttProductContract& contract = mqtt_room_contract();
    const MqttWillConfig will = mqtt_will_config(contract);

    char uri[128];
    std::snprintf(uri, sizeof(uri), "mqtt://%s:%d", ATMOSMESH_MQTT_HOST, ATMOSMESH_MQTT_PORT);

    // Arduino-ESP32 on this board uses the IDF 4.x flat mqtt config (not IDF 5 nested).
    esp_mqtt_client_config_t cfg = {};
    cfg.uri = uri;
    cfg.username = ATMOSMESH_MQTT_USER;
    cfg.password = ATMOSMESH_MQTT_PASSWORD;
    // A stable client id rather than esp-mqtt's "ESP32_<mac>" default, so this board is
    // identifiable in broker logs and can never collide with another AtmosMesh node.
    cfg.client_id = contract.station_id;
    // This board sits at about -75 dBm. The stock 15 s keepalive and 10 s network timeout tore
    // the session down roughly every ten seconds on that link -- the observed failure was
    // error_type=TCP_TRANSPORT with sock_errno 0, i.e. the read simply returned nothing in
    // time. Wider timeouts let a lossy but working link stay up; expire_after is 90 s, so Home
    // Assistant still marks entities unavailable well before a genuinely dead board looks live.
    cfg.keepalive = kRoomMqttKeepaliveSec;
    cfg.network_timeout_ms = kRoomMqttNetworkTimeoutMs;
    // The room's own availability topic, not v1's. A last-will is fixed at client construction,
    // so getting this wrong would mark the wrong station offline when this board drops.
    cfg.lwt_topic = will.topic;
    cfg.lwt_msg = will.payload;
    cfg.lwt_msg_len = static_cast<int>(strlen(will.payload));
    cfg.lwt_qos = will.qos;
    cfg.lwt_retain = will.retained ? 1 : 0;

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
    Serial.printf("mqtt: starting broker=%s:%d user=%s state=%s\n", ATMOSMESH_MQTT_HOST,
                  ATMOSMESH_MQTT_PORT, ATMOSMESH_MQTT_USER, contract.state_topic);
#else
    (void)0;
#endif
}

}  // namespace

bool room_mqtt_runtime_enabled() {
    return g_enabled;
}

bool room_mqtt_runtime_wifi_up() {
    return WiFi.status() == WL_CONNECTED;
}

bool room_mqtt_runtime_mqtt_up() {
    return g_session.mqtt_connected;
}

bool room_mqtt_runtime_begin() {
    // Bound before anything can tick: an MqttSession with a null contract silently answers with
    // the v1 contract, which would put room readings on atmosmesh-0001's topics.
    mqtt_session_use_contract(g_session, mqtt_room_contract());
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
    g_wifi_attempt_ms = millis();
    Serial.printf("wifi: connecting ssid=%s (async, retry every %lus)\n", ATMOSMESH_WIFI_SSID,
                  kWifiRetryMs / 1000UL);
    return true;
#endif
}

void room_mqtt_runtime_tick(unsigned long now_ms) {
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

    g_session.wifi_up = room_mqtt_runtime_wifi_up();
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
        // Worth one line: a board that associates slowly and drops often is usually an antenna
        // problem, and RSSI is the number that says so.
        Serial.printf("wifi: ip=%s rssi=%ddBm ch=%d\n", WiFi.localIP().toString().c_str(),
                      static_cast<int>(WiFi.RSSI()), static_cast<int>(WiFi.channel()));
        g_rssi_logged = true;
    }
    g_wifi_attempt_ms = now_ms;   // associated: the retry clock only runs while it is not

    if (!g_mqtt_started) {
        if (mqtt_session_should_attempt_reconnect(g_session, now_ms) || !g_session.reconnect_due) {
            ensure_mqtt_client();
        }
    }

    // The only place actions are applied, so every esp_mqtt_client_publish() call in this
    // translation unit is made from the loop task and never from the client's event callback.
    apply_actions(mqtt_session_tick(g_session, now_ms));
}

void room_mqtt_runtime_publish_state(const RoomMqttState& state) {
    if (!g_enabled) {
        return;
    }
    // Queued only. room_mqtt_runtime_tick() runs on the next loop pass, a millisecond or so
    // away, and drains it -- there is no reason for two publish paths.
    mqtt_session_queue_payload(g_session, room_mqtt_state_json(state));
}

}  // namespace atmosmesh

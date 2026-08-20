#include <string>

#include <unity.h>

#include "atmosmesh/mqtt_contract.hpp"
#include "atmosmesh/mqtt_session.hpp"

namespace {

void assert_no_room_or_forbidden_gas(const std::string& text) {
    TEST_ASSERT_FALSE_MESSAGE(atmosmesh::mqtt_payload_mentions_forbidden_room(text), text.c_str());
    TEST_ASSERT_FALSE_MESSAGE(atmosmesh::mqtt_payload_mentions_forbidden_gas_label(text),
                              text.c_str());
    TEST_ASSERT_EQUAL(std::string::npos, text.find("lux"));
    TEST_ASSERT_EQUAL(std::string::npos, text.find("wohnzimmer"));
    TEST_ASSERT_EQUAL(std::string::npos, text.find("office"));
}

}  // namespace

void test_mqtt_ids_and_topics_are_station_not_room() {
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1", atmosmesh::kMqttDeviceId);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-0001", atmosmesh::kMqttStationId);
    TEST_ASSERT_EQUAL_STRING("atmosmesh_0001", atmosmesh::kMqttDiscoveryNode);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-0001/state", atmosmesh::kMqttStateTopic);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-0001/availability",
                             atmosmesh::kMqttAvailabilityTopic);
    TEST_ASSERT_EQUAL_STRING("online", atmosmesh::kMqttAvailabilityOnline);
    TEST_ASSERT_EQUAL_STRING("offline", atmosmesh::kMqttAvailabilityOffline);
    TEST_ASSERT_EQUAL_INT(15, atmosmesh::kMqttKeepaliveSec);
    TEST_ASSERT_EQUAL_INT(90, atmosmesh::kMqttExpireAfterSec);
    assert_no_room_or_forbidden_gas(atmosmesh::kMqttStateTopic);
    assert_no_room_or_forbidden_gas(atmosmesh::kMqttAvailabilityTopic);
}

void test_state_json_includes_identity_units_and_omits_invalid_values() {
    atmosmesh::MqttStationState state{};
    state.temperature = {23.4F, true, 100};
    state.humidity = {48.0F, true, 100};
    state.pressure = {1013.2F, true, 200};
    state.bmp_temperature = {22.1F, true, 200};
    state.pm25 = {12.3F, true, 50};
    state.pm10.valid = false;
    state.pm10.age_ms = 6000;
    state.gas_index = {20.0F, true, 100};
    state.mq135_raw = 819;
    state.mq135_raw_valid = true;
    state.motion = {true, true, 10};

    const std::string json = atmosmesh::mqtt_state_json(state);
    assert_no_room_or_forbidden_gas(json);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"id\":\"atmosmesh-0001\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"device\":\"atmosmesh-v1\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"gas_index\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"index\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"mq135_raw\":819"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"value\":23.4"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"C\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"%\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"hPa\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"ug/m3\""));
    // Invalid PM10 must not invent a numeric zero.
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"pm10\":{\"value\":"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"pm10\":{\"unit\":\"ug/m3\",\"valid\":false"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("co2"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("CO2"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("ppm"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"lux\""));
}

void test_discovery_configs_cover_sensors_without_lux() {
    const std::size_t count = atmosmesh::mqtt_discovery_config_count();
    TEST_ASSERT_GREATER_OR_EQUAL(8, static_cast<int>(count));

    bool saw_temperature = false;
    bool saw_motion = false;
    bool saw_gas = false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto cfg = atmosmesh::mqtt_discovery_config_at(i);
        assert_no_room_or_forbidden_gas(cfg.topic);
        assert_no_room_or_forbidden_gas(cfg.payload);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("homeassistant/"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("atmosmesh_0001"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("\"availability_topic\""));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(atmosmesh::kMqttAvailabilityTopic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(atmosmesh::kMqttStateTopic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("\"expire_after\":90"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("value_template"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("atmosmesh-0001_"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("atmosmesh-v1"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("lux"));
        if (std::string(cfg.object_id) == "temperature") {
            saw_temperature = true;
            TEST_ASSERT_EQUAL_STRING("sensor", cfg.component);
        }
        if (std::string(cfg.object_id) == "motion") {
            saw_motion = true;
            TEST_ASSERT_EQUAL_STRING("binary_sensor", cfg.component);
        }
        if (std::string(cfg.object_id) == "gas_index") {
            saw_gas = true;
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("device_class"));
        }
    }
    TEST_ASSERT_TRUE(saw_temperature);
    TEST_ASSERT_TRUE(saw_motion);
    TEST_ASSERT_TRUE(saw_gas);
}

void test_backoff_doubles_until_cap() {
    TEST_ASSERT_EQUAL_UINT32(1000, atmosmesh::kMqttBackoffInitialMs);
    TEST_ASSERT_EQUAL_UINT32(30000, atmosmesh::kMqttBackoffCapMs);
    TEST_ASSERT_EQUAL_UINT32(2000, atmosmesh::mqtt_session_next_backoff_ms(1000));
    TEST_ASSERT_EQUAL_UINT32(4000, atmosmesh::mqtt_session_next_backoff_ms(2000));
    TEST_ASSERT_EQUAL_UINT32(30000, atmosmesh::mqtt_session_next_backoff_ms(16000));
    TEST_ASSERT_EQUAL_UINT32(30000, atmosmesh::mqtt_session_next_backoff_ms(30000));
}

void test_session_does_not_publish_when_disconnected() {
    atmosmesh::MqttSession session{};
    session.state_pending = true;
    session.pending_state.temperature = {21.0F, true, 0};
    const auto actions = atmosmesh::mqtt_session_tick(session, 5000);
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(actions.size()));
}

void test_session_publishes_discovery_online_then_state_on_connect() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::MqttStationState state{};
    state.temperature = {21.5F, true, 10};
    atmosmesh::mqtt_session_queue_state(session, state);

    const auto actions = atmosmesh::mqtt_session_tick(session, 100);
    TEST_ASSERT_GREATER_OR_EQUAL(3, static_cast<int>(actions.size()));
    TEST_ASSERT_EQUAL(static_cast<int>(atmosmesh::MqttSessionActionKind::PublishDiscovery),
                      static_cast<int>(actions.front().kind));
    TEST_ASSERT_TRUE(actions.front().retained);

    bool saw_online = false;
    bool saw_state = false;
    for (const auto& action : actions) {
        if (action.kind == atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline) {
            saw_online = true;
            TEST_ASSERT_EQUAL_STRING(atmosmesh::kMqttAvailabilityTopic, action.topic.c_str());
            TEST_ASSERT_EQUAL_STRING(atmosmesh::kMqttAvailabilityOnline, action.payload.c_str());
            TEST_ASSERT_TRUE(action.retained);
        }
        if (action.kind == atmosmesh::MqttSessionActionKind::PublishState) {
            saw_state = true;
            TEST_ASSERT_EQUAL_STRING(atmosmesh::kMqttStateTopic, action.topic.c_str());
            TEST_ASSERT_FALSE(action.retained);
            assert_no_room_or_forbidden_gas(action.payload);
        }
    }
    TEST_ASSERT_TRUE(saw_online);
    TEST_ASSERT_TRUE(saw_state);
}

void test_session_reconnect_backoff_gates_attempts() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_note_disconnect(session, 1000);
    TEST_ASSERT_FALSE(atmosmesh::mqtt_session_should_attempt_reconnect(session, 1500));
    TEST_ASSERT_TRUE(atmosmesh::mqtt_session_should_attempt_reconnect(session, 2500));
    atmosmesh::mqtt_session_note_disconnect(session, 2500);
    // First disconnect advanced 1s→2s; second advances 2s→4s.
    TEST_ASSERT_EQUAL_UINT32(4000, session.backoff_ms);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_ids_and_topics_are_station_not_room);
    RUN_TEST(test_state_json_includes_identity_units_and_omits_invalid_values);
    RUN_TEST(test_discovery_configs_cover_sensors_without_lux);
    RUN_TEST(test_backoff_doubles_until_cap);
    RUN_TEST(test_session_does_not_publish_when_disconnected);
    RUN_TEST(test_session_publishes_discovery_online_then_state_on_connect);
    RUN_TEST(test_session_reconnect_backoff_gates_attempts);
    return UNITY_END();
}

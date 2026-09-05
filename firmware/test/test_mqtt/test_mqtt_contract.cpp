#include <cstdint>
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

void test_esp_publish_message_id_clears_only_successfully_queued_state() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::MqttStationState state{};
    state.temperature = {21.5F, true, 10};
    atmosmesh::mqtt_session_queue_state(session, state);

    atmosmesh::mqtt_session_note_publish_result(
        session, atmosmesh::MqttSessionActionKind::PublishState, -1);
    TEST_ASSERT_TRUE(session.state_pending);

    auto actions = atmosmesh::mqtt_session_tick(session, 100);
    bool replayed_state = false;
    for (const auto& action : actions) {
        replayed_state = replayed_state ||
                         action.kind == atmosmesh::MqttSessionActionKind::PublishState;
    }
    TEST_ASSERT_TRUE(replayed_state);

    // ESP-MQTT documents QoS 0 success as message id zero; positive ids are also success.
    atmosmesh::mqtt_session_note_publish_result(
        session, atmosmesh::MqttSessionActionKind::PublishState, 0);
    TEST_ASSERT_FALSE(session.state_pending);

    atmosmesh::mqtt_session_queue_state(session, state);
    atmosmesh::mqtt_session_note_publish_result(
        session, atmosmesh::MqttSessionActionKind::PublishState, 42);
    TEST_ASSERT_FALSE(session.state_pending);
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

void test_grove_contract_uses_stable_ids_and_separate_topics() {
    const auto& contract = atmosmesh::mqtt_grove_contract();
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-v1.5", contract.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-0001", contract.station_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh_grove_0001", contract.discovery_node);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-grove-0001/state", contract.state_topic);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-grove-0001/availability",
                             contract.availability_topic);
}

void test_grove_will_is_retained_offline_on_product_availability_topic() {
    const auto will = atmosmesh::mqtt_will_config(atmosmesh::mqtt_grove_contract());
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-grove-0001/availability", will.topic);
    TEST_ASSERT_EQUAL_STRING("offline", will.payload);
    TEST_ASSERT_EQUAL_INT(0, will.qos);
    TEST_ASSERT_TRUE(will.retained);
}

void test_grove_state_omits_missing_light_but_preserves_valid_zero_environment() {
    atmosmesh::GroveMqttState state{};
    state.temperature_c = {0.0F, true, 0};
    state.humidity_pct = {32.0F, true, 0};
    state.pressure_hpa = {984.0F, true, 0};
    state.light_charge_us = {0.0F, false, 0};
    state.soil_adc_raw = {0.0F, false, 0};

    std::string json = atmosmesh::grove_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"station_id\":\"atmosmesh-grove-0001\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"product_id\":\"atmosmesh-grove-v1.5\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"temperature_c\":0.0"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"humidity_pct\":32.0"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"pressure_hpa\":984.0"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("light_charge_us"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("soil_adc_raw"));

    state.light_charge_us = {4321.0F, true, 0};
    json = atmosmesh::grove_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"light_charge_us\":4321"));
    state.soil_adc_raw = {0.0F, true, 0};
    json = atmosmesh::grove_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"temperature_c\":"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"humidity_pct\":"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"pressure_hpa\":"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"light_charge_us\":"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"soil_adc_raw\":0"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("bmp_temperature"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("lux"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("percent"));
}

void test_grove_discovery_has_five_truthful_entities() {
    const auto& contract = atmosmesh::mqtt_grove_contract();
    TEST_ASSERT_EQUAL_INT(5, static_cast<int>(atmosmesh::mqtt_discovery_config_count(contract)));
    bool saw_light = false;
    bool saw_soil = false;
    bool saw_temperature = false;
    bool saw_humidity = false;
    bool saw_pressure = false;
    for (std::size_t i = 0; i < atmosmesh::mqtt_discovery_config_count(contract); ++i) {
        const auto cfg = atmosmesh::mqtt_discovery_config_at(contract, i);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("atmosmesh_grove_0001"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(contract.state_topic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(contract.availability_topic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("atmosmesh-grove-0001_"));
        TEST_ASSERT_LESS_THAN_UINT32(768U, static_cast<std::uint32_t>(cfg.payload.size()));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("lux"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("illuminance"));
        if (std::string(cfg.object_id) == "temperature_c") {
            saw_temperature = true;
        } else if (std::string(cfg.object_id) == "humidity_pct") {
            saw_humidity = true;
        } else if (std::string(cfg.object_id) == "pressure_hpa") {
            saw_pressure = true;
        }
        if (std::string(cfg.object_id) == "light_charge_us") {
            saw_light = true;
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("Uncalibrated RC"));
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("µs"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("device_class"));
        }
        if (std::string(cfg.object_id) == "soil_adc_raw") {
            saw_soil = true;
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("Soil Probe ADC Raw"));
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("ADC count"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("device_class"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("moisture"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("%"));
        }
    }
    TEST_ASSERT_TRUE(saw_light);
    TEST_ASSERT_TRUE(saw_soil);
    TEST_ASSERT_TRUE(saw_temperature);
    TEST_ASSERT_TRUE(saw_humidity);
    TEST_ASSERT_TRUE(saw_pressure);
}

void test_grove_session_replays_retained_discovery_and_availability_on_reconnect() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_use_contract(session, atmosmesh::mqtt_grove_contract());
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::GroveMqttState state{};
    state.temperature_c = {31.0F, true, 0};
    atmosmesh::mqtt_session_queue_payload(session, atmosmesh::grove_mqtt_state_json(state));

    auto actions = atmosmesh::mqtt_session_tick(session, 0);
    TEST_ASSERT_EQUAL_INT(7, static_cast<int>(actions.size()));
    for (std::size_t i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions[i].kind);
        TEST_ASSERT_TRUE(actions[i].retained);
    }
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline,
                      actions[5].kind);
    TEST_ASSERT_TRUE(actions[5].retained);
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishState, actions[6].kind);
    TEST_ASSERT_FALSE(actions[6].retained);
    atmosmesh::mqtt_session_note_publish_success(session, actions[6].kind);

    atmosmesh::mqtt_session_note_disconnect(session, 1000);
    atmosmesh::mqtt_session_note_connect(session);
    actions = atmosmesh::mqtt_session_tick(session, 2000);
    TEST_ASSERT_EQUAL_INT(6, static_cast<int>(actions.size()));
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions.front().kind);
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline,
                      actions.back().kind);
}

void test_pending_state_survives_failure_before_state_publish_and_clears_only_on_success() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_use_contract(session, atmosmesh::mqtt_grove_contract());
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::GroveMqttState state{};
    state.temperature_c = {22.0F, true, 0};
    state.soil_adc_raw = {0.0F, true, 0};
    atmosmesh::mqtt_session_queue_payload(session, atmosmesh::grove_mqtt_state_json(state));

    auto actions = atmosmesh::mqtt_session_tick(session, 0);
    TEST_ASSERT_TRUE(session.state_pending);
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions.front().kind);

    // Transport fails on discovery before it reaches the state action.
    atmosmesh::mqtt_session_note_disconnect(session, 10);
    TEST_ASSERT_TRUE(session.state_pending);
    atmosmesh::mqtt_session_note_connect(session);
    actions = atmosmesh::mqtt_session_tick(session, 20);
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishState, actions.back().kind);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          actions.back().payload.find("\"soil_adc_raw\":0"));
    TEST_ASSERT_TRUE(session.state_pending);

    atmosmesh::mqtt_session_note_publish_success(
        session, atmosmesh::MqttSessionActionKind::PublishState);
    TEST_ASSERT_FALSE(session.state_pending);
    TEST_ASSERT_TRUE(session.pending_state_payload.empty());
}

void test_aqua_contract_uses_stable_ids_and_separate_topics() {
    const auto& contract = atmosmesh::mqtt_aqua_contract();
    TEST_ASSERT_EQUAL_STRING("atmosmesh-aqua-v1", contract.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-aqua-0001", contract.station_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh_aqua_0001", contract.discovery_node);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-aqua-0001/state", contract.state_topic);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-aqua-0001/availability",
                             contract.availability_topic);
}

void test_aqua_will_is_retained_offline_on_product_availability_topic() {
    const auto will = atmosmesh::mqtt_will_config(atmosmesh::mqtt_aqua_contract());
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-aqua-0001/availability", will.topic);
    TEST_ASSERT_EQUAL_STRING("offline", will.payload);
    TEST_ASSERT_EQUAL_INT(0, will.qos);
    TEST_ASSERT_TRUE(will.retained);
}

void test_aqua_state_omits_missing_readings_and_never_carries_pressure() {
    atmosmesh::AquaMqttState state{};
    state.temperature_c = {0.0F, false, 0};
    state.humidity_pct = {0.0F, false, 0};
    state.water_adc_raw = {0.0F, false, 0};

    std::string json = atmosmesh::aqua_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"station_id\":\"atmosmesh-aqua-0001\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"product_id\":\"atmosmesh-aqua-v1\""));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("temperature_c\":"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("humidity_pct\":"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("water_adc_raw\":"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("pressure"));

    state.temperature_c = {23.4F, true, 0};
    state.humidity_pct = {48.0F, true, 0};
    state.water_adc_raw = {0.0F, true, 0};
    json = atmosmesh::aqua_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"temperature_c\":23.4"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"humidity_pct\":48.0"));
    // A raw zero reading is a valid measurement, not a missing one.
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"water_adc_raw\":0"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("pressure"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("moisture"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("level"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"level\""));
}

void test_aqua_discovery_has_exactly_three_entities_and_no_pressure() {
    const auto& contract = atmosmesh::mqtt_aqua_contract();
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(atmosmesh::mqtt_discovery_config_count(contract)));
    bool saw_temperature = false;
    bool saw_humidity = false;
    bool saw_water = false;
    for (std::size_t i = 0; i < atmosmesh::mqtt_discovery_config_count(contract); ++i) {
        const auto cfg = atmosmesh::mqtt_discovery_config_at(contract, i);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("atmosmesh_aqua_0001"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(contract.state_topic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find(contract.availability_topic));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("atmosmesh-aqua-0001_"));
        TEST_ASSERT_LESS_THAN_UINT32(768U, static_cast<std::uint32_t>(cfg.payload.size()));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("pressure"));
        if (std::string(cfg.object_id) == "temperature_c") {
            saw_temperature = true;
        } else if (std::string(cfg.object_id) == "humidity_pct") {
            saw_humidity = true;
        } else if (std::string(cfg.object_id) == "water_adc_raw") {
            saw_water = true;
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("Water Probe ADC Raw"));
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("ADC count"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("device_class"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("moisture"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("level"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("%"));
        }
    }
    TEST_ASSERT_TRUE(saw_temperature);
    TEST_ASSERT_TRUE(saw_humidity);
    TEST_ASSERT_TRUE(saw_water);
}

void test_aqua_session_replays_retained_discovery_and_availability_on_reconnect() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_use_contract(session, atmosmesh::mqtt_aqua_contract());
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::AquaMqttState state{};
    state.temperature_c = {21.0F, true, 0};
    atmosmesh::mqtt_session_queue_payload(session, atmosmesh::aqua_mqtt_state_json(state));

    auto actions = atmosmesh::mqtt_session_tick(session, 0);
    TEST_ASSERT_EQUAL_INT(5, static_cast<int>(actions.size()));
    for (std::size_t i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions[i].kind);
        TEST_ASSERT_TRUE(actions[i].retained);
    }
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline,
                      actions[3].kind);
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishState, actions[4].kind);
    TEST_ASSERT_FALSE(actions[4].retained);
}

void test_room_contract_uses_stable_ids_and_separate_topics() {
    const auto& contract = atmosmesh::mqtt_room_contract();
    TEST_ASSERT_EQUAL(atmosmesh::MqttProductKind::AtmosMeshRoomV1, contract.kind);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-room-v1", contract.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-room-0001", contract.station_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh_room_0001", contract.discovery_node);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-room-0001/state", contract.state_topic);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-room-0001/availability",
                             contract.availability_topic);
    // Sharing a topic with another station would let two boards overwrite each other's readings.
    TEST_ASSERT_NOT_EQUAL(0, std::string(contract.state_topic)
                                 .compare(atmosmesh::mqtt_v1_contract().state_topic));
    TEST_ASSERT_NOT_EQUAL(0, std::string(contract.state_topic)
                                 .compare(atmosmesh::mqtt_aqua_contract().state_topic));
    TEST_ASSERT_FALSE(atmosmesh::mqtt_payload_mentions_forbidden_room(contract.state_topic));
    TEST_ASSERT_FALSE(atmosmesh::mqtt_payload_mentions_forbidden_gas_label(contract.state_topic));
}

void test_room_will_is_retained_offline_on_product_availability_topic() {
    const auto will = atmosmesh::mqtt_will_config(atmosmesh::mqtt_room_contract());
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-room-0001/availability", will.topic);
    TEST_ASSERT_EQUAL_STRING("offline", will.payload);
    TEST_ASSERT_TRUE(will.retained);
    // The v1 topic here would mark a different station dead whenever the room board dropped.
    TEST_ASSERT_NOT_EQUAL(0, std::string(will.topic).compare(atmosmesh::kMqttAvailabilityTopic));
}

void test_room_state_marks_missing_particulates_invalid_rather_than_clean_air() {
    atmosmesh::RoomMqttState state{};
    state.temperature_c = {21.5F, true, 100};
    state.humidity_pct = {44.0F, true, 100};
    state.illuminance_lx = {318.0F, true, 100};
    state.pm25.valid = false;   // sensor silent
    state.pm25.age_ms = 9000;
    state.pm10.valid = false;
    state.pm10.age_ms = 9000;
    state.motion = {false, true, 45000};
    state.pm_alarm = {false, false, 9000};

    const std::string json = atmosmesh::room_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          json.find("\"station_id\":\"atmosmesh-room-0001\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          json.find("\"product_id\":\"atmosmesh-room-v1\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"value\":21.5"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"lx\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"ug/m3\""));
    // A silent SDS011 carries no value at all. A zero here would read in Home Assistant as
    // perfectly clean air, which is the opposite of what is known.
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"pm25\":{\"value\""));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"pm10\":{\"value\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"pm25\":{\"unit\":\"ug/m3\",\"valid\":false"));
    // Motion is a measured false here, so it does carry a value.
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"motion\":{\"value\":false"));
    // The alarm latch has never seen a sample, so it carries none.
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"pm_alarm\":{\"valid\":false"));
}

void test_room_discovery_is_the_room_entity_set_not_the_v1_fallthrough() {
    const auto& contract = atmosmesh::mqtt_room_contract();
    const std::size_t count = atmosmesh::mqtt_discovery_config_count(contract);
    TEST_ASSERT_EQUAL_INT(7, static_cast<int>(count));

    bool saw_illuminance = false;
    bool saw_motion = false;
    bool saw_pm_alarm = false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto cfg = atmosmesh::mqtt_discovery_config_at(contract, i);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("atmosmesh_room_0001"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos,
                              cfg.payload.find("home/air/atmosmesh-room-0001/state"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos,
                              cfg.payload.find("\"unique_id\":\"atmosmesh-room-0001_"));
        // The v1 spec list leaked here before discovery_specs() became an exhaustive switch.
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("bmp_temperature"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("gas_index"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("\"hPa\""));
        if (std::string(cfg.object_id) == "illuminance_lx") {
            saw_illuminance = true;
            TEST_ASSERT_EQUAL_STRING("sensor", cfg.component);
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"illuminance\""));
        }
        if (std::string(cfg.object_id) == "motion") {
            saw_motion = true;
            TEST_ASSERT_EQUAL_STRING("binary_sensor", cfg.component);
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"occupancy\""));
        }
        if (std::string(cfg.object_id) == "pm_alarm") {
            saw_pm_alarm = true;
            TEST_ASSERT_EQUAL_STRING("binary_sensor", cfg.component);
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"problem\""));
        }
        // Every room template must go unavailable rather than report a number, on invalid.
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("valid"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("none"));
    }
    TEST_ASSERT_TRUE(saw_illuminance);
    TEST_ASSERT_TRUE(saw_motion);
    TEST_ASSERT_TRUE(saw_pm_alarm);
}

void test_room_session_replays_retained_discovery_and_availability_on_reconnect() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_use_contract(session, atmosmesh::mqtt_room_contract());
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::RoomMqttState state{};
    state.temperature_c = {21.0F, true, 0};
    atmosmesh::mqtt_session_queue_payload(session, atmosmesh::room_mqtt_state_json(state));

    auto actions = atmosmesh::mqtt_session_tick(session, 0);
    TEST_ASSERT_EQUAL_INT(9, static_cast<int>(actions.size()));
    for (std::size_t i = 0; i < 7; ++i) {
        TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions[i].kind);
        TEST_ASSERT_TRUE(actions[i].retained);
    }
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline,
                      actions[7].kind);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-room-0001/availability", actions[7].topic.c_str());
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishState, actions[8].kind);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-room-0001/state", actions[8].topic.c_str());
    TEST_ASSERT_FALSE(actions[8].retained);
}

void test_spot_contract_uses_stable_ids_and_separate_topics() {
    const auto& contract = atmosmesh::mqtt_spot_contract();
    TEST_ASSERT_EQUAL(atmosmesh::MqttProductKind::AtmosMeshSpotV1, contract.kind);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-spot-v1", contract.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-spot-0001", contract.station_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh_spot_0001", contract.discovery_node);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-spot-0001/state", contract.state_topic);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-spot-0001/availability",
                             contract.availability_topic);
    TEST_ASSERT_NOT_EQUAL(0, std::string(contract.state_topic)
                                 .compare(atmosmesh::mqtt_room_contract().state_topic));
    TEST_ASSERT_NOT_EQUAL(0, std::string(contract.state_topic)
                                 .compare(atmosmesh::mqtt_v1_contract().state_topic));
    TEST_ASSERT_FALSE(atmosmesh::mqtt_payload_mentions_forbidden_room(contract.state_topic));
    const auto will = atmosmesh::mqtt_will_config(contract);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-spot-0001/availability", will.topic);
    TEST_ASSERT_EQUAL_STRING("offline", will.payload);
    TEST_ASSERT_TRUE(will.retained);
}

void test_spot_state_marks_missing_probe_and_radar_invalid_rather_than_zero() {
    atmosmesh::SpotMqttState state{};
    state.temperature_c = {21.5F, true, 100};
    state.humidity_pct = {44.0F, true, 100};
    state.illuminance_lx = {318.0F, true, 100};
    state.probe_temperature_c.valid = false;   // probe unplugged
    state.probe_temperature_c.age_ms = 12000;
    state.presence_distance_cm.valid = false;  // no UART frame lately
    state.presence_state.valid = false;
    state.wifi_rssi_dbm = {-61.0F, true, 0};
    state.presence = {true, true, 3000};

    const std::string json = atmosmesh::spot_mqtt_state_json(state);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          json.find("\"station_id\":\"atmosmesh-spot-0001\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"product_id\":\"atmosmesh-spot-v1\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"temperature_c\":{\"value\":21.5"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"unit\":\"lx\""));
    // An unplugged probe carries no value: 0.0 C would read as a frozen radiator.
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"probe_temperature_c\":{\"value\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          json.find("\"probe_temperature_c\":{\"unit\":\"C\",\"valid\":false"));
    // A silent UART is no distance, not a target at 0 cm.
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"presence_distance_cm\":{\"value\""));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("\"presence_state\":{\"value\""));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"wifi_rssi_dbm\":{\"value\":-61.0"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"presence\":{\"value\":true"));
    // No particulates, no pressure, no gas: the Spot has none of those sensors.
    TEST_ASSERT_EQUAL(std::string::npos, json.find("pm25"));
    TEST_ASSERT_EQUAL(std::string::npos, json.find("hPa"));
    assert_no_room_or_forbidden_gas(json);
}

void test_spot_discovery_is_the_spot_entity_set() {
    const auto& contract = atmosmesh::mqtt_spot_contract();
    const std::size_t count = atmosmesh::mqtt_discovery_config_count(contract);
    TEST_ASSERT_EQUAL_INT(8, static_cast<int>(count));

    bool saw_probe = false;
    bool saw_distance = false;
    bool saw_state = false;
    bool saw_presence = false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto cfg = atmosmesh::mqtt_discovery_config_at(contract, i);
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.topic.find("atmosmesh_spot_0001"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos,
                              cfg.payload.find("home/air/atmosmesh-spot-0001/state"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos,
                              cfg.payload.find("\"unique_id\":\"atmosmesh-spot-0001_"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("pm25"));
        TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("\"hPa\""));
        if (std::string(cfg.object_id) == "probe_temperature_c") {
            saw_probe = true;
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"temperature\""));
            TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("Probe Temperature"));
        }
        if (std::string(cfg.object_id) == "presence_distance_cm") {
            saw_distance = true;
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"distance\""));
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"unit_of_measurement\":\"cm\""));
        }
        if (std::string(cfg.object_id) == "presence_state") {
            saw_state = true;
            TEST_ASSERT_EQUAL_STRING("sensor", cfg.component);
            // Raw 0..3 from the radar: no device class, no unit, no invented semantics.
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("device_class"));
            TEST_ASSERT_EQUAL(std::string::npos, cfg.payload.find("unit_of_measurement"));
        }
        if (std::string(cfg.object_id) == "presence") {
            saw_presence = true;
            TEST_ASSERT_EQUAL_STRING("binary_sensor", cfg.component);
            TEST_ASSERT_NOT_EQUAL(std::string::npos,
                                  cfg.payload.find("\"device_class\":\"occupancy\""));
        }
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("valid"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, cfg.payload.find("none"));
        assert_no_room_or_forbidden_gas(cfg.payload);
    }
    TEST_ASSERT_TRUE(saw_probe);
    TEST_ASSERT_TRUE(saw_distance);
    TEST_ASSERT_TRUE(saw_state);
    TEST_ASSERT_TRUE(saw_presence);
}

void test_spot_session_replays_retained_discovery_and_availability_on_reconnect() {
    atmosmesh::MqttSession session{};
    atmosmesh::mqtt_session_use_contract(session, atmosmesh::mqtt_spot_contract());
    atmosmesh::mqtt_session_note_connect(session);
    atmosmesh::SpotMqttState state{};
    state.temperature_c = {21.0F, true, 0};
    atmosmesh::mqtt_session_queue_payload(session, atmosmesh::spot_mqtt_state_json(state));

    auto actions = atmosmesh::mqtt_session_tick(session, 0);
    TEST_ASSERT_EQUAL_INT(10, static_cast<int>(actions.size()));
    for (std::size_t i = 0; i < 8; ++i) {
        TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishDiscovery, actions[i].kind);
        TEST_ASSERT_TRUE(actions[i].retained);
    }
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishAvailabilityOnline,
                      actions[8].kind);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-spot-0001/availability", actions[8].topic.c_str());
    TEST_ASSERT_EQUAL(atmosmesh::MqttSessionActionKind::PublishState, actions[9].kind);
    TEST_ASSERT_EQUAL_STRING("home/air/atmosmesh-spot-0001/state", actions[9].topic.c_str());
    TEST_ASSERT_FALSE(actions[9].retained);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_ids_and_topics_are_station_not_room);
    RUN_TEST(test_state_json_includes_identity_units_and_omits_invalid_values);
    RUN_TEST(test_discovery_configs_cover_sensors_without_lux);
    RUN_TEST(test_backoff_doubles_until_cap);
    RUN_TEST(test_session_does_not_publish_when_disconnected);
    RUN_TEST(test_session_publishes_discovery_online_then_state_on_connect);
    RUN_TEST(test_esp_publish_message_id_clears_only_successfully_queued_state);
    RUN_TEST(test_session_reconnect_backoff_gates_attempts);
    RUN_TEST(test_grove_contract_uses_stable_ids_and_separate_topics);
    RUN_TEST(test_grove_will_is_retained_offline_on_product_availability_topic);
    RUN_TEST(test_grove_state_omits_missing_light_but_preserves_valid_zero_environment);
    RUN_TEST(test_grove_discovery_has_five_truthful_entities);
    RUN_TEST(test_grove_session_replays_retained_discovery_and_availability_on_reconnect);
    RUN_TEST(test_pending_state_survives_failure_before_state_publish_and_clears_only_on_success);
    RUN_TEST(test_aqua_contract_uses_stable_ids_and_separate_topics);
    RUN_TEST(test_aqua_will_is_retained_offline_on_product_availability_topic);
    RUN_TEST(test_aqua_state_omits_missing_readings_and_never_carries_pressure);
    RUN_TEST(test_aqua_discovery_has_exactly_three_entities_and_no_pressure);
    RUN_TEST(test_aqua_session_replays_retained_discovery_and_availability_on_reconnect);
    RUN_TEST(test_room_contract_uses_stable_ids_and_separate_topics);
    RUN_TEST(test_room_will_is_retained_offline_on_product_availability_topic);
    RUN_TEST(test_room_state_marks_missing_particulates_invalid_rather_than_clean_air);
    RUN_TEST(test_room_discovery_is_the_room_entity_set_not_the_v1_fallthrough);
    RUN_TEST(test_room_session_replays_retained_discovery_and_availability_on_reconnect);
    RUN_TEST(test_spot_contract_uses_stable_ids_and_separate_topics);
    RUN_TEST(test_spot_state_marks_missing_probe_and_radar_invalid_rather_than_zero);
    RUN_TEST(test_spot_discovery_is_the_spot_entity_set);
    RUN_TEST(test_spot_session_replays_retained_discovery_and_availability_on_reconnect);
    return UNITY_END();
}

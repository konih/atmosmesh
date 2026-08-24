#include <cstring>
#include <string>

#include <unity.h>

#include "atmosmesh/grove_status.hpp"
#include "atmosmesh/grove_mqtt_runtime.hpp"
#include "atmosmesh/product_profile.hpp"
#include "atmosmesh/rc_light.hpp"
#include "atmosmesh/soil_sampler.hpp"
#include "atmosmesh/status_led.hpp"

void test_atmosmesh_v1_profile_has_stable_identity_and_existing_pins() {
    const auto& profile = atmosmesh::atmosmesh_v1_profile();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh", profile.product_name);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1", profile.product_id);
    TEST_ASSERT_EQUAL_STRING("esp32-full-station", profile.product_variant);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-0001", profile.station_id);
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_id, profile.product_variant));
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_id, profile.station_id));
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_variant, profile.station_id));
    TEST_ASSERT_EQUAL_INT(5, profile.i2c_sda_gpio);
    TEST_ASSERT_EQUAL_INT(4, profile.i2c_scl_gpio);
    TEST_ASSERT_TRUE(profile.i2c_sda_is_bootstrap);
    TEST_ASSERT_FALSE(profile.i2c_scl_is_bootstrap);
    TEST_ASSERT_EQUAL_INT(18, profile.dht_data_gpio);
    TEST_ASSERT_EQUAL_INT(128, profile.oled_width_px);
    TEST_ASSERT_EQUAL_INT(64, profile.oled_height_px);
    TEST_ASSERT_EQUAL_INT(-1, profile.light_rc_gpio);
    TEST_ASSERT_EQUAL_INT(-1, profile.status_led_red_gpio);
    TEST_ASSERT_EQUAL_INT(-1, profile.status_led_green_gpio);
    TEST_ASSERT_EQUAL_INT(-1, profile.soil_power_control_gpio);
}

void test_grove_profile_is_id_based_and_matches_wiring() {
    const auto& profile = atmosmesh::grove_profile();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh Grove", profile.product_name);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-v1.5", profile.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1.5", profile.product_variant);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-0001", profile.station_id);
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_id, profile.product_variant));
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_id, profile.station_id));
    TEST_ASSERT_NOT_EQUAL(0, std::strcmp(profile.product_variant, profile.station_id));
    TEST_ASSERT_EQUAL_INT(4, profile.i2c_sda_gpio);
    TEST_ASSERT_EQUAL_INT(0, profile.i2c_scl_gpio);
    TEST_ASSERT_FALSE(profile.i2c_sda_is_bootstrap);
    TEST_ASSERT_TRUE(profile.i2c_scl_is_bootstrap);
    TEST_ASSERT_EQUAL_INT(14, profile.dht_data_gpio);
    TEST_ASSERT_EQUAL_INT(128, profile.oled_width_px);
    TEST_ASSERT_EQUAL_INT(32, profile.oled_height_px);
    TEST_ASSERT_EQUAL_INT(13, profile.light_rc_gpio);
    TEST_ASSERT_EQUAL_INT(12, profile.status_led_red_gpio);
    TEST_ASSERT_EQUAL_INT(16, profile.status_led_green_gpio);
    TEST_ASSERT_EQUAL_INT(5, profile.soil_power_control_gpio);
    TEST_ASSERT_FALSE(profile.status_led_common_anode);
}

void test_grove_page_formats_valid_measurements() {
    atmosmesh::GroveReadings readings{};
    readings.dht_temperature = {true, 23.4F};
    readings.humidity = {true, 48.1F};
    readings.bmp_temperature = {true, 22.8F};
    readings.pressure = {true, 1013.2F};

    const auto lines = atmosmesh::grove_oled_lines(readings);
    TEST_ASSERT_EQUAL_STRING("AtmosMesh Grove", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("T 23.4C  H 48%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("P 1013.2 hPa", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("L-----us S---- D1B1", lines[3].c_str());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(21, static_cast<int>(line.size()));
    }
}

void test_grove_page_never_turns_missing_into_zero() {
    const auto lines = atmosmesh::grove_oled_lines({});
    TEST_ASSERT_EQUAL_STRING("T --.-C  H --%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("P ----.- hPa", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("L-----us S---- D0B0", lines[3].c_str());
    TEST_ASSERT_EQUAL(std::string::npos, lines[1].find("0.0"));
    TEST_ASSERT_EQUAL(std::string::npos, lines[2].find("0.0"));
}

void test_grove_partial_failure_is_explicit() {
    atmosmesh::GroveReadings readings{};
    readings.dht_temperature = {true, 0.0F};
    readings.humidity = {true, 0.0F};
    const auto lines = atmosmesh::grove_oled_lines(readings);
    TEST_ASSERT_EQUAL_STRING("T 0.0C  H 0%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("P ----.- hPa", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("L-----us S---- D1B0", lines[3].c_str());
    TEST_ASSERT_EQUAL_STRING("dht=ok bmp=error", atmosmesh::grove_health_text(readings).c_str());
}

void test_grove_bmp_presence_rejects_cached_plausible_values_and_recovers() {
    atmosmesh::GroveReadings readings{};
    readings.bmp_temperature = {true, 22.8F};
    readings.pressure = {true, 1013.2F};

    TEST_ASSERT_EQUAL(atmosmesh::GroveBmpAction::Unavailable,
                      atmosmesh::grove_bmp_action(false, true));
    atmosmesh::invalidate_grove_bmp(readings);
    TEST_ASSERT_FALSE(readings.bmp_temperature.valid);
    TEST_ASSERT_FALSE(readings.pressure.valid);
    TEST_ASSERT_EQUAL_STRING("L-----us S---- D0B0",
                             atmosmesh::grove_oled_lines(readings)[3].c_str());
    TEST_ASSERT_EQUAL(atmosmesh::GroveBmpAction::Initialize,
                      atmosmesh::grove_bmp_action(true, false));
    TEST_ASSERT_EQUAL(atmosmesh::GroveBmpAction::Read,
                      atmosmesh::grove_bmp_action(true, true));
}

void test_rc_light_policy_is_cooperative_and_reports_microseconds() {
    atmosmesh::RcLightState state{};
    auto step = atmosmesh::rc_light_begin(state, 100U);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightPinAction::DriveLow, step.pin_action);
    TEST_ASSERT_FALSE(step.completed);

    step = atmosmesh::rc_light_tick(state, 100U + atmosmesh::kRcLightDischargeUs - 1U, false);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightPinAction::None, step.pin_action);
    TEST_ASSERT_FALSE(step.completed);

    const std::uint32_t release_us = 100U + atmosmesh::kRcLightDischargeUs;
    step = atmosmesh::rc_light_tick(state, release_us, false);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightPinAction::ReleaseInput, step.pin_action);
    TEST_ASSERT_FALSE(step.completed);

    step = atmosmesh::rc_light_tick(state, release_us + 127U, true);
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightStatus::Valid, state.status);
    TEST_ASSERT_TRUE(state.measurement.valid);
    TEST_ASSERT_EQUAL_UINT32(127U, state.measurement.charge_us);
    TEST_ASSERT_EQUAL_STRING("light: ok charge=127 us (raw; lower=brighter)",
                             atmosmesh::rc_light_serial_text(state).c_str());
}

void test_rc_light_immediate_high_is_saturated_not_zero() {
    atmosmesh::RcLightState state{};
    atmosmesh::rc_light_begin(state, 0U);
    const auto release =
        atmosmesh::rc_light_tick(state, atmosmesh::kRcLightDischargeUs, false);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightPinAction::ReleaseInput, release.pin_action);
    // The hardware caller samples D7 synchronously after applying ReleaseInput. A line that is
    // already HIGH at that point is saturated even though a later loop tick cannot have elapsed=0.
    const auto step = atmosmesh::rc_light_note_released_level(state, true);
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightStatus::Saturated, state.status);
    TEST_ASSERT_FALSE(state.measurement.valid);
    TEST_ASSERT_EQUAL_UINT32(0U, state.measurement.charge_us);
    TEST_ASSERT_EQUAL_STRING("light: unavailable saturated/immediate",
                             atmosmesh::rc_light_serial_text(state).c_str());
}

void test_grove_dns_and_tcp_share_one_bounded_connect_budget() {
    TEST_ASSERT_EQUAL_UINT32(1000U, atmosmesh::kGroveMqttTransportConnectBudgetMs);
    TEST_ASSERT_EQUAL_UINT32(
        750U, atmosmesh::grove_mqtt_connect_budget_remaining_ms(1000U, 1250U));
    TEST_ASSERT_EQUAL_UINT32(
        0U, atmosmesh::grove_mqtt_connect_budget_remaining_ms(1000U, 2000U));
    TEST_ASSERT_EQUAL_UINT32(
        0U, atmosmesh::grove_mqtt_connect_budget_remaining_ms(1000U, 2500U));
    TEST_ASSERT_TRUE(atmosmesh::grove_network_work_allowed(false, false));
    TEST_ASSERT_FALSE(atmosmesh::grove_network_work_allowed(true, false));
    TEST_ASSERT_FALSE(atmosmesh::grove_network_work_allowed(false, true));
}

void test_rc_light_timeout_is_unavailable_and_never_zero() {
    atmosmesh::RcLightState state{};
    atmosmesh::rc_light_begin(state, 1000U);
    const std::uint32_t release_us = 1000U + atmosmesh::kRcLightDischargeUs;
    atmosmesh::rc_light_tick(state, release_us, false);
    const auto step =
        atmosmesh::rc_light_tick(state, release_us + atmosmesh::kRcLightTimeoutUs, false);
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightStatus::Timeout, state.status);
    TEST_ASSERT_FALSE(state.measurement.valid);
    TEST_ASSERT_EQUAL_UINT32(0U, state.measurement.charge_us);
    TEST_ASSERT_EQUAL_STRING("light: unavailable timeout/disconnected",
                             atmosmesh::rc_light_serial_text(state).c_str());
}

void test_rc_light_rejects_high_first_seen_after_hard_timeout() {
    atmosmesh::RcLightState state{};
    atmosmesh::rc_light_begin(state, 0U);
    const std::uint32_t release_us = atmosmesh::kRcLightDischargeUs;
    atmosmesh::rc_light_tick(state, release_us, false);
    const auto step =
        atmosmesh::rc_light_tick(state, release_us + atmosmesh::kRcLightTimeoutUs + 1U, true);
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::RcLightStatus::Timeout, state.status);
    TEST_ASSERT_FALSE(state.measurement.valid);
}

void test_grove_oled_shows_raw_light_or_explicit_missing() {
    atmosmesh::GroveReadings readings{};
    readings.light = {true, 4321U};
    readings.soil = {true, 512U};
    TEST_ASSERT_EQUAL_STRING("L4321us S512 D0B0",
                             atmosmesh::grove_oled_lines(readings)[3].c_str());
    readings.light = {};
    readings.soil = {};
    TEST_ASSERT_EQUAL_STRING("L-----us S---- D0B0",
                             atmosmesh::grove_oled_lines(readings)[3].c_str());
}

void test_status_led_maps_health_and_polarity_deterministically() {
    using atmosmesh::GroveLedStatus;
    using atmosmesh::LedPolarity;

    TEST_ASSERT_EQUAL(GroveLedStatus::SensorFault,
                      atmosmesh::grove_led_status(false, false, false));
    TEST_ASSERT_EQUAL(GroveLedStatus::SensorFault,
                      atmosmesh::grove_led_status(true, true, true));
    TEST_ASSERT_EQUAL(GroveLedStatus::LocalOnly,
                      atmosmesh::grove_led_status(true, false, false));
    TEST_ASSERT_EQUAL(GroveLedStatus::Healthy,
                      atmosmesh::grove_led_status(true, false, true));

    auto levels = atmosmesh::grove_led_pin_levels(GroveLedStatus::SensorFault,
                                                   LedPolarity::CommonCathode);
    TEST_ASSERT_TRUE(levels.red_high);
    TEST_ASSERT_FALSE(levels.green_high);
    levels = atmosmesh::grove_led_pin_levels(GroveLedStatus::LocalOnly,
                                              LedPolarity::CommonCathode);
    TEST_ASSERT_TRUE(levels.red_high);
    TEST_ASSERT_TRUE(levels.green_high);
    levels = atmosmesh::grove_led_pin_levels(GroveLedStatus::Healthy,
                                              LedPolarity::CommonCathode);
    TEST_ASSERT_FALSE(levels.red_high);
    TEST_ASSERT_TRUE(levels.green_high);

    levels = atmosmesh::grove_led_pin_levels(GroveLedStatus::Healthy,
                                              LedPolarity::CommonAnode);
    TEST_ASSERT_TRUE(levels.red_high);
    TEST_ASSERT_FALSE(levels.green_high);
}

void test_soil_sampler_is_cooperative_bounded_and_accepts_raw_zero() {
    atmosmesh::SoilSamplerState state{};
    auto step = atmosmesh::soil_sampler_begin(state, 1000U);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::EnsureOff, step.power_action);
    TEST_ASSERT_FALSE(state.measurement.valid);
    TEST_ASSERT_FALSE(atmosmesh::soil_sampler_power_active(state));

    step = atmosmesh::soil_sampler_tick(state, 1000U + atmosmesh::kSoilSampleIntervalMs - 1U);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::None, step.power_action);
    step = atmosmesh::soil_sampler_tick(state, 1000U + atmosmesh::kSoilSampleIntervalMs);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOn, step.power_action);
    TEST_ASSERT_TRUE(atmosmesh::soil_sampler_power_active(state));

    const std::uint32_t first_sample_ms =
        1000U + atmosmesh::kSoilSampleIntervalMs + atmosmesh::kSoilSettleMs;
    step = atmosmesh::soil_sampler_tick(state, first_sample_ms - 1U);
    TEST_ASSERT_FALSE(step.sample_adc);
    for (std::uint16_t i = 0; i < atmosmesh::kSoilSampleCount; ++i) {
        const std::uint32_t sample_ms =
            first_sample_ms + i * atmosmesh::kSoilSampleSpacingMs;
        step = atmosmesh::soil_sampler_tick(state, sample_ms);
        TEST_ASSERT_TRUE(step.sample_adc);
        step = atmosmesh::soil_sampler_record_sample(state, sample_ms, 0);
    }
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOff, step.power_action);
    TEST_ASSERT_FALSE(atmosmesh::soil_sampler_power_active(state));
    TEST_ASSERT_TRUE(state.measurement.valid);
    TEST_ASSERT_EQUAL_UINT16(0U, state.measurement.raw);
    TEST_ASSERT_EQUAL_STRING("soil: ok adc_raw=0 samples=5 power=off",
                             atmosmesh::soil_sampler_serial_text(state).c_str());
    TEST_ASSERT_EQUAL_UINT16(5U, atmosmesh::kSoilSampleCount);
    TEST_ASSERT_EQUAL_UINT32(120U, atmosmesh::kSoilNormalPowerOnMs);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(9U, atmosmesh::kSoilMaxDutyPermille);
}

void test_soil_sampler_averages_and_fails_off_at_hard_timeout() {
    atmosmesh::SoilSamplerState state{};
    atmosmesh::soil_sampler_begin(state, 0U);
    atmosmesh::soil_sampler_tick(state, atmosmesh::kSoilSampleIntervalMs);
    const std::uint32_t first = atmosmesh::kSoilSampleIntervalMs + atmosmesh::kSoilSettleMs;
    const int samples[] = {0, 100, 200, 300, 400};
    atmosmesh::SoilSamplerStep step{};
    for (std::uint16_t i = 0; i < atmosmesh::kSoilSampleCount; ++i) {
        const auto now = first + i * atmosmesh::kSoilSampleSpacingMs;
        TEST_ASSERT_TRUE(atmosmesh::soil_sampler_tick(state, now).sample_adc);
        step = atmosmesh::soil_sampler_record_sample(state, now, samples[i]);
    }
    TEST_ASSERT_TRUE(state.measurement.valid);
    TEST_ASSERT_EQUAL_UINT16(200U, state.measurement.raw);

    atmosmesh::soil_sampler_tick(state, 2U * atmosmesh::kSoilSampleIntervalMs);
    step = atmosmesh::soil_sampler_tick(
        state, 2U * atmosmesh::kSoilSampleIntervalMs + atmosmesh::kSoilMaxPowerOnMs);
    TEST_ASSERT_TRUE(step.completed);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOff, step.power_action);
    TEST_ASSERT_TRUE(atmosmesh::soil_sampler_acquisition_failed(state));
    TEST_ASSERT_FALSE(state.measurement.valid);
    TEST_ASSERT_EQUAL_STRING("soil: unavailable acquisition-timeout power=off",
                             atmosmesh::soil_sampler_serial_text(state).c_str());
    TEST_ASSERT_TRUE(atmosmesh::soil_power_pin_high(atmosmesh::SoilPowerAction::PowerOff));
    TEST_ASSERT_FALSE(atmosmesh::soil_power_pin_high(atmosmesh::SoilPowerAction::PowerOn));

    atmosmesh::soil_sampler_begin(state, 0U);
    atmosmesh::soil_sampler_tick(state, atmosmesh::kSoilSampleIntervalMs);
    TEST_ASSERT_TRUE(
        atmosmesh::soil_sampler_tick(state, first).sample_adc);
    step = atmosmesh::soil_sampler_record_sample(state, first, -1);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOff, step.power_action);
    TEST_ASSERT_TRUE(atmosmesh::soil_sampler_acquisition_failed(state));
    TEST_ASSERT_EQUAL_STRING("soil: unavailable acquisition-failed power=off",
                             atmosmesh::soil_sampler_serial_text(state).c_str());
}

void test_soil_sampler_deadlines_are_millis_wraparound_safe() {
    atmosmesh::SoilSamplerState state{};
    constexpr std::uint32_t started = 0xFFFFF000U;
    atmosmesh::soil_sampler_begin(state, started);
    const std::uint32_t due = started + atmosmesh::kSoilSampleIntervalMs;
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::None,
                      atmosmesh::soil_sampler_tick(state, due - 1U).power_action);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOn,
                      atmosmesh::soil_sampler_tick(state, due).power_action);
    TEST_ASSERT_EQUAL(atmosmesh::SoilPowerAction::PowerOff,
                      atmosmesh::soil_sampler_tick(
                          state, due + atmosmesh::kSoilMaxPowerOnMs).power_action);
}

void register_product_variant_tests() {
    RUN_TEST(test_atmosmesh_v1_profile_has_stable_identity_and_existing_pins);
    RUN_TEST(test_grove_profile_is_id_based_and_matches_wiring);
    RUN_TEST(test_grove_page_formats_valid_measurements);
    RUN_TEST(test_grove_page_never_turns_missing_into_zero);
    RUN_TEST(test_grove_partial_failure_is_explicit);
    RUN_TEST(test_grove_bmp_presence_rejects_cached_plausible_values_and_recovers);
    RUN_TEST(test_rc_light_policy_is_cooperative_and_reports_microseconds);
    RUN_TEST(test_rc_light_immediate_high_is_saturated_not_zero);
    RUN_TEST(test_rc_light_timeout_is_unavailable_and_never_zero);
    RUN_TEST(test_rc_light_rejects_high_first_seen_after_hard_timeout);
    RUN_TEST(test_grove_dns_and_tcp_share_one_bounded_connect_budget);
    RUN_TEST(test_grove_oled_shows_raw_light_or_explicit_missing);
    RUN_TEST(test_status_led_maps_health_and_polarity_deterministically);
    RUN_TEST(test_soil_sampler_is_cooperative_bounded_and_accepts_raw_zero);
    RUN_TEST(test_soil_sampler_averages_and_fails_off_at_hard_timeout);
    RUN_TEST(test_soil_sampler_deadlines_are_millis_wraparound_safe);
}

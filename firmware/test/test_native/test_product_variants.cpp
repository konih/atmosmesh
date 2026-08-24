#include <string>

#include <unity.h>

#include "atmosmesh/grove_status.hpp"
#include "atmosmesh/product_profile.hpp"

void test_atmosmesh_v1_profile_has_stable_identity_and_existing_pins() {
    const auto& profile = atmosmesh::atmosmesh_v1_profile();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh", profile.product_name);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1", profile.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1", profile.product_variant);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-0001", profile.station_id);
    TEST_ASSERT_EQUAL_INT(5, profile.i2c_sda_gpio);
    TEST_ASSERT_EQUAL_INT(4, profile.i2c_scl_gpio);
    TEST_ASSERT_TRUE(profile.i2c_sda_is_bootstrap);
    TEST_ASSERT_FALSE(profile.i2c_scl_is_bootstrap);
    TEST_ASSERT_EQUAL_INT(18, profile.dht_data_gpio);
    TEST_ASSERT_EQUAL_INT(128, profile.oled_width_px);
    TEST_ASSERT_EQUAL_INT(64, profile.oled_height_px);
}

void test_grove_profile_is_id_based_and_matches_wiring() {
    const auto& profile = atmosmesh::grove_profile();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh Grove", profile.product_name);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-v1.5", profile.product_id);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-v1.5", profile.product_variant);
    TEST_ASSERT_EQUAL_STRING("atmosmesh-grove-0001", profile.station_id);
    TEST_ASSERT_EQUAL_INT(4, profile.i2c_sda_gpio);
    TEST_ASSERT_EQUAL_INT(0, profile.i2c_scl_gpio);
    TEST_ASSERT_FALSE(profile.i2c_sda_is_bootstrap);
    TEST_ASSERT_TRUE(profile.i2c_scl_is_bootstrap);
    TEST_ASSERT_EQUAL_INT(14, profile.dht_data_gpio);
    TEST_ASSERT_EQUAL_INT(128, profile.oled_width_px);
    TEST_ASSERT_EQUAL_INT(32, profile.oled_height_px);
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
    TEST_ASSERT_EQUAL_STRING("DHT OK BMP OK", lines[3].c_str());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(21, static_cast<int>(line.size()));
    }
}

void test_grove_page_never_turns_missing_into_zero() {
    const auto lines = atmosmesh::grove_oled_lines({});
    TEST_ASSERT_EQUAL_STRING("T --.-C  H --%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("P ----.- hPa", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("DHT ERR BMP ERR", lines[3].c_str());
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
    TEST_ASSERT_EQUAL_STRING("DHT OK BMP ERR", lines[3].c_str());
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
    TEST_ASSERT_EQUAL_STRING("DHT ERR BMP ERR",
                             atmosmesh::grove_oled_lines(readings)[3].c_str());
    TEST_ASSERT_EQUAL(atmosmesh::GroveBmpAction::Initialize,
                      atmosmesh::grove_bmp_action(true, false));
    TEST_ASSERT_EQUAL(atmosmesh::GroveBmpAction::Read,
                      atmosmesh::grove_bmp_action(true, true));
}

void register_product_variant_tests() {
    RUN_TEST(test_atmosmesh_v1_profile_has_stable_identity_and_existing_pins);
    RUN_TEST(test_grove_profile_is_id_based_and_matches_wiring);
    RUN_TEST(test_grove_page_formats_valid_measurements);
    RUN_TEST(test_grove_page_never_turns_missing_into_zero);
    RUN_TEST(test_grove_partial_failure_is_explicit);
    RUN_TEST(test_grove_bmp_presence_rejects_cached_plausible_values_and_recovers);
}

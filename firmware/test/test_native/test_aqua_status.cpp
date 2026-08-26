#include <string>

#include <unity.h>

#include "atmosmesh/aqua_status.hpp"

namespace {

void test_aqua_page_formats_valid_measurements() {
    atmosmesh::AquaReadings readings{};
    readings.temperature = {true, 23.4F};
    readings.humidity = {true, 48.1F};
    readings.water = {true, 512U};
    readings.mqtt_up = true;

    const auto lines = atmosmesh::aqua_oled_lines(readings);
    TEST_ASSERT_EQUAL_STRING("T:23.4C", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("RH:48%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("Water:512", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("mqtt:ok", lines[3].c_str());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(21, static_cast<int>(line.size()));
    }
}

void test_aqua_page_never_turns_missing_into_zero() {
    const auto lines = atmosmesh::aqua_oled_lines({});
    TEST_ASSERT_EQUAL_STRING("T:--", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("RH:--", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("Water:--", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("mqtt:off", lines[3].c_str());
}

void test_aqua_water_raw_zero_is_shown_not_hidden() {
    atmosmesh::AquaReadings readings{};
    readings.water = {true, 0U};
    const auto lines = atmosmesh::aqua_oled_lines(readings);
    TEST_ASSERT_EQUAL_STRING("Water:0", lines[2].c_str());
}

}  // namespace

void register_aqua_status_tests() {
    RUN_TEST(test_aqua_page_formats_valid_measurements);
    RUN_TEST(test_aqua_page_never_turns_missing_into_zero);
    RUN_TEST(test_aqua_water_raw_zero_is_shown_not_hidden);
}

#include <cstdint>
#include <string>

#include <unity.h>

#include "atmosmesh/am2302_frame.hpp"
#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/oled_address.hpp"
#include "atmosmesh/pins.hpp"

void test_clip_truncates_to_oled_width() {
    const std::string clipped = atmosmesh::clip_oled_line("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TEST_ASSERT_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(clipped.size()));
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOPQRSTU", clipped.c_str());
}

void test_clip_keeps_short_text() {
    TEST_ASSERT_EQUAL_STRING("hi", atmosmesh::clip_oled_line("hi").c_str());
}

void test_oled_page_counts_for_64_and_32() {
    TEST_ASSERT_EQUAL_INT(8, atmosmesh::oled_page_count(64));
    TEST_ASSERT_EQUAL_INT(4, atmosmesh::oled_page_count(32));
}

void test_dummy_banner_fits_ssd1306() {
    const auto lines = atmosmesh::dummy_banner();
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(lines.size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(lines[0].size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(lines[1].size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::oled_page_count(32), static_cast<int>(lines.size()));
}

void test_dummy_banner_is_identifiable() {
    const auto lines = atmosmesh::dummy_banner();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("OLED bring-up", lines[1].c_str());
}

void test_live_page_fits_and_shows_sensors() {
    const auto lines = atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 0x76);
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(lines.size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::oled_page_count(32), static_cast<int>(lines.size()));
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(line.size()));
    }
    TEST_ASSERT_EQUAL_STRING("AtmosMesh", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("T 23.4C RH 48.1%", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("BMP 0x76", lines[2].c_str());
}

void test_live_page_missing_sensors() {
    const auto lines = atmosmesh::live_sensor_lines(false, 0.0F, 0.0F, false, -1);
    TEST_ASSERT_EQUAL_STRING("AM2302 missing", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("BMP280 missing", lines[2].c_str());
}

void test_i2c_pins_match_operator_oled_d5_d4() {
    TEST_ASSERT_EQUAL_INT(5, atmosmesh::kOledSdaGpio);
    TEST_ASSERT_EQUAL_INT(4, atmosmesh::kOledSclGpio);
}

void test_sensor_pins_match_operator_bmp_am2302() {
    TEST_ASSERT_EQUAL_INT(21, atmosmesh::kSensorSdaGpio);
    TEST_ASSERT_EQUAL_INT(19, atmosmesh::kSensorSclGpio);
    TEST_ASSERT_EQUAL_INT(18, atmosmesh::kAm2302DataGpio);
}

void test_oled_address_list_prefers_ssd1306_not_lcd() {
    TEST_ASSERT_EQUAL_INT(0x3C, static_cast<int>(atmosmesh::kOledI2cAddresses[0]));
    TEST_ASSERT_EQUAL_INT(0x3D, static_cast<int>(atmosmesh::kOledI2cAddresses[1]));
    for (unsigned addr : atmosmesh::kOledI2cAddresses) {
        TEST_ASSERT_NOT_EQUAL(0x27, static_cast<int>(addr));
        TEST_ASSERT_NOT_EQUAL(0x3F, static_cast<int>(addr));
    }
}

void test_pick_oled_address_prefers_0x3c_over_0x3d() {
    const std::uint8_t found[] = {0x3D, 0x3C};
    TEST_ASSERT_EQUAL_INT(0x3C, atmosmesh::pick_oled_address(found, 2));
}

void test_pick_oled_address_ignores_lcd_backpack_when_oled_present() {
    const std::uint8_t found[] = {0x27, 0x3C};
    TEST_ASSERT_EQUAL_INT(0x3C, atmosmesh::pick_oled_address(found, 2));
}

void test_pick_oled_address_falls_back_to_0x3d() {
    const std::uint8_t found[] = {0x27, 0x3D};
    TEST_ASSERT_EQUAL_INT(0x3D, atmosmesh::pick_oled_address(found, 2));
}

void test_pick_oled_address_empty_is_missing() {
    TEST_ASSERT_EQUAL_INT(-1, atmosmesh::pick_oled_address(nullptr, 0));
}

void test_pick_bmp_address_prefers_0x76() {
    const std::uint8_t found[] = {0x3C, 0x77, 0x76};
    TEST_ASSERT_EQUAL_INT(0x76, atmosmesh::pick_bmp_address(found, 3));
}

void test_pick_bmp_address_accepts_0x77() {
    const std::uint8_t found[] = {0x77};
    TEST_ASSERT_EQUAL_INT(0x77, atmosmesh::pick_bmp_address(found, 1));
}

void test_bmp_family_ids() {
    TEST_ASSERT_TRUE(atmosmesh::is_bmp_family_id(0x58));
    TEST_ASSERT_TRUE(atmosmesh::is_bmp_family_id(0x60));
    TEST_ASSERT_FALSE(atmosmesh::is_bmp_family_id(0x00));
}

void test_am2302_checksum_and_parse() {
    const std::uint8_t frame[5] = {0x02, 0x92, 0x01, 0x0B, 0xA0};  // 65.8 %RH, 26.7 C
    TEST_ASSERT_TRUE(atmosmesh::am2302_checksum_ok(frame));
    const auto sample = atmosmesh::parse_am2302_frame(frame);
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 65.8F, sample.humidity_rh);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 26.7F, sample.temperature_c);
}

void test_am2302_bad_checksum_is_missing() {
    const std::uint8_t frame[5] = {0x02, 0x92, 0x01, 0x0B, 0x00};
    TEST_ASSERT_FALSE(atmosmesh::am2302_checksum_ok(frame));
    TEST_ASSERT_FALSE(atmosmesh::parse_am2302_frame(frame).ok);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_clip_truncates_to_oled_width);
    RUN_TEST(test_clip_keeps_short_text);
    RUN_TEST(test_oled_page_counts_for_64_and_32);
    RUN_TEST(test_dummy_banner_fits_ssd1306);
    RUN_TEST(test_dummy_banner_is_identifiable);
    RUN_TEST(test_live_page_fits_and_shows_sensors);
    RUN_TEST(test_live_page_missing_sensors);
    RUN_TEST(test_i2c_pins_match_operator_oled_d5_d4);
    RUN_TEST(test_sensor_pins_match_operator_bmp_am2302);
    RUN_TEST(test_oled_address_list_prefers_ssd1306_not_lcd);
    RUN_TEST(test_pick_oled_address_prefers_0x3c_over_0x3d);
    RUN_TEST(test_pick_oled_address_ignores_lcd_backpack_when_oled_present);
    RUN_TEST(test_pick_oled_address_falls_back_to_0x3d);
    RUN_TEST(test_pick_oled_address_empty_is_missing);
    RUN_TEST(test_pick_bmp_address_prefers_0x76);
    RUN_TEST(test_pick_bmp_address_accepts_0x77);
    RUN_TEST(test_bmp_family_ids);
    RUN_TEST(test_am2302_checksum_and_parse);
    RUN_TEST(test_am2302_bad_checksum_is_missing);
    return UNITY_END();
}

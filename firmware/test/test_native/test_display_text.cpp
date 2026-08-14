#include <cstdint>
#include <string>

#include <unity.h>

#include "atmosmesh/am2302_frame.hpp"
#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/lcd_address.hpp"
#include "atmosmesh/pins.hpp"

void test_clip_truncates_to_sixteen_characters() {
    const std::string clipped = atmosmesh::clip_lcd_line("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TEST_ASSERT_EQUAL_INT(atmosmesh::kLcdColumns, static_cast<int>(clipped.size()));
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOP", clipped.c_str());
}

void test_clip_keeps_short_text() {
    TEST_ASSERT_EQUAL_STRING("hi", atmosmesh::clip_lcd_line("hi").c_str());
}

void test_dummy_banner_fits_1602() {
    const auto lines = atmosmesh::dummy_banner();
    TEST_ASSERT_EQUAL_INT(atmosmesh::kLcdRows, static_cast<int>(lines.size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kLcdColumns, static_cast<int>(lines[0].size()));
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kLcdColumns, static_cast<int>(lines[1].size()));
}

void test_dummy_banner_is_identifiable() {
    const auto lines = atmosmesh::dummy_banner();
    TEST_ASSERT_EQUAL_STRING("AtmosMesh", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hello, LCD", lines[1].c_str());
}

void test_i2c_pins_match_operator_lcd_d5_d4() {
    TEST_ASSERT_EQUAL_INT(5, atmosmesh::kLcdSdaGpio);
    TEST_ASSERT_EQUAL_INT(4, atmosmesh::kLcdSclGpio);
}

void test_sensor_pins_match_operator_bmp_am2302() {
    TEST_ASSERT_EQUAL_INT(21, atmosmesh::kSensorSdaGpio);
    TEST_ASSERT_EQUAL_INT(19, atmosmesh::kSensorSclGpio);
    TEST_ASSERT_EQUAL_INT(18, atmosmesh::kAm2302DataGpio);
}

void test_lcd_address_list_includes_common_backpacks() {
    bool has_27 = false;
    bool has_3f = false;
    for (unsigned addr : atmosmesh::kLcdI2cAddresses) {
        has_27 = has_27 || addr == 0x27;
        has_3f = has_3f || addr == 0x3F;
    }
    TEST_ASSERT_TRUE(has_27);
    TEST_ASSERT_TRUE(has_3f);
}

void test_pick_lcd_address_prefers_known_backpack() {
    const std::uint8_t found[] = {0x3C, 0x27};
    TEST_ASSERT_EQUAL_INT(0x27, atmosmesh::pick_lcd_address(found, 2));
}

void test_pick_lcd_address_falls_back_to_first_hit() {
    const std::uint8_t found[] = {0x3C};
    TEST_ASSERT_EQUAL_INT(0x3C, atmosmesh::pick_lcd_address(found, 1));
}

void test_pick_lcd_address_empty_is_missing() {
    TEST_ASSERT_EQUAL_INT(-1, atmosmesh::pick_lcd_address(nullptr, 0));
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
    RUN_TEST(test_clip_truncates_to_sixteen_characters);
    RUN_TEST(test_clip_keeps_short_text);
    RUN_TEST(test_dummy_banner_fits_1602);
    RUN_TEST(test_dummy_banner_is_identifiable);
    RUN_TEST(test_i2c_pins_match_operator_lcd_d5_d4);
    RUN_TEST(test_sensor_pins_match_operator_bmp_am2302);
    RUN_TEST(test_lcd_address_list_includes_common_backpacks);
    RUN_TEST(test_pick_lcd_address_prefers_known_backpack);
    RUN_TEST(test_pick_lcd_address_falls_back_to_first_hit);
    RUN_TEST(test_pick_lcd_address_empty_is_missing);
    RUN_TEST(test_pick_bmp_address_prefers_0x76);
    RUN_TEST(test_pick_bmp_address_accepts_0x77);
    RUN_TEST(test_bmp_family_ids);
    RUN_TEST(test_am2302_checksum_and_parse);
    RUN_TEST(test_am2302_bad_checksum_is_missing);
    return UNITY_END();
}

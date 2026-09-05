#include <cstdint>
#include <string>

#include <unity.h>

#include "atmosmesh/am2302_frame.hpp"
#include "atmosmesh/bmp_address.hpp"
#include "atmosmesh/digital_edge.hpp"
#include "atmosmesh/display_text.hpp"
#include "atmosmesh/oled_address.hpp"
#include "atmosmesh/mq135_scale.hpp"
#include "atmosmesh/oled_profile.hpp"
#include "atmosmesh/pins.hpp"
#include "atmosmesh/sds011_frame.hpp"
#include "atmosmesh/veml7700_text.hpp"

void register_product_variant_tests();
void register_sht41_frame_tests();
void register_sds011_frame_tests();
void register_pm_alarm_tests();
void register_aqua_status_tests();
void register_ld2410s_frame_tests();

void test_clip_truncates_to_oled_width() {
    const std::string clipped = atmosmesh::clip_oled_line("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TEST_ASSERT_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(clipped.size()));
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOPQRSTU", clipped.c_str());
}

void test_clip_keeps_short_text() {
    TEST_ASSERT_EQUAL_STRING("hi", atmosmesh::clip_oled_line("hi").c_str());
}

void test_oled_page_counts_for_64_48_and_32() {
    TEST_ASSERT_EQUAL_INT(8, atmosmesh::oled_page_count(64));
    TEST_ASSERT_EQUAL_INT(6, atmosmesh::oled_page_count(48));
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

void test_live_page_is_three_rows_on_64px() {
    const auto lines =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819);
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(lines.size()));
    TEST_ASSERT_EQUAL_INT(3, atmosmesh::oled_live_line_count(64));
    TEST_ASSERT_EQUAL_INT(12, atmosmesh::oled_line_pitch_px(64));
    TEST_ASSERT_EQUAL_INT(34, atmosmesh::oled_live_row_y_px(0));
    TEST_ASSERT_EQUAL_INT(46, atmosmesh::oled_live_row_y_px(1));
    TEST_ASSERT_EQUAL_INT(58, atmosmesh::oled_live_row_y_px(2));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(32, atmosmesh::oled_live_row_y_px(0));
    TEST_ASSERT_LESS_OR_EQUAL_INT(62, atmosmesh::oled_live_row_y_px(2));
    TEST_ASSERT_EQUAL_INT(62, atmosmesh::oled_telltale_bar_y_px());
    TEST_ASSERT_EQUAL_INT(2, atmosmesh::oled_telltale_bar_height_px());
    TEST_ASSERT_EQUAL_INT(64, atmosmesh::oled_telltale_bar_y_px() +
                                  atmosmesh::oled_telltale_bar_height_px());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(line.size()));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("CO2"));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("co2"));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("ppm"));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("MQ "));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("/"));
    }
    TEST_ASSERT_EQUAL_STRING("23.4C  48% RH idle", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("1013hPa 0.0C   -- lx", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("PM2.5 12   PM10 20", lines[2].c_str());
}

void test_live_page_labels_pir_state_not_bare_flag() {
    const auto idle =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819,
                                     false);
    // Idle is spelled out: a blank cell must never be mistaken for "no reading".
    TEST_ASSERT_EQUAL_STRING("23.4C  48% RH idle", idle[0].c_str());
    const auto motion =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819,
                                     true);
    TEST_ASSERT_EQUAL_STRING("23.4C  48% RH MOT", motion[0].c_str());
    TEST_ASSERT_EQUAL_STRING(idle[1].c_str(), motion[1].c_str());
    TEST_ASSERT_EQUAL_STRING(idle[2].c_str(), motion[2].c_str());
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(motion[0].size()));
    TEST_ASSERT_EQUAL_STRING("MOT", atmosmesh::format_pir_oled(true).c_str());
    TEST_ASSERT_EQUAL_STRING("idle", atmosmesh::format_pir_oled(false).c_str());
}

void test_live_page_shows_lux_on_hpa_line() {
    const auto missing =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819,
                                     false, false, 0.0F, 21.5F);
    TEST_ASSERT_EQUAL_STRING("1013hPa 21.5C  -- lx", missing[1].c_str());
    const auto present =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819,
                                     false, true, 123.4F, 21.5F);
    TEST_ASSERT_EQUAL_STRING("1013hPa 21.5C  123 lx", present[1].c_str());
    TEST_ASSERT_EQUAL_STRING("23.4C  48% RH idle", present[0].c_str());
    TEST_ASSERT_EQUAL_STRING("PM2.5 12   PM10 20", present[2].c_str());
    TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(present[1].size()));
}

void test_live_page_shows_bmp_temperature_beside_pressure() {
    const auto lines =
        atmosmesh::live_sensor_lines(true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 819,
                                     false, true, 123.4F, 29.7F);
    // BMP280 temperature was read and logged but never displayed before 2026-08-17.
    TEST_ASSERT_EQUAL_STRING("1013hPa 29.7C  123 lx", lines[1].c_str());
    // AM2302 temperature stays on its own row so the two are comparable, not conflated.
    TEST_ASSERT_EQUAL_STRING("23.4C  48% RH idle", lines[0].c_str());
}

void test_right_cell_alternates_lux_and_mq_over_time() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledRightCell::Lux),
                          static_cast<int>(atmosmesh::oled_right_cell_for_ms(0UL)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledRightCell::Lux),
                          static_cast<int>(atmosmesh::oled_right_cell_for_ms(3999UL)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledRightCell::Mq),
                          static_cast<int>(atmosmesh::oled_right_cell_for_ms(4000UL)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledRightCell::Mq),
                          static_cast<int>(atmosmesh::oled_right_cell_for_ms(7999UL)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledRightCell::Lux),
                          static_cast<int>(atmosmesh::oled_right_cell_for_ms(8000UL)));
    // Default phase is Lux, so a caller that passes no phase keeps the pre-rotation layout.
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(atmosmesh::OledRightCell::Lux));
}

void test_mq135_cell_shows_raw_count_and_err_on_zero() {
    TEST_ASSERT_EQUAL_STRING("MQ724", atmosmesh::format_mq135_oled(724).c_str());
    TEST_ASSERT_EQUAL_STRING("MQ4095", atmosmesh::format_mq135_oled(4095).c_str());
    // raw=0 is the firmware's existing "check AOUT/GND/5V heater" fault, not a real reading.
    TEST_ASSERT_EQUAL_STRING("MQ ERR", atmosmesh::format_mq135_oled(0).c_str());
    TEST_ASSERT_EQUAL_STRING("MQ ERR", atmosmesh::format_mq135_oled(-1).c_str());
    // The bench MQ135 is uncalibrated: raw counts only, never a gas concentration.
    for (const int raw : {0, 1, 724, 4095}) {
        const std::string cell = atmosmesh::format_mq135_oled(raw);
        TEST_ASSERT_EQUAL(std::string::npos, cell.find("ppm"));
        TEST_ASSERT_EQUAL(std::string::npos, cell.find("CO2"));
    }
}

void test_live_page_mq_phase_replaces_lux_cell() {
    const auto lux_phase = atmosmesh::live_sensor_lines(
        true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 724, false, true, 123.4F, 29.7F,
        atmosmesh::OledRightCell::Lux);
    const auto mq_phase = atmosmesh::live_sensor_lines(
        true, 23.4F, 48.1F, true, 1013.2F, true, 12.3F, 20.1F, 724, false, true, 123.4F, 29.7F,
        atmosmesh::OledRightCell::Mq);
    TEST_ASSERT_EQUAL_STRING("1013hPa 29.7C  123 lx", lux_phase[1].c_str());
    TEST_ASSERT_EQUAL_STRING("1013hPa 29.7C  MQ724", mq_phase[1].c_str());
    // Only the hPa row's right cell moves; the other two rows are stable across the swap.
    TEST_ASSERT_EQUAL_STRING(lux_phase[0].c_str(), mq_phase[0].c_str());
    TEST_ASSERT_EQUAL_STRING(lux_phase[2].c_str(), mq_phase[2].c_str());
}

void test_live_page_fits_worst_case_six_cells() {
    // Widest possible row: negative BMP temp and a full-scale MQ count in the rotating cell.
    const auto lines = atmosmesh::live_sensor_lines(
        true, -10.0F, 100.0F, true, 1013.2F, true, 999.4F, 999.4F, 4095, false, false, 0.0F,
        -10.0F, atmosmesh::OledRightCell::Mq);
    TEST_ASSERT_EQUAL_STRING("-10.0C  100% RH idle", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("1013hPa -10.0C MQ4095", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("PM2.5 999  PM10 999", lines[2].c_str());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(line.size()));
    }
}

void test_live_page_missing_sensors() {
    const auto lines =
        atmosmesh::live_sensor_lines(false, 0.0F, 0.0F, false, 0.0F, false, 0.0F, 0.0F, 0);
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(lines.size()));
    TEST_ASSERT_EQUAL_STRING("--C  --% RH idle", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("--hPa --C      -- lx", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("PM2.5 --   PM10 --", lines[2].c_str());
    for (const auto& line : lines) {
        TEST_ASSERT_LESS_OR_EQUAL_INT(atmosmesh::kOledMaxChars, static_cast<int>(line.size()));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("SDS011 missing"));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("AM2302 missing"));
        TEST_ASSERT_EQUAL(std::string::npos, line.find("BMP280 missing"));
    }
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

void test_sds011_uart_pins_are_rx2_tx2() {
    TEST_ASSERT_EQUAL_INT(16, atmosmesh::kSds011RxGpio);
    TEST_ASSERT_EQUAL_INT(17, atmosmesh::kSds011TxGpio);
    TEST_ASSERT_EQUAL_INT(9600, atmosmesh::kSds011Baud);
    TEST_ASSERT_EQUAL_INT(34, atmosmesh::kMq135AdcGpio);
}

void test_extra_peripheral_pins_match_live_bench() {
    TEST_ASSERT_EQUAL_INT(25, atmosmesh::kBeeperGpio);
    TEST_ASSERT_EQUAL_INT(33, atmosmesh::kPirGpio);
    TEST_ASSERT_EQUAL_INT(50, atmosmesh::kDigitalDebounceMs);
    TEST_ASSERT_EQUAL_INT(50, atmosmesh::kBeeperPulseMs);
    TEST_ASSERT_EQUAL_INT(21, atmosmesh::kSensorSdaGpio);
    TEST_ASSERT_EQUAL_INT(19, atmosmesh::kSensorSclGpio);
    TEST_ASSERT_FALSE(atmosmesh::gpio_is_input_only(atmosmesh::kBeeperGpio));
    TEST_ASSERT_TRUE(atmosmesh::gpio_is_adc1(atmosmesh::kMq135AdcGpio));
}

void test_veml7700_address_is_0x10_no_bmp_clash() {
    TEST_ASSERT_EQUAL_HEX8(0x10, atmosmesh::kVeml7700Address);
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(atmosmesh::kBmp280AddressGnd),
                          static_cast<int>(atmosmesh::kVeml7700Address));
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(atmosmesh::kBmp280AddressVdd),
                          static_cast<int>(atmosmesh::kVeml7700Address));
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(atmosmesh::kOledI2cAddresses[0]),
                          static_cast<int>(atmosmesh::kVeml7700Address));
    const std::uint8_t found[] = {0x76, 0x10};
    TEST_ASSERT_TRUE(atmosmesh::has_veml7700_address(found, 2));
    const std::uint8_t bmp_only[] = {0x76};
    TEST_ASSERT_FALSE(atmosmesh::has_veml7700_address(bmp_only, 1));
    TEST_ASSERT_FALSE(atmosmesh::has_veml7700_address(nullptr, 0));
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

void test_oled_i2c_clock_is_100khz_for_cheap_modules() {
    TEST_ASSERT_EQUAL_UINT32(100000U, atmosmesh::kOledI2cHz);
}

void test_default_oled_profile_is_ssd1306_128x64_alt0() {
    const auto profile = atmosmesh::default_oled_profile();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledController::Ssd1306),
                          static_cast<int>(profile.controller));
    TEST_ASSERT_EQUAL_INT(128, profile.width_px);
    TEST_ASSERT_EQUAL_INT(64, profile.height_px);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledComPins::Sequential),
                          static_cast<int>(profile.com_pins));
    TEST_ASSERT_EQUAL_HEX8(0x02, atmosmesh::oled_compins_arg(profile.com_pins));
    TEST_ASSERT_EQUAL_INT(0, profile.column_offset_px);
    TEST_ASSERT_EQUAL_INT(63, profile.clip_max_y);
    TEST_ASSERT_FALSE(atmosmesh::oled_should_set_mux(profile));
    TEST_ASSERT_EQUAL_STRING("SSD1306_ALT0", atmosmesh::oled_profile_name(profile));
    TEST_ASSERT_EQUAL_STRING("SSD1306", atmosmesh::oled_controller_name(profile.controller));
    TEST_ASSERT_EQUAL_STRING("U8G2_SSD1306_128X64_ALT0_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(profile));
}

void test_sh1106_compile_fallback_is_controller_id_one() {
    const auto profile =
        atmosmesh::resolve_oled_profile(atmosmesh::OledController::Sh1106, 64);
    TEST_ASSERT_EQUAL_INT(2, profile.column_offset_px);
    TEST_ASSERT_EQUAL_STRING("SH1106", atmosmesh::oled_profile_name(profile));
    TEST_ASSERT_EQUAL_STRING("U8G2_SH1106_128X64_NONAME_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(profile));
}

void test_compiled_oled_profile_defaults_to_ssd1306_128x64_alt0() {
    const auto profile = atmosmesh::compiled_oled_profile();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledController::Ssd1306),
                          static_cast<int>(profile.controller));
    TEST_ASSERT_EQUAL_INT(64, profile.height_px);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledComPins::Sequential),
                          static_cast<int>(profile.com_pins));
    TEST_ASSERT_EQUAL_HEX8(0x02, atmosmesh::oled_compins_arg(profile.com_pins));
    TEST_ASSERT_EQUAL_INT(0, profile.column_offset_px);
    TEST_ASSERT_FALSE(atmosmesh::oled_should_set_mux(profile));
    TEST_ASSERT_EQUAL_STRING("SSD1306_ALT0", atmosmesh::oled_profile_name(profile));
    TEST_ASSERT_EQUAL_STRING("U8G2_SSD1306_128X64_ALT0_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(profile));
}

void test_resolve_sh1106_uses_two_pixel_column_offset() {
    const auto profile =
        atmosmesh::resolve_oled_profile(atmosmesh::OledController::Sh1106, 64);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledController::Sh1106),
                          static_cast<int>(profile.controller));
    TEST_ASSERT_EQUAL_INT(128, profile.width_px);
    TEST_ASSERT_EQUAL_INT(64, profile.height_px);
    TEST_ASSERT_EQUAL_INT(2, profile.column_offset_px);
    TEST_ASSERT_EQUAL_STRING("SH1106", atmosmesh::oled_controller_name(profile.controller));
}

void test_resolve_ssd1306_32px_uses_four_pages() {
    const auto profile =
        atmosmesh::resolve_oled_profile(atmosmesh::OledController::Ssd1306, 32);
    TEST_ASSERT_EQUAL_INT(32, profile.height_px);
    TEST_ASSERT_EQUAL_INT(4, atmosmesh::oled_page_count(profile.height_px));
    TEST_ASSERT_EQUAL_INT(0, profile.column_offset_px);
    TEST_ASSERT_EQUAL_STRING("SSD1306", atmosmesh::oled_controller_name(profile.controller));
}

void test_oled_init_log_includes_controller_geometry_and_addr() {
    const auto profile = atmosmesh::default_oled_profile();
    const std::string line = atmosmesh::format_oled_init_log(profile, 0x3C);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("controller=SSD1306"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("profile=SSD1306_ALT0"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("width=128"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("height=64"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("addr=0x3C"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("com=sequential"));
}

void test_parse_oled_controller_flag() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledController::Ssd1306),
                          static_cast<int>(atmosmesh::parse_oled_controller_flag("SSD1306")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(atmosmesh::OledController::Sh1106),
                          static_cast<int>(atmosmesh::parse_oled_controller_flag("SH1106")));
}

void test_oled_boot_bars_mark_eight_pixel_pages() {
    TEST_ASSERT_EQUAL_INT(5, atmosmesh::oled_boot_bar_count());
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::oled_boot_bar_y_px(0));
    TEST_ASSERT_EQUAL_INT(16, atmosmesh::oled_boot_bar_y_px(1));
    TEST_ASSERT_EQUAL_INT(32, atmosmesh::oled_boot_bar_y_px(2));
    TEST_ASSERT_EQUAL_INT(48, atmosmesh::oled_boot_bar_y_px(3));
    TEST_ASSERT_EQUAL_INT(62, atmosmesh::oled_boot_bar_y_px(4));
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::oled_boot_bar_y_px(-1));
    TEST_ASSERT_EQUAL_INT(1500, atmosmesh::oled_boot_bar_hold_ms());
}

void test_oled_prove_life_serial_lines() {
    TEST_ASSERT_EQUAL_STRING("oled: display on", atmosmesh::format_oled_display_on_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: contrast 255", atmosmesh::format_oled_contrast_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: invert off", atmosmesh::format_oled_invert_off_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: full white", atmosmesh::format_oled_full_white_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: text HI", atmosmesh::format_oled_text_hi_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: mux=0x1F (128x32 attempt)",
                             atmosmesh::format_oled_mux32_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: height=48 mux=0x2F",
                             atmosmesh::format_oled_mux48_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: telltale bar y=62 h=2",
                             atmosmesh::format_oled_telltale_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: bars y=0,16,32,48,62 — say which you see",
                             atmosmesh::format_oled_boot_bars_log().c_str());
    TEST_ASSERT_EQUAL_STRING("oled: flip=0", atmosmesh::format_oled_flip_log().c_str());
}

void test_compiled_oled_flip_defaults_to_off() {
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::compiled_oled_flip_mode());
}

void test_ssd1306_mux32_command_is_a8_1f() {
    TEST_ASSERT_EQUAL_HEX8(0xA8, atmosmesh::kSsd1306SetMultiplex);
    TEST_ASSERT_EQUAL_HEX8(0x1F, atmosmesh::kSsd1306MuxRatio32);
}

void test_ssd1306_mux48_command_is_a8_2f() {
    TEST_ASSERT_EQUAL_HEX8(0xA8, atmosmesh::kSsd1306SetMultiplex);
    TEST_ASSERT_EQUAL_HEX8(0x2F, atmosmesh::kSsd1306MuxRatio48);
    TEST_ASSERT_EQUAL_HEX8(0xD3, atmosmesh::kSsd1306SetDisplayOffset);
    TEST_ASSERT_EQUAL_HEX8(0x00, atmosmesh::kSsd1306DisplayOffset0);
}

void test_u8g2_constructor_names_match_profile() {
    TEST_ASSERT_EQUAL_STRING(
        "U8G2_SSD1306_128X64_ALT0_F_HW_I2C",
        atmosmesh::u8g2_hw_i2c_constructor_name(atmosmesh::default_oled_profile()));
    TEST_ASSERT_EQUAL_STRING("U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(atmosmesh::resolve_oled_profile(
                                 atmosmesh::OledController::Ssd1306, 32)));
    TEST_ASSERT_EQUAL_STRING("U8G2_SH1106_128X64_NONAME_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(atmosmesh::resolve_oled_profile(
                                 atmosmesh::OledController::Sh1106, 64)));
    TEST_ASSERT_EQUAL_STRING("U8G2_SSD1306_128X64_ALT0_F_HW_I2C",
                             atmosmesh::u8g2_hw_i2c_constructor_name(atmosmesh::resolve_oled_profile(
                                 atmosmesh::OledController::Ssd1306, 64)));
}

void test_sds011_listen_log_names_gpio16_not_tx2() {
    const std::string line = atmosmesh::format_sds011_listen_log();
    TEST_ASSERT_EQUAL_STRING(
        "sds011: listen GPIO16 (RX2); TX2=GPIO17 is ESP output — sensor TX goes to RX2",
        line.c_str());
    TEST_ASSERT_EQUAL(std::string::npos, line.find("GPIO17 as RX"));
}

void test_sds011_no_frame_log_points_at_rx2() {
    const std::string line = atmosmesh::format_sds011_no_frame_log();
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("no AA C0 frame"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("GPIO16"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, line.find("TX2"));
}

void test_am2302_bad_checksum_is_missing() {
    const std::uint8_t frame[5] = {0x02, 0x92, 0x01, 0x0B, 0x00};
    TEST_ASSERT_FALSE(atmosmesh::am2302_checksum_ok(frame));
    TEST_ASSERT_FALSE(atmosmesh::parse_am2302_frame(frame).ok);
}

void test_am2302_poll_interval_is_at_least_two_seconds() {
    TEST_ASSERT_GREATER_OR_EQUAL_INT(2500, atmosmesh::kAm2302MinIntervalMs);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, atmosmesh::kAm2302HoldMisses);
}

void test_am2302_hold_keeps_last_good_across_misses() {
    atmosmesh::Am2302Hold hold{};
    TEST_ASSERT_FALSE(
        atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F, atmosmesh::kAm2302HoldMisses));
    TEST_ASSERT_FALSE(hold.show);

    TEST_ASSERT_TRUE(atmosmesh::update_am2302_hold(hold, true, 23.4F, 48.1F,
                                                    atmosmesh::kAm2302HoldMisses));
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 23.4F, hold.temperature_c);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 48.1F, hold.humidity_rh);

    TEST_ASSERT_TRUE(atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F,
                                                   atmosmesh::kAm2302HoldMisses));
    TEST_ASSERT_TRUE(hold.show);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 23.4F, hold.temperature_c);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 48.1F, hold.humidity_rh);
}

void test_am2302_hold_oled_does_not_flash_dash_on_one_miss() {
    atmosmesh::Am2302Hold hold{};
    atmosmesh::update_am2302_hold(hold, true, 32.2F, 30.0F, atmosmesh::kAm2302HoldMisses);
    atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F, atmosmesh::kAm2302HoldMisses);
    const auto lines = atmosmesh::live_sensor_lines(hold.show, hold.temperature_c, hold.humidity_rh,
                                                    true, 1013.2F, true, 12.3F, 20.1F, 819);
    TEST_ASSERT_EQUAL_STRING("32.2C  30% RH idle", lines[0].c_str());
}

void test_am2302_hold_blanks_after_max_consecutive_misses() {
    atmosmesh::Am2302Hold hold{};
    atmosmesh::update_am2302_hold(hold, true, 23.4F, 48.1F, 2);
    TEST_ASSERT_TRUE(atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F, 2));
    TEST_ASSERT_TRUE(atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F, 2));
    TEST_ASSERT_FALSE(atmosmesh::update_am2302_hold(hold, false, 0.0F, 0.0F, 2));
    TEST_ASSERT_FALSE(hold.show);
    const auto lines = atmosmesh::live_sensor_lines(hold.show, hold.temperature_c, hold.humidity_rh,
                                                    false, 0.0F, false, 0.0F, 0.0F, 0);
    TEST_ASSERT_EQUAL_STRING("--C  --% RH idle", lines[0].c_str());
}

void test_sds011_checksum_and_parse() {
    // PM2.5=12.3, PM10=20.1, id=A1B2; CRC = sum of payload bytes 2..7.
    const std::uint8_t frame[10] = {0xAA, 0xC0, 0x7B, 0x00, 0xC9, 0x00, 0xA1, 0xB2, 0x97, 0xAB};
    TEST_ASSERT_TRUE(atmosmesh::sds011_checksum_ok(frame));
    const auto sample = atmosmesh::parse_sds011_frame(frame);
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 12.3F, sample.pm25_ug_m3);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 20.1F, sample.pm10_ug_m3);
}

void test_sds011_bad_checksum_is_missing() {
    const std::uint8_t frame[10] = {0xAA, 0xC0, 0x7B, 0x00, 0xC9, 0x00, 0xA1, 0xB2, 0x00, 0xAB};
    TEST_ASSERT_FALSE(atmosmesh::sds011_checksum_ok(frame));
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(frame).ok);
}

void test_sds011_rejects_wrong_header_or_tail() {
    const std::uint8_t bad_cmd[10] = {0xAA, 0xC5, 0x7B, 0x00, 0xC9, 0x00, 0xA1, 0xB2, 0x97, 0xAB};
    const std::uint8_t bad_tail[10] = {0xAA, 0xC0, 0x7B, 0x00, 0xC9, 0x00, 0xA1, 0xB2, 0x97, 0x00};
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(bad_cmd).ok);
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(bad_tail).ok);
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(nullptr).ok);
}

void test_mq135_divider_is_two_thirds_ten_k_twenty_k() {
    TEST_ASSERT_EQUAL_INT(10000, atmosmesh::kMq135SeriesOhms);
    TEST_ASSERT_EQUAL_INT(20000, atmosmesh::kMq135GndOhms);
    TEST_ASSERT_EQUAL_INT(34, atmosmesh::kMq135AdcGpio);
    TEST_ASSERT_EQUAL_INT(4095, atmosmesh::kMq135AdcMax);
    TEST_ASSERT_EQUAL_INT(3300, atmosmesh::kMq135AdcFullScaleMv);
}

void test_mq135_millivolts_from_adc_inverts_divider() {
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::mq135_gpio_millivolts(0));
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::mq135_aout_millivolts(0));
    TEST_ASSERT_EQUAL_INT(1650, atmosmesh::mq135_gpio_millivolts(2048));
    TEST_ASSERT_EQUAL_INT(2475, atmosmesh::mq135_aout_millivolts(1650));
    TEST_ASSERT_EQUAL_INT(3300, atmosmesh::mq135_gpio_millivolts(4095));
    TEST_ASSERT_EQUAL_INT(4950, atmosmesh::mq135_aout_millivolts(3300));
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::mq135_gpio_millivolts(-1));
    TEST_ASSERT_EQUAL_INT(3300, atmosmesh::mq135_gpio_millivolts(5000));
}

void test_mq135_five_volt_aout_has_no_gpio_headroom() {
    const int gpio_at_5v_aout =
        atmosmesh::mq135_aout_to_gpio_millivolts(5000);
    TEST_ASSERT_EQUAL_INT(3333, gpio_at_5v_aout);
    TEST_ASSERT_TRUE(gpio_at_5v_aout > atmosmesh::kMq135AdcFullScaleMv);
}

void test_mq135_serial_and_oled_never_say_co2() {
    const std::string serial = atmosmesh::format_mq135_serial(2048);
    const std::string oled = atmosmesh::format_mq135_oled_line(2048);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, serial.find("mq135: raw=2048"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, serial.find("gpio_mv=1650"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, serial.find("aout_mv=2475"));
    TEST_ASSERT_EQUAL_INT(50, atmosmesh::mq135_gas_index(2048));
    TEST_ASSERT_EQUAL_INT(0, atmosmesh::mq135_gas_index(0));
    TEST_ASSERT_EQUAL_INT(100, atmosmesh::mq135_gas_index(4095));
    TEST_ASSERT_EQUAL_STRING("gas 50", oled.c_str());
    TEST_ASSERT_EQUAL(std::string::npos, serial.find("CO2"));
    TEST_ASSERT_EQUAL(std::string::npos, serial.find("co2"));
    TEST_ASSERT_EQUAL(std::string::npos, serial.find("ppm"));
    TEST_ASSERT_EQUAL(std::string::npos, oled.find("CO2"));
    TEST_ASSERT_EQUAL(std::string::npos, oled.find("V"));
}

void test_bmp280_serial_shows_celsius_and_hpa() {
    const std::string line = atmosmesh::format_bmp280_serial(23.1F, 1013.2F);
    TEST_ASSERT_EQUAL_STRING("bmp280: t=23.1C p=1013.2 hPa", line.c_str());
}

void test_mq135_serial_warns_when_adc_near_zero() {
    const std::string serial = atmosmesh::format_mq135_serial(0);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, serial.find("raw=0"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, serial.find("check AOUT/GND/5V heater"));
}

void test_sds011_stream_skips_noise_then_parses() {
    atmosmesh::Sds011Stream stream;
    const std::uint8_t noise[] = {0x00, 0xAA, 0x11, 0xAA};
    for (std::uint8_t b : noise) {
        TEST_ASSERT_FALSE(stream.feed(b).ok);
    }
    const std::uint8_t frame[10] = {0xAA, 0xC0, 0x7B, 0x00, 0xC9, 0x00, 0xA1, 0xB2, 0x97, 0xAB};
    atmosmesh::Sds011Sample last{};
    for (std::uint8_t b : frame) {
        last = stream.feed(b);
    }
    TEST_ASSERT_TRUE(last.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 12.3F, last.pm25_ug_m3);
}

void test_pir_serial_labels() {
    TEST_ASSERT_EQUAL_STRING("pir: motion", atmosmesh::format_pir_log(true).c_str());
    TEST_ASSERT_EQUAL_STRING("pir: idle", atmosmesh::format_pir_log(false).c_str());
    TEST_ASSERT_EQUAL_STRING("beep: boot", atmosmesh::format_beep_boot_log().c_str());
}

void test_veml7700_lux_formatter() {
    TEST_ASSERT_EQUAL_STRING("123 lx", atmosmesh::format_lux_oled(true, 123.4F).c_str());
    TEST_ASSERT_EQUAL_STRING("-- lx", atmosmesh::format_lux_oled(false, 123.4F).c_str());
    TEST_ASSERT_EQUAL_STRING("veml7700: lux=123",
                             atmosmesh::format_veml7700_serial(true, 123.4F).c_str());
    TEST_ASSERT_EQUAL_STRING("veml7700: not found (ok until fitted)",
                             atmosmesh::format_veml7700_serial(false, 0.0F).c_str());
}

void test_debounce_ignores_glitch_under_50ms() {
    atmosmesh::DebouncedLevel pir{};
    TEST_ASSERT_TRUE(atmosmesh::update_debounced_level(pir, false, 0, atmosmesh::kDigitalDebounceMs));
    TEST_ASSERT_FALSE(pir.stable);
    TEST_ASSERT_FALSE(atmosmesh::update_debounced_level(pir, true, 10, atmosmesh::kDigitalDebounceMs));
    TEST_ASSERT_FALSE(pir.stable);
    TEST_ASSERT_FALSE(atmosmesh::update_debounced_level(pir, false, 20, atmosmesh::kDigitalDebounceMs));
    TEST_ASSERT_FALSE(pir.stable);
    TEST_ASSERT_FALSE(atmosmesh::update_debounced_level(pir, true, 30, atmosmesh::kDigitalDebounceMs));
    TEST_ASSERT_TRUE(atmosmesh::update_debounced_level(pir, true, 80, atmosmesh::kDigitalDebounceMs));
    TEST_ASSERT_TRUE(pir.stable);
}

int main() {
    UNITY_BEGIN();
    register_product_variant_tests();
    register_sht41_frame_tests();
    register_sds011_frame_tests();
    register_pm_alarm_tests();
    register_aqua_status_tests();
    register_ld2410s_frame_tests();
    RUN_TEST(test_clip_truncates_to_oled_width);
    RUN_TEST(test_clip_keeps_short_text);
    RUN_TEST(test_oled_page_counts_for_64_48_and_32);
    RUN_TEST(test_dummy_banner_fits_ssd1306);
    RUN_TEST(test_dummy_banner_is_identifiable);
    RUN_TEST(test_live_page_is_three_rows_on_64px);
    RUN_TEST(test_oled_boot_bars_mark_eight_pixel_pages);
    RUN_TEST(test_compiled_oled_flip_defaults_to_off);
    RUN_TEST(test_live_page_fits_worst_case_six_cells);
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
    RUN_TEST(test_oled_i2c_clock_is_100khz_for_cheap_modules);
    RUN_TEST(test_default_oled_profile_is_ssd1306_128x64_alt0);
    RUN_TEST(test_sh1106_compile_fallback_is_controller_id_one);
    RUN_TEST(test_compiled_oled_profile_defaults_to_ssd1306_128x64_alt0);
    RUN_TEST(test_mq135_divider_is_two_thirds_ten_k_twenty_k);
    RUN_TEST(test_mq135_millivolts_from_adc_inverts_divider);
    RUN_TEST(test_mq135_five_volt_aout_has_no_gpio_headroom);
    RUN_TEST(test_mq135_serial_and_oled_never_say_co2);
    RUN_TEST(test_bmp280_serial_shows_celsius_and_hpa);
    RUN_TEST(test_mq135_serial_warns_when_adc_near_zero);
    RUN_TEST(test_resolve_sh1106_uses_two_pixel_column_offset);
    RUN_TEST(test_resolve_ssd1306_32px_uses_four_pages);
    RUN_TEST(test_oled_init_log_includes_controller_geometry_and_addr);
    RUN_TEST(test_parse_oled_controller_flag);
    RUN_TEST(test_oled_prove_life_serial_lines);
    RUN_TEST(test_ssd1306_mux32_command_is_a8_1f);
    RUN_TEST(test_ssd1306_mux48_command_is_a8_2f);
    RUN_TEST(test_u8g2_constructor_names_match_profile);
    RUN_TEST(test_sds011_listen_log_names_gpio16_not_tx2);
    RUN_TEST(test_sds011_no_frame_log_points_at_rx2);
    RUN_TEST(test_am2302_checksum_and_parse);
    RUN_TEST(test_am2302_bad_checksum_is_missing);
    RUN_TEST(test_am2302_poll_interval_is_at_least_two_seconds);
    RUN_TEST(test_am2302_hold_keeps_last_good_across_misses);
    RUN_TEST(test_am2302_hold_oled_does_not_flash_dash_on_one_miss);
    RUN_TEST(test_am2302_hold_blanks_after_max_consecutive_misses);
    RUN_TEST(test_sds011_uart_pins_are_rx2_tx2);
    RUN_TEST(test_sds011_checksum_and_parse);
    RUN_TEST(test_sds011_bad_checksum_is_missing);
    RUN_TEST(test_sds011_rejects_wrong_header_or_tail);
    RUN_TEST(test_sds011_stream_skips_noise_then_parses);
    RUN_TEST(test_live_page_labels_pir_state_not_bare_flag);
    RUN_TEST(test_live_page_shows_lux_on_hpa_line);
    RUN_TEST(test_live_page_shows_bmp_temperature_beside_pressure);
    RUN_TEST(test_right_cell_alternates_lux_and_mq_over_time);
    RUN_TEST(test_mq135_cell_shows_raw_count_and_err_on_zero);
    RUN_TEST(test_live_page_mq_phase_replaces_lux_cell);
    RUN_TEST(test_extra_peripheral_pins_match_live_bench);
    RUN_TEST(test_veml7700_address_is_0x10_no_bmp_clash);
    RUN_TEST(test_pir_serial_labels);
    RUN_TEST(test_veml7700_lux_formatter);
    RUN_TEST(test_debounce_ignores_glitch_under_50ms);
    return UNITY_END();
}

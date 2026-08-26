#include <cstdint>

#include <unity.h>

#include "atmosmesh/sht4x_frame.hpp"

namespace {

void test_sht41_crc8_matches_datasheet_vector() {
    const std::uint8_t data[] = {0xBE, 0xEF};
    TEST_ASSERT_EQUAL_HEX8(0x92, atmosmesh::sht41_crc8(data, 2));
}

void test_sht41_frame_decodes_known_reading() {
    const std::uint8_t bytes[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x92};
    const auto sample = atmosmesh::parse_sht41_frame(bytes);
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.2F, 85.52F, sample.temperature_c);
    TEST_ASSERT_FLOAT_WITHIN(0.2F, 87.23F, sample.humidity_pct);
}

void test_sht41_frame_rejects_temperature_crc_mismatch() {
    const std::uint8_t bytes[6] = {0xBE, 0xEF, 0x00, 0xBE, 0xEF, 0x92};
    const auto sample = atmosmesh::parse_sht41_frame(bytes);
    TEST_ASSERT_FALSE(sample.ok);
}

void test_sht41_frame_rejects_humidity_crc_mismatch() {
    const std::uint8_t bytes[6] = {0xBE, 0xEF, 0x92, 0xBE, 0xEF, 0x00};
    const auto sample = atmosmesh::parse_sht41_frame(bytes);
    TEST_ASSERT_FALSE(sample.ok);
}

void test_sht41_humidity_clamps_to_valid_percent_range() {
    const std::uint8_t zero_bytes[] = {0x00, 0x00};
    const std::uint8_t high_bytes[] = {0xFF, 0xFF};
    const std::uint8_t low_frame[6] = {0xBE, 0xEF, 0x92, 0x00, 0x00,
                                       atmosmesh::sht41_crc8(zero_bytes, 2)};
    const std::uint8_t high_frame[6] = {0xBE, 0xEF, 0x92, 0xFF, 0xFF,
                                        atmosmesh::sht41_crc8(high_bytes, 2)};
    TEST_ASSERT_EQUAL_FLOAT(0.0F, atmosmesh::parse_sht41_frame(low_frame).humidity_pct);
    TEST_ASSERT_EQUAL_FLOAT(100.0F, atmosmesh::parse_sht41_frame(high_frame).humidity_pct);
}

void test_sht41_frame_null_is_unavailable_not_zero() {
    const auto sample = atmosmesh::parse_sht41_frame(nullptr);
    TEST_ASSERT_FALSE(sample.ok);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, sample.temperature_c);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, sample.humidity_pct);
}

}  // namespace

void register_sht41_frame_tests() {
    RUN_TEST(test_sht41_crc8_matches_datasheet_vector);
    RUN_TEST(test_sht41_frame_decodes_known_reading);
    RUN_TEST(test_sht41_frame_rejects_temperature_crc_mismatch);
    RUN_TEST(test_sht41_frame_rejects_humidity_crc_mismatch);
    RUN_TEST(test_sht41_humidity_clamps_to_valid_percent_range);
    RUN_TEST(test_sht41_frame_null_is_unavailable_not_zero);
}

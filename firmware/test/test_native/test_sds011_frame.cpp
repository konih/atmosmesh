#include <cstdint>

#include <unity.h>

#include "atmosmesh/sds011_frame.hpp"

namespace {

// AA C0 | PM25 lo hi | PM10 lo hi | ID | CRC | AB
// PM2.5 = 0x007B = 123 -> 12.3 ug/m3, PM10 = 0x01C8 = 456 -> 45.6 ug/m3.
// CRC = (0x7B + 0x00 + 0xC8 + 0x01 + 0x12 + 0x34) & 0xFF = 0x8A.
constexpr std::uint8_t kGoodFrame[10] = {0xAA, 0xC0, 0x7B, 0x00, 0xC8,
                                         0x01, 0x12, 0x34, 0x8A, 0xAB};

void test_sds011_checksum_matches_hand_computed_vector() {
    TEST_ASSERT_TRUE(atmosmesh::sds011_checksum_ok(kGoodFrame));
}

void test_sds011_frame_decodes_known_reading() {
    const auto sample = atmosmesh::parse_sds011_frame(kGoodFrame);
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 12.3F, sample.pm25_ug_m3);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 45.6F, sample.pm10_ug_m3);
}

void test_sds011_frame_rejects_a_corrupt_checksum() {
    std::uint8_t bad[10];
    for (int i = 0; i < 10; ++i) {
        bad[i] = kGoodFrame[i];
    }
    bad[8] ^= 0x01;
    TEST_ASSERT_FALSE(atmosmesh::sds011_checksum_ok(bad));
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(bad).ok);
}

void test_sds011_frame_rejects_a_payload_bit_flip() {
    std::uint8_t bad[10];
    for (int i = 0; i < 10; ++i) {
        bad[i] = kGoodFrame[i];
    }
    bad[2] ^= 0x01;   // payload moves, checksum does not: this must not read as 12.4
    TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(bad).ok);
}

void test_sds011_frame_rejects_wrong_head_command_and_tail() {
    for (const int index : {0, 1, 9}) {
        std::uint8_t bad[10];
        for (int i = 0; i < 10; ++i) {
            bad[i] = kGoodFrame[i];
        }
        bad[index] = 0x00;
        TEST_ASSERT_FALSE(atmosmesh::parse_sds011_frame(bad).ok);
    }
}

void test_sds011_frame_null_is_unavailable_not_zero() {
    const auto sample = atmosmesh::parse_sds011_frame(nullptr);
    TEST_ASSERT_FALSE(sample.ok);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, sample.pm25_ug_m3);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, sample.pm10_ug_m3);
}

void test_sds011_stream_yields_nothing_until_the_frame_completes() {
    atmosmesh::Sds011Stream stream;
    for (int i = 0; i < 9; ++i) {
        TEST_ASSERT_FALSE(stream.feed(kGoodFrame[i]).ok);
    }
    TEST_ASSERT_TRUE(stream.feed(kGoodFrame[9]).ok);
}

void test_sds011_stream_ignores_leading_noise() {
    atmosmesh::Sds011Stream stream;
    for (const std::uint8_t noise : {0x00, 0xFF, 0x42, 0xC0}) {
        TEST_ASSERT_FALSE(stream.feed(noise).ok);
    }
    atmosmesh::Sds011Sample sample{};
    for (const std::uint8_t byte : kGoodFrame) {
        sample = stream.feed(byte);
    }
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 12.3F, sample.pm25_ug_m3);
}

void test_sds011_stream_resyncs_on_a_repeated_head_byte() {
    atmosmesh::Sds011Stream stream;
    // A stray 0xAA immediately before the real frame must not consume the frame's own head byte.
    TEST_ASSERT_FALSE(stream.feed(0xAA).ok);
    atmosmesh::Sds011Sample sample{};
    for (const std::uint8_t byte : kGoodFrame) {
        sample = stream.feed(byte);
    }
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 45.6F, sample.pm10_ug_m3);
}

void test_sds011_stream_recovers_after_a_truncated_frame() {
    atmosmesh::Sds011Stream stream;
    for (int i = 0; i < 5; ++i) {
        stream.feed(kGoodFrame[i]);
    }
    stream.feed(0x00);   // garbage where the rest of the frame should have been
    atmosmesh::Sds011Sample sample{};
    for (const std::uint8_t byte : kGoodFrame) {
        sample = stream.feed(byte);
    }
    TEST_ASSERT_TRUE(sample.ok);
}

void test_sds011_stream_reads_two_frames_back_to_back() {
    atmosmesh::Sds011Stream stream;
    int completed = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (const std::uint8_t byte : kGoodFrame) {
            if (stream.feed(byte).ok) {
                ++completed;
            }
        }
    }
    TEST_ASSERT_EQUAL_INT(2, completed);
}

void test_sds011_zero_reading_is_valid_not_a_failure() {
    // Clean air is a real answer. CRC = 0x00 + 0x00 + 0x00 + 0x00 + 0x12 + 0x34 = 0x46.
    const std::uint8_t clean[10] = {0xAA, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x46, 0xAB};
    const auto sample = atmosmesh::parse_sds011_frame(clean);
    TEST_ASSERT_TRUE(sample.ok);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, sample.pm25_ug_m3);
}

}  // namespace

void register_sds011_frame_tests() {
    RUN_TEST(test_sds011_checksum_matches_hand_computed_vector);
    RUN_TEST(test_sds011_frame_decodes_known_reading);
    RUN_TEST(test_sds011_frame_rejects_a_corrupt_checksum);
    RUN_TEST(test_sds011_frame_rejects_a_payload_bit_flip);
    RUN_TEST(test_sds011_frame_rejects_wrong_head_command_and_tail);
    RUN_TEST(test_sds011_frame_null_is_unavailable_not_zero);
    RUN_TEST(test_sds011_stream_yields_nothing_until_the_frame_completes);
    RUN_TEST(test_sds011_stream_ignores_leading_noise);
    RUN_TEST(test_sds011_stream_resyncs_on_a_repeated_head_byte);
    RUN_TEST(test_sds011_stream_recovers_after_a_truncated_frame);
    RUN_TEST(test_sds011_stream_reads_two_frames_back_to_back);
    RUN_TEST(test_sds011_zero_reading_is_valid_not_a_failure);
}

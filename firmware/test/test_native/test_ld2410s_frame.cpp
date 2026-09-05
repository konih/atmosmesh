#include <cstdint>
#include <cstring>

#include <unity.h>

#include "atmosmesh/ld2410s_frame.hpp"
#include "atmosmesh/presence_hold.hpp"

namespace {

atmosmesh::Ld2410sReport feed_all(atmosmesh::Ld2410sStream& stream, const std::uint8_t* bytes,
                                  std::size_t len) {
    atmosmesh::Ld2410sReport last{};
    for (std::size_t i = 0; i < len; ++i) {
        const auto r = stream.feed(bytes[i]);
        if (r.ok) {
            last = r;
        }
    }
    return last;
}

void test_ld2410s_minimal_frame_decodes_somebody_with_distance() {
    atmosmesh::Ld2410sStream stream;
    // state 2 = somebody, distance 0x0078 = 120 cm
    const std::uint8_t frame[5] = {0x6E, 0x02, 0x78, 0x00, 0x62};
    atmosmesh::Ld2410sReport last{};
    for (std::size_t i = 0; i < 4; ++i) {
        TEST_ASSERT_FALSE(stream.feed(frame[i]).ok);   // nothing before the tail byte
    }
    last = stream.feed(frame[4]);
    TEST_ASSERT_TRUE(last.ok);
    TEST_ASSERT_TRUE(last.occupied);
    TEST_ASSERT_EQUAL_UINT8(2, last.state);
    TEST_ASSERT_EQUAL_UINT16(120, last.distance_cm);
    TEST_ASSERT_FALSE(last.standard);
}

void test_ld2410s_minimal_frame_states_0_and_1_are_nobody() {
    for (std::uint8_t state = 0; state <= 3; ++state) {
        atmosmesh::Ld2410sStream stream;
        const std::uint8_t frame[5] = {0x6E, state, 0x00, 0x00, 0x62};
        const auto r = feed_all(stream, frame, 5);
        TEST_ASSERT_TRUE(r.ok);
        TEST_ASSERT_EQUAL(state >= 2, r.occupied);
    }
}

void test_ld2410s_stream_resyncs_after_boot_log_noise_and_a_torn_frame() {
    atmosmesh::Ld2410sStream stream;
    // ASCII boot-log bytes, then the tail half of a frame, then a good frame.
    const std::uint8_t noise[] = {'E', 'S', 'P', '-', 'R', 'O', 'M', '\n', 0x00, 0x62,
                                  0x6E, 0x03, 0x2C, 0x01, 0x62};
    const auto r = feed_all(stream, noise, sizeof(noise));
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(r.occupied);
    TEST_ASSERT_EQUAL_UINT16(300, r.distance_cm);
}

void test_ld2410s_distance_bytes_that_look_like_head_or_tail_do_not_confuse_two_frames() {
    atmosmesh::Ld2410sStream stream;
    // distance 0x006E (110 cm) then distance 0x0062 (98 cm), back to back.
    const std::uint8_t frames[10] = {0x6E, 0x02, 0x6E, 0x00, 0x62, 0x6E, 0x02, 0x62, 0x00, 0x62};
    int reports = 0;
    std::uint16_t seen[2] = {0, 0};
    for (std::uint8_t b : frames) {
        const auto r = stream.feed(b);
        if (r.ok && reports < 2) {
            seen[reports++] = r.distance_cm;
        }
    }
    TEST_ASSERT_EQUAL_INT(2, reports);
    TEST_ASSERT_EQUAL_UINT16(110, seen[0]);
    TEST_ASSERT_EQUAL_UINT16(98, seen[1]);
}

void test_ld2410s_rejects_bad_state_or_absurd_distance() {
    atmosmesh::Ld2410sStream stream;
    const std::uint8_t bad_state[5] = {0x6E, 0x09, 0x10, 0x00, 0x62};
    TEST_ASSERT_FALSE(feed_all(stream, bad_state, 5).ok);
    const std::uint8_t too_far[5] = {0x6E, 0x02, 0xFF, 0x7F, 0x62};   // 32767 cm
    TEST_ASSERT_FALSE(feed_all(stream, too_far, 5).ok);
}

void test_ld2410s_standard_frame_decodes_state_and_distance() {
    atmosmesh::Ld2410sStream stream;
    std::uint8_t frame[80];
    std::size_t n = 0;
    const std::uint8_t head[6] = {0xF4, 0xF3, 0xF2, 0xF1, 0x46, 0x00};   // length 70
    std::memcpy(frame, head, 6);
    n = 6;
    frame[n++] = 0x01;   // type
    frame[n++] = 0x03;   // state: somebody
    frame[n++] = 0xF4;   // distance 0x01F4 = 500 cm, low byte happens to equal the head byte
    frame[n++] = 0x01;
    frame[n++] = 0x00;   // reserved
    frame[n++] = 0x00;
    for (int i = 0; i < 64; ++i) {
        frame[n++] = static_cast<std::uint8_t>(i);   // gate energies, includes 0x62 and 0x6E? no: 0..63
    }
    const std::uint8_t tail[4] = {0xF8, 0xF7, 0xF6, 0xF5};
    std::memcpy(frame + n, tail, 4);
    n += 4;
    TEST_ASSERT_EQUAL_UINT32(80, n);
    const auto r = feed_all(stream, frame, n);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(r.standard);
    TEST_ASSERT_TRUE(r.occupied);
    TEST_ASSERT_EQUAL_UINT16(500, r.distance_cm);
}

void test_ld2410s_command_ack_frames_are_not_reports() {
    atmosmesh::Ld2410sStream stream;
    // ACK for "enable configuration": FD FC FB FA · 04 00 · FF 01 · 00 00 · 04 03 02 01
    const std::uint8_t ack[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF,
                                  0x01, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_ASSERT_FALSE(feed_all(stream, ack, 14).ok);
    const std::uint8_t frame[5] = {0x6E, 0x00, 0x00, 0x00, 0x62};
    const auto r = feed_all(stream, frame, 5);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_FALSE(r.occupied);
}

void test_ld2410s_enable_config_ack_is_parsed_with_status_and_value() {
    // Captured on the first unit on 2026-09-05 right after every C3 reset.
    atmosmesh::Ld2410sStream stream;
    const std::uint8_t ack[18] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0xFF, 0x01, 0x00,
                                  0x00, 0x03, 0x00, 0x80, 0x00, 0x04, 0x03, 0x02, 0x01};
    atmosmesh::Ld2410sReport last{};
    for (std::size_t i = 0; i < 17; ++i) {
        const auto r = stream.feed(ack[i]);
        TEST_ASSERT_FALSE(r.ok);
        TEST_ASSERT_FALSE(r.ack.ok);
    }
    last = stream.feed(ack[17]);
    TEST_ASSERT_FALSE(last.ok);   // an ACK is not a presence report
    TEST_ASSERT_TRUE(last.ack.ok);
    TEST_ASSERT_EQUAL_HEX16(atmosmesh::kLd2410sCmdEnableConfig, last.ack.command);
    TEST_ASSERT_EQUAL_UINT32(6, last.ack.value_len);
    TEST_ASSERT_EQUAL_UINT16(0, atmosmesh::ld2410s_ack_status(last.ack));
    TEST_ASSERT_EQUAL_HEX8(0x03, last.ack.value[2]);   // protocol version 3
    TEST_ASSERT_EQUAL_HEX8(0x80, last.ack.value[4]);   // buffer size 0x80
    // Reporting resumes right after: the next minimal frame still decodes.
    const std::uint8_t frame[5] = {0x6E, 0x02, 0x4B, 0x00, 0x62};
    const auto r = feed_all(stream, frame, 5);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_UINT16(75, r.distance_cm);
}

void test_ld2410s_read_version_ack_carries_three_version_words() {
    atmosmesh::Ld2410sStream stream;
    const std::uint8_t ack[18] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x00, 0x01, 0x01,
                                  0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x03, 0x02, 0x01};
    atmosmesh::Ld2410sReport last{};
    for (std::uint8_t b : ack) {
        last = stream.feed(b);
    }
    TEST_ASSERT_TRUE(last.ack.ok);
    TEST_ASSERT_EQUAL_HEX16(atmosmesh::kLd2410sCmdReadVersion, last.ack.command);
    TEST_ASSERT_EQUAL_UINT32(6, last.ack.value_len);
    TEST_ASSERT_EQUAL_UINT8(1, last.ack.value[0]);
    TEST_ASSERT_EQUAL_UINT8(2, last.ack.value[2]);
    TEST_ASSERT_EQUAL_UINT8(3, last.ack.value[4]);
}

void test_ld2410s_build_command_matches_the_protocol_examples() {
    std::uint8_t out[atmosmesh::kLd2410sCommandMaxBytes];
    const std::uint8_t enable_value[2] = {0x01, 0x00};
    std::size_t n = atmosmesh::ld2410s_build_command(out, sizeof(out),
                                                     atmosmesh::kLd2410sCmdEnableConfig,
                                                     enable_value, 2);
    const std::uint8_t expect_enable[14] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF,
                                            0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_ASSERT_EQUAL_UINT32(14, n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expect_enable, out, 14);

    n = atmosmesh::ld2410s_build_command(out, sizeof(out), atmosmesh::kLd2410sCmdEndConfig,
                                         nullptr, 0);
    const std::uint8_t expect_end[12] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00,
                                         0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_ASSERT_EQUAL_UINT32(12, n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expect_end, out, 12);

    n = atmosmesh::ld2410s_build_command(out, sizeof(out), atmosmesh::kLd2410sCmdOutputMode,
                                         atmosmesh::kLd2410sOutputStandard, 6);
    const std::uint8_t expect_mode[18] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x7A, 0x00, 0x00,
                                          0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
    TEST_ASSERT_EQUAL_UINT32(18, n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expect_mode, out, 18);

    TEST_ASSERT_EQUAL_UINT32(0, atmosmesh::ld2410s_build_command(out, 10,
                                                                 atmosmesh::kLd2410sCmdEndConfig,
                                                                 nullptr, 0));
}

void test_presence_hold_debounces_then_holds_after_the_pin_drops() {
    atmosmesh::PresenceHold hold{};
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 0, 50, 5000));
    TEST_ASSERT_FALSE(hold.occupied);
    // A 10 ms blip is not a person.
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, true, 100, 50, 5000));
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 110, 50, 5000));
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 200, 50, 5000));
    TEST_ASSERT_FALSE(hold.occupied);
    // A held HIGH is.
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, true, 300, 50, 5000));
    TEST_ASSERT_TRUE(atmosmesh::presence_hold_update(hold, true, 360, 50, 5000));
    TEST_ASSERT_TRUE(hold.occupied);
    TEST_ASSERT_EQUAL_UINT32(360, hold.occupied_since_ms);
    // Pin drops: occupied is held for hold_ms, then releases.
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 1000, 50, 5000));
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 1100, 50, 5000));
    TEST_ASSERT_TRUE(hold.occupied);
    TEST_ASSERT_FALSE(atmosmesh::presence_hold_update(hold, false, 5000, 50, 5000));
    TEST_ASSERT_TRUE(hold.occupied);
    TEST_ASSERT_TRUE(atmosmesh::presence_hold_update(hold, false, 6100, 50, 5000));
    TEST_ASSERT_FALSE(hold.occupied);
}

void test_presence_hold_first_sample_high_is_occupied_without_waiting() {
    // Booting into an occupied room: the first stable sample is trusted as-is.
    atmosmesh::PresenceHold hold{};
    TEST_ASSERT_TRUE(atmosmesh::presence_hold_update(hold, true, 0, 50, 5000));
    TEST_ASSERT_TRUE(hold.occupied);
}

}  // namespace

void register_ld2410s_frame_tests() {
    RUN_TEST(test_ld2410s_minimal_frame_decodes_somebody_with_distance);
    RUN_TEST(test_ld2410s_minimal_frame_states_0_and_1_are_nobody);
    RUN_TEST(test_ld2410s_stream_resyncs_after_boot_log_noise_and_a_torn_frame);
    RUN_TEST(test_ld2410s_distance_bytes_that_look_like_head_or_tail_do_not_confuse_two_frames);
    RUN_TEST(test_ld2410s_rejects_bad_state_or_absurd_distance);
    RUN_TEST(test_ld2410s_standard_frame_decodes_state_and_distance);
    RUN_TEST(test_ld2410s_command_ack_frames_are_not_reports);
    RUN_TEST(test_ld2410s_enable_config_ack_is_parsed_with_status_and_value);
    RUN_TEST(test_ld2410s_read_version_ack_carries_three_version_words);
    RUN_TEST(test_ld2410s_build_command_matches_the_protocol_examples);
    RUN_TEST(test_presence_hold_debounces_then_holds_after_the_pin_drops);
    RUN_TEST(test_presence_hold_first_sample_high_is_occupied_without_waiting);
}

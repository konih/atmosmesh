#include <unity.h>
#include <initializer_list>
#include "atmosmesh/jiggler.hpp"

using atmosmesh::Jiggler;
void setUp() {}
void tearDown() {}

static void press(Jiggler& jig, std::uint32_t t) {
    jig.update(t, true, true);
    jig.update(t + 30, true, true);
}
static void release(Jiggler& jig, std::uint32_t t) {
    jig.update(t, true, false);
    jig.update(t + 30, true, false);
}
void starts_off_and_ignores_button_held_at_boot() {
    Jiggler jig;
    TEST_ASSERT_EQUAL_INT8(0, jig.update(0, true, true));
    jig.update(1000, true, true);
    TEST_ASSERT_FALSE(jig.enabled());
    release(jig, 1100);
    press(jig, 1200);
    TEST_ASSERT_TRUE(jig.enabled());
}
void bounce_and_long_hold_toggle_only_once() {
    Jiggler jig;
    release(jig, 0);
    jig.update(100, true, true);
    jig.update(110, true, false);
    jig.update(120, true, true);
    jig.update(149, true, true);
    TEST_ASSERT_FALSE(jig.enabled());
    jig.update(150, true, true);
    TEST_ASSERT_TRUE(jig.enabled());
    jig.update(1000, true, true);
    TEST_ASSERT_TRUE(jig.enabled());
    release(jig, 1100);
    press(jig, 1200);
    TEST_ASSERT_FALSE(jig.enabled());
}
void movement_is_small_paired_and_periodic() {
    Jiggler jig;
    release(jig, 0);
    press(jig, 100);
    release(jig, 200);
    TEST_ASSERT_EQUAL_INT8(0, jig.update(15129, true, false));
    TEST_ASSERT_EQUAL_INT8(1, jig.update(15130, true, false));
    TEST_ASSERT_EQUAL_INT8(0, jig.update(15209, true, false));
    TEST_ASSERT_EQUAL_INT8(-1, jig.update(15210, true, false));
    TEST_ASSERT_EQUAL_INT8(0, jig.update(30129, true, false));
    TEST_ASSERT_EQUAL_INT8(1, jig.update(30130, true, false));
}
void disconnect_cancels_pending_move_and_requires_new_press() {
    Jiggler jig;
    release(jig, 0);
    press(jig, 100);
    release(jig, 200);
    TEST_ASSERT_EQUAL_INT8(1, jig.update(15130, true, false));
    TEST_ASSERT_EQUAL_INT8(0, jig.update(15140, false, false));
    TEST_ASSERT_FALSE(jig.enabled());
    TEST_ASSERT_EQUAL_INT8(0, jig.update(20000, true, false));
    press(jig, 21000);
    TEST_ASSERT_TRUE(jig.enabled());
}
void offline_press_does_not_arm_future_connection() {
    Jiggler jig;
    jig.update(0, false, false);
    jig.update(100, false, true);
    jig.update(130, false, true);
    jig.update(200, true, true);
    TEST_ASSERT_FALSE(jig.enabled());
}
void disable_cancels_pending_return() {
    Jiggler jig;
    release(jig, 0);
    press(jig, 100);
    release(jig, 200);
    TEST_ASSERT_EQUAL_INT8(1, jig.update(15130, true, false));
    press(jig, 15140);
    TEST_ASSERT_FALSE(jig.enabled());
    TEST_ASSERT_EQUAL_INT8(0, jig.update(16000, true, true));
}
void timers_survive_millis_wrap_without_catchup_bursts() {
    Jiggler jig;
    release(jig, 0xfffff000U);
    press(jig, 0xfffff100U);
    release(jig, 0xfffff200U);
    TEST_ASSERT_EQUAL_INT8(1, jig.update(0xfffff11eU + 15000U, true, false));
    TEST_ASSERT_EQUAL_INT8(-1, jig.update(0xfffff11eU + 90000U, true, false));
    TEST_ASSERT_EQUAL_INT8(0, jig.update(0xfffff11eU + 90001U, true, false));
}
void hid_requires_encryption_and_subscription() {
    atmosmesh::JigglerHid hid;
    hid.authenticated(true);
    TEST_ASSERT_FALSE(hid.ready());
    hid.subscribed(true);
    TEST_ASSERT_TRUE(hid.ready());
    hid.authenticated(false);
    TEST_ASSERT_FALSE(hid.ready());
}
void mouse_report_map_is_a_standard_three_button_relative_mouse() {
    const auto& map = atmosmesh::kMouseReportMap;
    TEST_ASSERT_GREATER_THAN(20, static_cast<int>(sizeof(map)));
    bool saw_buttons = false, saw_x = false, saw_y = false, saw_keyboard = false;
    for (unsigned i = 0; i + 1 < sizeof(map); ++i) {
        if (map[i] == 0x05 && map[i + 1] == 0x09) saw_buttons = true;
        if (map[i] == 0x09 && map[i + 1] == 0x30) saw_x = true;
        if (map[i] == 0x09 && map[i + 1] == 0x31) saw_y = true;
        if (map[i] == 0x05 && map[i + 1] == 0x01 && i + 3 < sizeof(map) &&
            map[i + 2] == 0x09 && map[i + 3] == 0x06)
            saw_keyboard = true;
    }
    TEST_ASSERT_TRUE(saw_buttons);
    TEST_ASSERT_TRUE(saw_x);
    TEST_ASSERT_TRUE(saw_y);
    TEST_ASSERT_FALSE(saw_keyboard);
}

void hid_reports_only_xy_and_disarms_on_send_failure() {
    atmosmesh::JigglerHid hid;
    hid.authenticated(true);
    hid.subscribed(true);
    int calls = 0;
    auto send = [&](const uint8_t* data, unsigned size) {
        ++calls;
        TEST_ASSERT_EQUAL_UINT(3, size);
        TEST_ASSERT_EQUAL_UINT8(0, data[0]);
        TEST_ASSERT_EQUAL_UINT8(1, data[1]);
        TEST_ASSERT_EQUAL_UINT8(0, data[2]);
        return false;
    };
    hid.update(0, false, send);
    hid.update(100, true, send);
    hid.update(130, true, send);
    TEST_ASSERT_TRUE(hid.enabled());
    TEST_ASSERT_FALSE(hid.update(15130, false, send));
    TEST_ASSERT_EQUAL_INT(1, calls);
    TEST_ASSERT_FALSE(hid.enabled());
    hid.update(15210, false, send);
    TEST_ASSERT_EQUAL_INT(1, calls);
}
void hid_disconnect_latch_survives_fast_reconnect() {
    atmosmesh::JigglerHid hid;
    hid.authenticated(true); hid.subscribed(true);
    auto send = [](const uint8_t*, unsigned) { return true; };
    hid.update(0, false, send);
    hid.update(100, true, send); hid.update(130, true, send);
    TEST_ASSERT_TRUE(hid.enabled());
    hid.disconnected();
    TEST_ASSERT_FALSE(hid.ready());
    hid.authenticated(true); hid.subscribed(true);
    hid.update(140, true, send);
    TEST_ASSERT_FALSE(hid.enabled());
    hid.update(200, false, send); hid.update(230, false, send);
    hid.update(300, true, send); hid.update(330, true, send);
    TEST_ASSERT_TRUE(hid.enabled());
    hid.subscribed(false);
    hid.subscribed(true);
    hid.update(340, true, send);
    TEST_ASSERT_FALSE(hid.enabled());
}
void hid_emits_signed_return_without_extra_reports() {
    atmosmesh::JigglerHid hid;
    hid.authenticated(true); hid.subscribed(true);
    int calls = 0;
    auto send = [&](const uint8_t* data, unsigned size) {
        TEST_ASSERT_EQUAL_UINT(3, size);
        TEST_ASSERT_EQUAL_UINT8(0, data[0]);
        TEST_ASSERT_EQUAL_UINT8(calls == 0 ? 1 : 255, data[1]);
        TEST_ASSERT_EQUAL_UINT8(0, data[2]);
        ++calls;
        return true;
    };
    hid.update(0, false, send);
    hid.update(100, true, send); hid.update(130, true, send);
    hid.update(15130, false, send); hid.update(15210, false, send);
    hid.update(15211, false, send);
    TEST_ASSERT_EQUAL_INT(2, calls);
}
void hid_cannot_arm_or_send_with_partial_connection() {
    for (bool encrypted : {false, true}) {
        atmosmesh::JigglerHid hid;
        hid.authenticated(encrypted);
        hid.subscribed(!encrypted);
        int sends = 0;
        auto send = [&](const uint8_t*, unsigned) { ++sends; return true; };
        hid.update(0, false, send);
        hid.update(100, true, send); hid.update(130, true, send);
        TEST_ASSERT_FALSE(hid.enabled());
        hid.update(15130, true, send);
        TEST_ASSERT_EQUAL_INT(0, sends);
        hid.authenticated(true); hid.subscribed(true);
        hid.update(16000, true, send);
        TEST_ASSERT_FALSE(hid.enabled());
        hid.update(17000, false, send); hid.update(17030, false, send);
        hid.update(18000, true, send); hid.update(18030, true, send);
        TEST_ASSERT_TRUE(hid.enabled());
        hid.update(33030, true, send);
        TEST_ASSERT_EQUAL_INT(1, sends);
    }
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(starts_off_and_ignores_button_held_at_boot);
    RUN_TEST(bounce_and_long_hold_toggle_only_once);
    RUN_TEST(movement_is_small_paired_and_periodic);
    RUN_TEST(disconnect_cancels_pending_move_and_requires_new_press);
    RUN_TEST(offline_press_does_not_arm_future_connection);
    RUN_TEST(disable_cancels_pending_return);
    RUN_TEST(timers_survive_millis_wrap_without_catchup_bursts);
    RUN_TEST(hid_requires_encryption_and_subscription);
    RUN_TEST(mouse_report_map_is_a_standard_three_button_relative_mouse);
    RUN_TEST(hid_reports_only_xy_and_disarms_on_send_failure);
    RUN_TEST(hid_disconnect_latch_survives_fast_reconnect);
    RUN_TEST(hid_emits_signed_return_without_extra_reports);
    RUN_TEST(hid_cannot_arm_or_send_with_partial_connection);
    return UNITY_END();
}

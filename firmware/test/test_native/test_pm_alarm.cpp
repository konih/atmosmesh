#include <limits>

#include <unity.h>

#include "atmosmesh/pm_alarm.hpp"

namespace {

using atmosmesh::PmAlarm;
using atmosmesh::PmLevel;

void test_pm_alarm_starts_unknown_not_good() {
    PmAlarm alarm;
    // Silence is not clean air. Before any frame arrives the level must not read Good.
    TEST_ASSERT_TRUE(alarm.level() == PmLevel::Unknown);
    TEST_ASSERT_FALSE(alarm.high());
}

void test_pm_alarm_beeps_on_crossing_into_high() {
    PmAlarm alarm;
    TEST_ASSERT_FALSE(alarm.update(5.0F, 8.0F, 1000UL));
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 2000UL));
    TEST_ASSERT_TRUE(alarm.high());
}

void test_pm_alarm_does_not_beep_every_sample_while_high() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 1000UL));
    for (unsigned long t = 2000UL; t < 61000UL; t += 1000UL) {
        TEST_ASSERT_FALSE(alarm.update(40.0F, 10.0F, t));
    }
}

void test_pm_alarm_reannounces_after_the_interval() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 1000UL));
    TEST_ASSERT_FALSE(alarm.update(40.0F, 10.0F, 60999UL));
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 61000UL));
}

void test_pm_alarm_holds_high_inside_the_hysteresis_band() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 1000UL));
    // 30 is below the 35 alarm level but above the 25 clear level: the band, so High is held.
    alarm.update(30.0F, 10.0F, 2000UL);
    TEST_ASSERT_TRUE(alarm.high());
    // Dropping to the clear level finally releases it.
    alarm.update(25.0F, 10.0F, 3000UL);
    TEST_ASSERT_FALSE(alarm.high());
    TEST_ASSERT_TRUE(alarm.level() == PmLevel::Good);
}

void test_pm_alarm_band_does_not_chatter_the_beeper() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(36.0F, 10.0F, 1000UL));
    // A reading oscillating across the alarm level must not produce a beep per sample.
    for (unsigned long t = 2000UL; t < 20000UL; t += 1000UL) {
        const float pm25 = (t / 1000UL) % 2 == 0 ? 34.0F : 36.0F;
        TEST_ASSERT_FALSE(alarm.update(pm25, 10.0F, t));
    }
}

void test_pm_alarm_reclears_then_realarms() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 1000UL));
    TEST_ASSERT_FALSE(alarm.update(5.0F, 5.0F, 2000UL));
    // A genuine new event announces immediately, without waiting out the re-announce interval.
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 3000UL));
}

void test_pm_alarm_pm10_alone_can_raise_it() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(2.0F, 60.0F, 1000UL));
    TEST_ASSERT_TRUE(alarm.high());
}

void test_pm_alarm_clearing_needs_both_channels_calm() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 60.0F, 1000UL));
    // PM2.5 is calm but PM10 is still above its clear level, so the alarm stands.
    alarm.update(5.0F, 50.0F, 2000UL);
    TEST_ASSERT_TRUE(alarm.high());
    alarm.update(5.0F, 40.0F, 3000UL);
    TEST_ASSERT_FALSE(alarm.high());
}

void test_pm_alarm_no_data_is_unknown_not_good() {
    PmAlarm alarm;
    alarm.update(5.0F, 5.0F, 1000UL);
    TEST_ASSERT_TRUE(alarm.level() == PmLevel::Good);
    alarm.mark_no_data();
    TEST_ASSERT_TRUE(alarm.level() == PmLevel::Unknown);
}

void test_pm_alarm_announces_afresh_after_a_dropout() {
    PmAlarm alarm;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 1000UL));
    alarm.mark_no_data();
    // The sensor came back still dirty. That must beep, not be swallowed by the old timer.
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 2000UL));
}

void test_pm_alarm_unknown_plus_band_reading_is_good() {
    PmAlarm alarm;
    // 30 sits in the band with no previous level to hold, so it must resolve to Good, not High.
    TEST_ASSERT_FALSE(alarm.update(30.0F, 10.0F, 1000UL));
    TEST_ASSERT_TRUE(alarm.level() == PmLevel::Good);
}

void test_pm_alarm_survives_millis_rollover() {
    PmAlarm alarm;
    const unsigned long near_max = std::numeric_limits<unsigned long>::max() - 100UL;
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, near_max));
    // 201 ms later on the far side of the wrap: far too soon to re-announce.
    TEST_ASSERT_FALSE(alarm.update(40.0F, 10.0F, 100UL));
    // Exactly the re-announce interval after the beep, still across the wrap.
    TEST_ASSERT_TRUE(alarm.update(40.0F, 10.0F, 59899UL));
}

}  // namespace

void register_pm_alarm_tests() {
    RUN_TEST(test_pm_alarm_starts_unknown_not_good);
    RUN_TEST(test_pm_alarm_beeps_on_crossing_into_high);
    RUN_TEST(test_pm_alarm_does_not_beep_every_sample_while_high);
    RUN_TEST(test_pm_alarm_reannounces_after_the_interval);
    RUN_TEST(test_pm_alarm_holds_high_inside_the_hysteresis_band);
    RUN_TEST(test_pm_alarm_band_does_not_chatter_the_beeper);
    RUN_TEST(test_pm_alarm_reclears_then_realarms);
    RUN_TEST(test_pm_alarm_pm10_alone_can_raise_it);
    RUN_TEST(test_pm_alarm_clearing_needs_both_channels_calm);
    RUN_TEST(test_pm_alarm_no_data_is_unknown_not_good);
    RUN_TEST(test_pm_alarm_announces_afresh_after_a_dropout);
    RUN_TEST(test_pm_alarm_unknown_plus_band_reading_is_good);
    RUN_TEST(test_pm_alarm_survives_millis_rollover);
}

#include "atmosmesh/pm_alarm.hpp"

namespace atmosmesh {

PmAlarm::PmAlarm(PmThresholds thresholds, unsigned long reannounce_ms)
    : thresholds_(thresholds),
      reannounce_ms_(reannounce_ms),
      level_(PmLevel::Unknown),
      last_beep_ms_(0),
      ever_beeped_(false) {}

bool PmAlarm::update(float pm25_ug_m3, float pm10_ug_m3, unsigned long now_ms) {
    // Either channel can raise the alarm; both must be calm to clear it.
    const bool over = pm25_ug_m3 >= thresholds_.pm25_alarm_ug_m3 ||
                      pm10_ug_m3 >= thresholds_.pm10_alarm_ug_m3;
    const bool under = pm25_ug_m3 <= thresholds_.pm25_clear_ug_m3 &&
                       pm10_ug_m3 <= thresholds_.pm10_clear_ug_m3;

    PmLevel next = level_;
    if (over) {
        next = PmLevel::High;
    } else if (under || level_ == PmLevel::Unknown) {
        // Between the two thresholds with a level already known, the previous level is held.
        // That holding is the hysteresis.
        next = PmLevel::Good;
    }

    // Unsigned subtraction throughout, so a millis() rollover is a correct small difference
    // rather than a ~49 day silence.
    const bool interval_elapsed = !ever_beeped_ || now_ms - last_beep_ms_ >= reannounce_ms_;

    bool beep = false;
    if (next == PmLevel::High) {
        if (level_ == PmLevel::Good) {
            // A crossing from air known to be clean is a real event and always announces.
            beep = true;
        } else if (level_ == PmLevel::Unknown) {
            // Coming back from a dropout, where the air may have been high the whole time. A
            // sensor that flaps every few seconds would otherwise beep once per recovery, which
            // is precisely the spam the re-announce interval exists to prevent.
            beep = interval_elapsed;
        } else {
            beep = interval_elapsed;   // still high, and it has been quiet long enough to repeat
        }
    }

    level_ = next;
    if (beep) {
        last_beep_ms_ = now_ms;
        ever_beeped_ = true;
    }
    return beep;
}

void PmAlarm::mark_no_data() {
    // The level is forgotten, but the re-announce timer deliberately is not: losing frames must
    // not become a way to earn an extra beep.
    level_ = PmLevel::Unknown;
}

}  // namespace atmosmesh

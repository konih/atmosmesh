#pragma once

namespace atmosmesh {

// When the beeper is allowed to sound for particulate levels.
//
// Two separate thresholds per channel on purpose. A single threshold makes a reading that happens
// to sit on it chatter the beeper once a second, because the SDS011 reports at 1 Hz and real air
// does not hold still. The alarm level is where it starts; the clear level is where it stops.
struct PmThresholds {
    float pm25_alarm_ug_m3;
    float pm25_clear_ug_m3;
    float pm10_alarm_ug_m3;
    float pm10_clear_ug_m3;
};

// PM2.5 35 ug/m3 is the US AQI "unhealthy for sensitive groups" boundary; PM10 55 is the matching
// step on that scale. Both clear well below, which is what gives the band its width.
inline constexpr PmThresholds kDefaultPmThresholds{35.0F, 25.0F, 55.0F, 40.0F};

inline constexpr unsigned long kDefaultPmReannounceMs = 60000UL;

enum class PmLevel { Unknown, Good, High };

class PmAlarm {
public:
    explicit PmAlarm(PmThresholds thresholds = kDefaultPmThresholds,
                     unsigned long reannounce_ms = kDefaultPmReannounceMs);

    // Feed one CRC-validated sample. Returns true only on the samples where the beeper should
    // sound: once when the air crosses into High, then at most once per reannounce_ms while it
    // stays there. A room that is genuinely bad must not beep sixty times a minute.
    bool update(float pm25_ug_m3, float pm10_ug_m3, unsigned long now_ms);

    // No valid frame for a while. The level goes back to Unknown so that silence is never
    // displayed as clean air. The re-announce timer keeps running across the gap on purpose: a
    // sensor that drops out every few seconds in genuinely bad air would otherwise beep once per
    // recovery. A dropout that outlasts the interval still announces on return.
    void mark_no_data();

    PmLevel level() const { return level_; }
    bool high() const { return level_ == PmLevel::High; }

private:
    PmThresholds thresholds_;
    unsigned long reannounce_ms_;
    PmLevel level_;
    unsigned long last_beep_ms_;
    bool ever_beeped_;
};

}  // namespace atmosmesh

#pragma once

namespace atmosmesh {

enum class GroveLedStatus {
    SensorFault,
    LocalOnly,
    Healthy,
};

enum class LedPolarity {
    CommonCathode,
    CommonAnode,
};

struct LedPinLevels {
    bool red_high = false;
    bool green_high = false;
};

GroveLedStatus grove_led_status(bool core_sensors_ok, bool acquisition_failed, bool mqtt_up);
LedPinLevels grove_led_pin_levels(GroveLedStatus status, LedPolarity polarity);
LedPinLevels grove_led_off_levels(LedPolarity polarity);
const char* grove_led_status_text(GroveLedStatus status);
const char* led_polarity_text(LedPolarity polarity);

}  // namespace atmosmesh

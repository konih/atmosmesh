#include "atmosmesh/status_led.hpp"

namespace atmosmesh {

GroveLedStatus grove_led_status(bool core_sensors_ok, bool acquisition_failed, bool mqtt_up) {
    if (!core_sensors_ok || acquisition_failed) {
        return GroveLedStatus::SensorFault;
    }
    return mqtt_up ? GroveLedStatus::Healthy : GroveLedStatus::LocalOnly;
}

LedPinLevels grove_led_pin_levels(GroveLedStatus status, LedPolarity polarity) {
    const bool red_on = status != GroveLedStatus::Healthy;
    const bool green_on = status != GroveLedStatus::SensorFault;
    if (polarity == LedPolarity::CommonAnode) {
        return {!red_on, !green_on};
    }
    return {red_on, green_on};
}

LedPinLevels grove_led_off_levels(LedPolarity polarity) {
    const bool off_high = polarity == LedPolarity::CommonAnode;
    return {off_high, off_high};
}

const char* grove_led_status_text(GroveLedStatus status) {
    switch (status) {
        case GroveLedStatus::SensorFault:
            return "red/sensor-fault";
        case GroveLedStatus::LocalOnly:
            return "amber/mqtt-offline";
        case GroveLedStatus::Healthy:
            return "green/healthy";
    }
    return "red/sensor-fault";
}

const char* led_polarity_text(LedPolarity polarity) {
    return polarity == LedPolarity::CommonAnode ? "common-anode active-LOW"
                                                : "common-cathode active-HIGH";
}

}  // namespace atmosmesh

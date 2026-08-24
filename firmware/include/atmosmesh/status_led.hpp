#pragma once

#include <string>

#include "atmosmesh/soil_status.hpp"

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

enum class GroveLedReason {
    CoreSensorError,
    AcquisitionError,
    MqttConfiguredOffline,
    SoilDry,
    SoilNeedsWatering,
    SoilSampleMissing,
    SoilCalibrationNeeded,
    SoilAcceptable,
};

struct GroveLedInputs {
    bool core_sensors_ok = false;
    bool acquisition_failed = false;
    bool mqtt_configured = false;
    bool mqtt_up = false;
    SoilCondition soil_condition = SoilCondition::Missing;
};

struct GroveLedDecision {
    GroveLedStatus status = GroveLedStatus::SensorFault;
    GroveLedReason reason = GroveLedReason::CoreSensorError;
};

GroveLedDecision grove_led_decision(const GroveLedInputs& inputs);
LedPinLevels grove_led_pin_levels(GroveLedStatus status, LedPolarity polarity);
LedPinLevels grove_led_off_levels(LedPolarity polarity);
const char* grove_led_status_text(GroveLedStatus status);
const char* grove_led_reason_text(GroveLedReason reason);
const char* led_polarity_text(LedPolarity polarity);
std::string soil_led_status_text(const GroveLedDecision& decision,
                                 const SoilClassification& classification,
                                 const SoilCalibration& calibration);

}  // namespace atmosmesh

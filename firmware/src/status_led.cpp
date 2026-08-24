#include "atmosmesh/status_led.hpp"

#include <sstream>

namespace atmosmesh {

GroveLedDecision grove_led_decision(const GroveLedInputs& inputs) {
    if (!inputs.core_sensors_ok) {
        return {GroveLedStatus::SensorFault, GroveLedReason::CoreSensorError};
    }
    if (inputs.acquisition_failed || inputs.soil_condition == SoilCondition::AcquisitionFailed) {
        return {GroveLedStatus::SensorFault, GroveLedReason::AcquisitionError};
    }
    if (inputs.mqtt_configured && !inputs.mqtt_up) {
        return {GroveLedStatus::SensorFault, GroveLedReason::MqttConfiguredOffline};
    }
    switch (inputs.soil_condition) {
        case SoilCondition::Dry:
            return {GroveLedStatus::SensorFault, GroveLedReason::SoilDry};
        case SoilCondition::NeedsWatering:
            return {GroveLedStatus::LocalOnly, GroveLedReason::SoilNeedsWatering};
        case SoilCondition::Missing:
            return {GroveLedStatus::LocalOnly, GroveLedReason::SoilSampleMissing};
        case SoilCondition::Unclassified:
            return {GroveLedStatus::LocalOnly, GroveLedReason::SoilCalibrationNeeded};
        case SoilCondition::Acceptable:
            return {GroveLedStatus::Healthy, GroveLedReason::SoilAcceptable};
        case SoilCondition::AcquisitionFailed:
            return {GroveLedStatus::SensorFault, GroveLedReason::AcquisitionError};
    }
    return {GroveLedStatus::SensorFault, GroveLedReason::AcquisitionError};
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
            return "red";
        case GroveLedStatus::LocalOnly:
            return "amber";
        case GroveLedStatus::Healthy:
            return "green";
    }
    return "red";
}

const char* grove_led_reason_text(GroveLedReason reason) {
    switch (reason) {
        case GroveLedReason::CoreSensorError:
            return "core-sensor-error";
        case GroveLedReason::AcquisitionError:
            return "acquisition-error";
        case GroveLedReason::MqttConfiguredOffline:
            return "mqtt-configured-offline";
        case GroveLedReason::SoilDry:
            return "soil-dry";
        case GroveLedReason::SoilNeedsWatering:
            return "soil-needs-watering";
        case GroveLedReason::SoilSampleMissing:
            return "soil-sample-missing";
        case GroveLedReason::SoilCalibrationNeeded:
            return "soil-calibration-needed";
        case GroveLedReason::SoilAcceptable:
            return "soil-acceptable";
    }
    return "acquisition-error";
}

const char* led_polarity_text(LedPolarity polarity) {
    return polarity == LedPolarity::CommonAnode ? "common-anode active-LOW"
                                                : "common-cathode active-HIGH";
}

std::string soil_led_status_text(const GroveLedDecision& decision,
                                 const SoilClassification& classification,
                                 const SoilCalibration& calibration) {
    std::ostringstream output;
    output << "soil-led-status: " << grove_led_status_text(decision.status)
           << " reason=" << grove_led_reason_text(decision.reason) << " raw=";
    if (classification.sample_valid) {
        output << classification.raw;
    } else {
        output << "unavailable";
    }
    output << " calibration="
           << soil_calibration_validation_text(classification.validation)
           << " direction=" << soil_direction_text(calibration.direction) << " dry=";
    if (calibration.dry_cutoff_raw >= 0) {
        output << calibration.dry_cutoff_raw;
    } else {
        output << "unset";
    }
    output << " acceptable=";
    if (calibration.acceptable_cutoff_raw >= 0) {
        output << calibration.acceptable_cutoff_raw;
    } else {
        output << "unset";
    }
    return output.str();
}

}  // namespace atmosmesh

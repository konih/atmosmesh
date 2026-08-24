#include "atmosmesh/soil_status.hpp"

namespace atmosmesh {

SoilCalibrationValidation validate_soil_calibration(const SoilCalibration& calibration) {
    if (!calibration.enabled) {
        return SoilCalibrationValidation::Disabled;
    }
    if (calibration.direction != SoilRawDirection::HigherIsWetter &&
        calibration.direction != SoilRawDirection::LowerIsWetter) {
        return SoilCalibrationValidation::InvalidDirection;
    }
    if (calibration.dry_cutoff_raw < 0 || calibration.dry_cutoff_raw > 1023 ||
        calibration.acceptable_cutoff_raw < 0 || calibration.acceptable_cutoff_raw > 1023) {
        return SoilCalibrationValidation::InvalidRange;
    }
    const bool correctly_ordered =
        calibration.direction == SoilRawDirection::HigherIsWetter
            ? calibration.dry_cutoff_raw < calibration.acceptable_cutoff_raw
            : calibration.dry_cutoff_raw > calibration.acceptable_cutoff_raw;
    return correctly_ordered ? SoilCalibrationValidation::Valid
                             : SoilCalibrationValidation::InvalidOrder;
}

SoilClassification classify_soil(const SoilCalibration& calibration, bool sample_valid, int raw,
                                 bool acquisition_failed) {
    SoilClassification result{};
    result.validation = validate_soil_calibration(calibration);
    result.sample_valid = sample_valid;
    result.raw = raw;
    if (acquisition_failed || (sample_valid && (raw < 0 || raw > 1023))) {
        result.condition = SoilCondition::AcquisitionFailed;
        result.sample_valid = false;
        return result;
    }
    if (!sample_valid) {
        result.condition = SoilCondition::Missing;
        return result;
    }
    if (result.validation != SoilCalibrationValidation::Valid) {
        result.condition = SoilCondition::Unclassified;
        return result;
    }
    if (calibration.direction == SoilRawDirection::HigherIsWetter) {
        result.condition = raw <= calibration.dry_cutoff_raw
                               ? SoilCondition::Dry
                               : (raw >= calibration.acceptable_cutoff_raw
                                      ? SoilCondition::Acceptable
                                      : SoilCondition::NeedsWatering);
    } else {
        result.condition = raw >= calibration.dry_cutoff_raw
                               ? SoilCondition::Dry
                               : (raw <= calibration.acceptable_cutoff_raw
                                      ? SoilCondition::Acceptable
                                      : SoilCondition::NeedsWatering);
    }
    return result;
}

const char* soil_direction_text(SoilRawDirection direction) {
    switch (direction) {
        case SoilRawDirection::HigherIsWetter:
            return "higher-is-wetter";
        case SoilRawDirection::LowerIsWetter:
            return "lower-is-wetter";
        case SoilRawDirection::Unknown:
            return "unknown";
    }
    return "unknown";
}

const char* soil_calibration_validation_text(SoilCalibrationValidation validation) {
    switch (validation) {
        case SoilCalibrationValidation::Disabled:
            return "disabled";
        case SoilCalibrationValidation::Valid:
            return "valid";
        case SoilCalibrationValidation::InvalidDirection:
            return "invalid-direction";
        case SoilCalibrationValidation::InvalidRange:
            return "invalid-range";
        case SoilCalibrationValidation::InvalidOrder:
            return "invalid-order";
    }
    return "invalid-direction";
}

}  // namespace atmosmesh

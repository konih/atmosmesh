#pragma once

#ifndef ATMOSMESH_GROVE_SOIL_CALIBRATION_ENABLED
#define ATMOSMESH_GROVE_SOIL_CALIBRATION_ENABLED 0
#endif

#ifndef ATMOSMESH_GROVE_SOIL_RAW_DIRECTION
#define ATMOSMESH_GROVE_SOIL_RAW_DIRECTION 0
#endif

#ifndef ATMOSMESH_GROVE_SOIL_DRY_CUTOFF_RAW
#define ATMOSMESH_GROVE_SOIL_DRY_CUTOFF_RAW -1
#endif

#ifndef ATMOSMESH_GROVE_SOIL_ACCEPTABLE_CUTOFF_RAW
#define ATMOSMESH_GROVE_SOIL_ACCEPTABLE_CUTOFF_RAW -1
#endif

namespace atmosmesh {

enum class SoilRawDirection {
    Unknown = 0,
    HigherIsWetter = 1,
    LowerIsWetter = 2,
};

struct SoilCalibration {
    bool enabled = false;
    SoilRawDirection direction = SoilRawDirection::Unknown;
    int dry_cutoff_raw = -1;
    int acceptable_cutoff_raw = -1;
};

enum class SoilCalibrationValidation {
    Disabled,
    Valid,
    InvalidDirection,
    InvalidRange,
    InvalidOrder,
};

enum class SoilCondition {
    Missing,
    Unclassified,
    AcquisitionFailed,
    Dry,
    NeedsWatering,
    Acceptable,
};

struct SoilClassification {
    SoilCondition condition = SoilCondition::Missing;
    SoilCalibrationValidation validation = SoilCalibrationValidation::Disabled;
    bool sample_valid = false;
    int raw = 0;
};

inline constexpr SoilRawDirection compiled_soil_raw_direction() {
    return ATMOSMESH_GROVE_SOIL_RAW_DIRECTION == 1
               ? SoilRawDirection::HigherIsWetter
               : (ATMOSMESH_GROVE_SOIL_RAW_DIRECTION == 2
                      ? SoilRawDirection::LowerIsWetter
                      : SoilRawDirection::Unknown);
}

inline constexpr SoilCalibration compiled_soil_calibration() {
    return {ATMOSMESH_GROVE_SOIL_CALIBRATION_ENABLED != 0,
            compiled_soil_raw_direction(), ATMOSMESH_GROVE_SOIL_DRY_CUTOFF_RAW,
            ATMOSMESH_GROVE_SOIL_ACCEPTABLE_CUTOFF_RAW};
}

SoilCalibrationValidation validate_soil_calibration(const SoilCalibration& calibration);
SoilClassification classify_soil(const SoilCalibration& calibration, bool sample_valid, int raw,
                                 bool acquisition_failed);
const char* soil_direction_text(SoilRawDirection direction);
const char* soil_calibration_validation_text(SoilCalibrationValidation validation);

}  // namespace atmosmesh

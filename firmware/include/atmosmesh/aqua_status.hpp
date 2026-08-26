#pragma once

#include <array>
#include <string>

#include "atmosmesh/grove_status.hpp"
#include "atmosmesh/soil_sampler.hpp"

namespace atmosmesh {

// Reuses Measurement (grove_status.hpp) for temperature/humidity and SoilAdcMeasurement
// (soil_sampler.hpp) for the raw water-probe ADC value — same shape, same "raw, never a percent"
// discipline as the soil probe (D-010/D-015), so no new measurement types are invented here.
struct AquaReadings {
    Measurement temperature{};
    Measurement humidity{};
    SoilAdcMeasurement water{};
    bool mqtt_up = false;
};

using AquaOledLines = std::array<std::string, 4>;

AquaOledLines aqua_oled_lines(const AquaReadings& readings);
bool aqua_core_sensors_ok(const AquaReadings& readings);

}  // namespace atmosmesh

#pragma once

#include <array>
#include <string>

#include "atmosmesh/rc_light.hpp"
#include "atmosmesh/soil_sampler.hpp"

namespace atmosmesh {

struct Measurement {
    bool valid = false;
    float value = 0.0F;
};

struct GroveReadings {
    Measurement dht_temperature{};
    Measurement humidity{};
    Measurement bmp_temperature{};
    Measurement pressure{};
    RcLightMeasurement light{};
    SoilAdcMeasurement soil{};
};

enum class GroveBmpAction {
    Unavailable,
    Initialize,
    Read,
};

using GroveOledLines = std::array<std::string, 4>;

GroveOledLines grove_oled_lines(const GroveReadings& readings);
std::string grove_health_text(const GroveReadings& readings);
GroveBmpAction grove_bmp_action(bool address_present, bool initialized);
void invalidate_grove_bmp(GroveReadings& readings);
bool grove_core_sensors_ok(const GroveReadings& readings);

}  // namespace atmosmesh

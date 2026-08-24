#include "atmosmesh/grove_status.hpp"

#include <cstdio>

namespace atmosmesh {
namespace {

bool dht_ok(const GroveReadings& readings) {
    return readings.dht_temperature.valid && readings.humidity.valid;
}

bool bmp_ok(const GroveReadings& readings) {
    return readings.bmp_temperature.valid && readings.pressure.valid;
}

}  // namespace

GroveOledLines grove_oled_lines(const GroveReadings& readings) {
    // The 5x7 OLED font fits at least 21 characters across 128 pixels. Bound every line to that
    // contract even if a corrupt/out-of-range sensor value formats longer than expected.
    char environment[22];
    if (readings.dht_temperature.valid && readings.humidity.valid) {
        std::snprintf(environment, sizeof(environment), "T:%.1fC RH:%.0f%%",
                      static_cast<double>(readings.dht_temperature.value),
                      static_cast<double>(readings.humidity.value));
    } else if (readings.dht_temperature.valid) {
        std::snprintf(environment, sizeof(environment), "T:%.1fC RH:--",
                      static_cast<double>(readings.dht_temperature.value));
    } else if (readings.humidity.valid) {
        std::snprintf(environment, sizeof(environment), "T:-- RH:%.0f%%",
                      static_cast<double>(readings.humidity.value));
    } else {
        std::snprintf(environment, sizeof(environment), "T:-- RH:--");
    }

    char pressure[22];
    if (readings.pressure.valid) {
        std::snprintf(pressure, sizeof(pressure), "P:%.1fhPa",
                      static_cast<double>(readings.pressure.value));
    } else {
        std::snprintf(pressure, sizeof(pressure), "P:ERR");
    }

    char light[22];
    if (readings.light.valid) {
        std::snprintf(light, sizeof(light), "Light:%luus",
                      static_cast<unsigned long>(readings.light.charge_us));
    } else {
        std::snprintf(light, sizeof(light), "Light:--");
    }

    char soil[22];
    if (readings.soil.valid) {
        std::snprintf(soil, sizeof(soil), "Soil:%u", static_cast<unsigned>(readings.soil.raw));
    } else {
        std::snprintf(soil, sizeof(soil), "Soil:--");
    }
    return {environment, pressure, light, soil};
}

std::string grove_health_text(const GroveReadings& readings) {
    return std::string("dht=") + (dht_ok(readings) ? "ok" : "error") +
           " bmp=" + (bmp_ok(readings) ? "ok" : "error");
}

GroveBmpAction grove_bmp_action(bool address_present, bool initialized) {
    if (!address_present) {
        return GroveBmpAction::Unavailable;
    }
    return initialized ? GroveBmpAction::Read : GroveBmpAction::Initialize;
}

void invalidate_grove_bmp(GroveReadings& readings) {
    readings.bmp_temperature = {};
    readings.pressure = {};
}

bool grove_core_sensors_ok(const GroveReadings& readings) {
    return dht_ok(readings) && bmp_ok(readings);
}

}  // namespace atmosmesh

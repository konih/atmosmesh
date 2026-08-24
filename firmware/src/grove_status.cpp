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
    char environment[24];
    if (dht_ok(readings)) {
        std::snprintf(environment, sizeof(environment), "T %.1fC  H %.0f%%",
                      static_cast<double>(readings.dht_temperature.value),
                      static_cast<double>(readings.humidity.value));
    } else {
        std::snprintf(environment, sizeof(environment), "T --.-C  H --%%");
    }

    char pressure[24];
    if (bmp_ok(readings)) {
        std::snprintf(pressure, sizeof(pressure), "P %.1f hPa",
                      static_cast<double>(readings.pressure.value));
    } else {
        std::snprintf(pressure, sizeof(pressure), "P ----.- hPa");
    }

    char status[32];
    const char* light = readings.light.valid ? nullptr : "-----";
    const char* soil = readings.soil.valid ? nullptr : "----";
    if (readings.light.valid && readings.soil.valid) {
        std::snprintf(status, sizeof(status), "L%luus S%u D%dB%d",
                      static_cast<unsigned long>(readings.light.charge_us),
                      static_cast<unsigned>(readings.soil.raw),
                      dht_ok(readings) ? 1 : 0, bmp_ok(readings) ? 1 : 0);
    } else if (readings.light.valid) {
        std::snprintf(status, sizeof(status), "L%luus S%s D%dB%d",
                      static_cast<unsigned long>(readings.light.charge_us), soil,
                      dht_ok(readings) ? 1 : 0, bmp_ok(readings) ? 1 : 0);
    } else if (readings.soil.valid) {
        std::snprintf(status, sizeof(status), "L%sus S%u D%dB%d", light,
                      static_cast<unsigned>(readings.soil.raw),
                      dht_ok(readings) ? 1 : 0, bmp_ok(readings) ? 1 : 0);
    } else {
        std::snprintf(status, sizeof(status), "L%sus S%s D%dB%d", light, soil,
                      dht_ok(readings) ? 1 : 0, bmp_ok(readings) ? 1 : 0);
    }
    return {"AtmosMesh Grove", environment, pressure, status};
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

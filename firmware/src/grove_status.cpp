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

    char health[24];
    std::snprintf(health, sizeof(health), "DHT %s BMP %s", dht_ok(readings) ? "OK" : "ERR",
                  bmp_ok(readings) ? "OK" : "ERR");
    return {"AtmosMesh Grove", environment, pressure, health};
}

std::string grove_health_text(const GroveReadings& readings) {
    return std::string("dht=") + (dht_ok(readings) ? "ok" : "error") +
           " bmp=" + (bmp_ok(readings) ? "ok" : "error");
}

}  // namespace atmosmesh

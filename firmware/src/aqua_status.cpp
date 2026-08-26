#include "atmosmesh/aqua_status.hpp"

#include <cstdio>

namespace atmosmesh {

AquaOledLines aqua_oled_lines(const AquaReadings& readings) {
    // The 6x10 OLED font fits at least 21 characters across 128 pixels. Bound every line to that
    // contract even if a corrupt/out-of-range sensor value formats longer than expected.
    char temperature[22];
    if (readings.temperature.valid) {
        std::snprintf(temperature, sizeof(temperature), "T:%.1fC",
                      static_cast<double>(readings.temperature.value));
    } else {
        std::snprintf(temperature, sizeof(temperature), "T:--");
    }

    char humidity[22];
    if (readings.humidity.valid) {
        std::snprintf(humidity, sizeof(humidity), "RH:%.0f%%",
                      static_cast<double>(readings.humidity.value));
    } else {
        std::snprintf(humidity, sizeof(humidity), "RH:--");
    }

    char water[22];
    if (readings.water.valid) {
        std::snprintf(water, sizeof(water), "Water:%u", static_cast<unsigned>(readings.water.raw));
    } else {
        std::snprintf(water, sizeof(water), "Water:--");
    }

    char status[22];
    std::snprintf(status, sizeof(status), "mqtt:%s", readings.mqtt_up ? "ok" : "off");

    return {temperature, humidity, water, status};
}

}  // namespace atmosmesh

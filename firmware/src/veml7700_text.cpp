#include "atmosmesh/veml7700_text.hpp"

#include "atmosmesh/pins.hpp"

#include <cmath>
#include <cstdio>

namespace atmosmesh {

bool has_veml7700_address(const std::uint8_t* found, std::size_t found_count) {
    if (found == nullptr) {
        return false;
    }
    for (std::size_t i = 0; i < found_count; ++i) {
        if (found[i] == kVeml7700Address) {
            return true;
        }
    }
    return false;
}

std::string format_lux_oled(bool present, float lux_lx) {
    if (!present) {
        return "-- lx";
    }
    char cell[16];
    std::snprintf(cell, sizeof(cell), "%d lx",
                  static_cast<int>(std::lround(static_cast<double>(lux_lx))));
    return cell;
}

std::string format_veml7700_serial(bool present, float lux_lx) {
    if (!present) {
        return "veml7700: not found (ok until fitted)";
    }
    char line[48];
    std::snprintf(line, sizeof(line), "veml7700: lux=%.0f", static_cast<double>(lux_lx));
    return line;
}

}  // namespace atmosmesh

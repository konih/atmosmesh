#include "atmosmesh/bmp_address.hpp"

#include <cstdio>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

int pick_bmp_address(const std::uint8_t* found, std::size_t found_count) {
    if (found == nullptr || found_count == 0) {
        return -1;
    }
    for (std::size_t i = 0; i < found_count; ++i) {
        if (found[i] == kBmp280AddressGnd) {
            return static_cast<int>(kBmp280AddressGnd);
        }
    }
    for (std::size_t i = 0; i < found_count; ++i) {
        if (found[i] == kBmp280AddressVdd) {
            return static_cast<int>(kBmp280AddressVdd);
        }
    }
    return static_cast<int>(found[0]);
}

bool is_bmp_family_id(std::uint8_t chip_id) {
    return chip_id == kBmp280ChipId || chip_id == kBme280ChipId;
}

std::string format_bmp280_serial(float temperature_c, float pressure_hpa) {
    char line[64];
    std::snprintf(line, sizeof(line), "bmp280: t=%.1fC p=%.1f hPa",
                  static_cast<double>(temperature_c), static_cast<double>(pressure_hpa));
    return line;
}

}  // namespace atmosmesh

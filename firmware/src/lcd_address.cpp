#include "atmosmesh/lcd_address.hpp"

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

int pick_lcd_address(const std::uint8_t* found, std::size_t found_count) {
    if (found == nullptr || found_count == 0) {
        return -1;
    }
    for (std::size_t i = 0; i < found_count; ++i) {
        for (unsigned candidate : kLcdI2cAddresses) {
            if (candidate == found[i]) {
                return static_cast<int>(found[i]);
            }
        }
    }
    return static_cast<int>(found[0]);
}

}  // namespace atmosmesh

#include "atmosmesh/oled_address.hpp"

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

int pick_oled_address(const std::uint8_t* found, std::size_t found_count) {
    if (found == nullptr || found_count == 0) {
        return -1;
    }
    for (unsigned candidate : kOledI2cAddresses) {
        for (std::size_t i = 0; i < found_count; ++i) {
            if (candidate == found[i]) {
                return static_cast<int>(found[i]);
            }
        }
    }
    return -1;
}

}  // namespace atmosmesh

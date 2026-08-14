#include "atmosmesh/i2c_bus.hpp"

#include <Wire.h>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

std::size_t scan_i2c_bus(TwoWire& bus, const I2cBusMap& pins, std::uint8_t* found,
                         std::size_t found_cap) {
    bus.begin(pins.sda_gpio, pins.scl_gpio);
    bus.setClock(kOledI2cHz);
    delay(50);

    std::size_t count = 0;
    for (std::uint8_t address = 0x08; address < 0x78; ++address) {
        bus.beginTransmission(address);
        if (bus.endTransmission() != 0) {
            continue;
        }
        if (count < found_cap) {
            found[count] = address;
        }
        ++count;
    }
    return count;
}

}  // namespace atmosmesh

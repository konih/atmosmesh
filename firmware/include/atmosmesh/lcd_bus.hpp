#pragma once

#include <cstddef>
#include <cstdint>

class TwoWire;

namespace atmosmesh {

struct I2cBusMap {
    int sda_gpio;
    int scl_gpio;
};

struct I2cDevice {
    std::uint8_t address;
    I2cBusMap pins;
};

std::size_t scan_i2c_bus(TwoWire& bus, const I2cBusMap& pins, std::uint8_t* found,
                         std::size_t found_cap);

}  // namespace atmosmesh

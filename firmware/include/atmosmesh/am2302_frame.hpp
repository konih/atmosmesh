#pragma once

#include <cstdint>

namespace atmosmesh {

struct Am2302Sample {
    float humidity_rh;
    float temperature_c;
    bool ok;
};

// AM2302/DHT22: checksum is the low 8 bits of the sum of the first four bytes.
bool am2302_checksum_ok(const std::uint8_t bytes[5]);

Am2302Sample parse_am2302_frame(const std::uint8_t bytes[5]);

}  // namespace atmosmesh

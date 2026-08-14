#pragma once

#include <cstdint>

namespace atmosmesh {

struct Am2302Sample {
    float humidity_rh;
    float temperature_c;
    bool ok;
};

struct Am2302Hold {
    bool show = false;
    float temperature_c = 0.0F;
    float humidity_rh = 0.0F;
    int consecutive_misses = 0;
};

// AM2302/DHT22: checksum is the low 8 bits of the sum of the first four bytes.
bool am2302_checksum_ok(const std::uint8_t bytes[5]);

Am2302Sample parse_am2302_frame(const std::uint8_t bytes[5]);

// On a good read, store T/RH. On a miss, keep the last good sample until
// consecutive_misses exceeds max_misses (then show becomes false → OLED "--").
bool update_am2302_hold(Am2302Hold& hold, bool read_ok, float temperature_c, float humidity_rh,
                        int max_misses);

}  // namespace atmosmesh

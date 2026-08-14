#include "atmosmesh/am2302_frame.hpp"

namespace atmosmesh {

bool am2302_checksum_ok(const std::uint8_t bytes[5]) {
    if (bytes == nullptr) {
        return false;
    }
    const std::uint8_t sum = static_cast<std::uint8_t>(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    return sum == bytes[4];
}

Am2302Sample parse_am2302_frame(const std::uint8_t bytes[5]) {
    Am2302Sample sample{};
    if (!am2302_checksum_ok(bytes)) {
        return sample;
    }
    const int humidity_raw = (static_cast<int>(bytes[0]) << 8) | bytes[1];
    int temperature_raw = (static_cast<int>(bytes[2] & 0x7F) << 8) | bytes[3];
    if ((bytes[2] & 0x80) != 0) {
        temperature_raw = -temperature_raw;
    }
    sample.humidity_rh = static_cast<float>(humidity_raw) / 10.0F;
    sample.temperature_c = static_cast<float>(temperature_raw) / 10.0F;
    sample.ok = true;
    return sample;
}

}  // namespace atmosmesh

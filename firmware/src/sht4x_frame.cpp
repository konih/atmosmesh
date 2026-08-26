#include "atmosmesh/sht4x_frame.hpp"

namespace atmosmesh {

std::uint8_t sht41_crc8(const std::uint8_t* data, std::size_t len) {
    std::uint8_t crc = 0xFF;
    if (data == nullptr) {
        return crc;
    }
    for (std::size_t i = 0; i < len; ++i) {
        crc = static_cast<std::uint8_t>(crc ^ data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = static_cast<std::uint8_t>((crc << 1) ^ 0x31U);
            } else {
                crc = static_cast<std::uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

Sht41Sample parse_sht41_frame(const std::uint8_t bytes[kSht41FrameBytes]) {
    Sht41Sample sample{};
    if (bytes == nullptr) {
        return sample;
    }
    if (sht41_crc8(bytes, 2) != bytes[2]) {
        return sample;
    }
    if (sht41_crc8(bytes + 3, 2) != bytes[5]) {
        return sample;
    }
    const std::uint16_t t_raw = static_cast<std::uint16_t>((bytes[0] << 8) | bytes[1]);
    const std::uint16_t rh_raw = static_cast<std::uint16_t>((bytes[3] << 8) | bytes[4]);
    sample.temperature_c = -45.0F + 175.0F * (static_cast<float>(t_raw) / 65535.0F);
    float humidity = -6.0F + 125.0F * (static_cast<float>(rh_raw) / 65535.0F);
    if (humidity < 0.0F) {
        humidity = 0.0F;
    } else if (humidity > 100.0F) {
        humidity = 100.0F;
    }
    sample.humidity_pct = humidity;
    sample.ok = true;
    return sample;
}

}  // namespace atmosmesh

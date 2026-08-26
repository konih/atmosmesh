#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// Sensirion SHT4x (SHT41 default variant): I2C address 0x44, no DO pin, no calibration.
inline constexpr std::uint8_t kSht41I2cAddress = 0x44;
// 0xFD = measure T & RH at high repeatability. Datasheet worst-case conversion time is 8.3 ms.
inline constexpr std::uint8_t kSht41MeasureHighPrecisionCmd = 0xFD;
inline constexpr std::uint32_t kSht41MeasureDelayMs = 10U;
inline constexpr std::size_t kSht41FrameBytes = 6U;

struct Sht41Sample {
    float temperature_c = 0.0F;
    float humidity_pct = 0.0F;
    bool ok = false;
};

// Sensirion CRC-8: polynomial 0x31, init 0xFF, no reflection, no final XOR.
std::uint8_t sht41_crc8(const std::uint8_t* data, std::size_t len);

// bytes: [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]. Rejects either half independently on a
// CRC mismatch — never returns ok=true from a partially-corrupt frame.
Sht41Sample parse_sht41_frame(const std::uint8_t bytes[kSht41FrameBytes]);

}  // namespace atmosmesh

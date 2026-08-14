#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace atmosmesh {

// Prefer 0x76 (SDO=GND), then 0x77, else first scan hit.
int pick_bmp_address(const std::uint8_t* found, std::size_t found_count);

bool is_bmp_family_id(std::uint8_t chip_id);

std::string format_bmp280_serial(float temperature_c, float pressure_hpa);

}  // namespace atmosmesh

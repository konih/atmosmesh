#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// Prefer 0x76 (SDO=GND), then 0x77, else first scan hit.
int pick_bmp_address(const std::uint8_t* found, std::size_t found_count);

bool is_bmp_family_id(std::uint8_t chip_id);

}  // namespace atmosmesh

#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// Prefer SSD1306 0x3C, then 0x3D. LCD backpack addresses are not OLED.
int pick_oled_address(const std::uint8_t* found, std::size_t found_count);

}  // namespace atmosmesh

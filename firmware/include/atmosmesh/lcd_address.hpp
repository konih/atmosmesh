#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// Prefer a known LCD backpack address from a scan result; otherwise first hit.
int pick_lcd_address(const std::uint8_t* found, std::size_t found_count);

}  // namespace atmosmesh

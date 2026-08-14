#pragma once

#include <array>
#include <string>
#include <string_view>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

using LcdLines = std::array<std::string, kLcdRows>;

// Truncate to the HD44780 width. Does not pad; the driver clears the row.
std::string clip_lcd_line(std::string_view text);

LcdLines dummy_banner();

}  // namespace atmosmesh

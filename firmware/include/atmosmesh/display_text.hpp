#pragma once

#include <array>
#include <string>
#include <string_view>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

using OledBanner = std::array<std::string, 2>;
using OledLivePage = std::array<std::string, 3>;

int oled_page_count(int height_px);

// Truncate to the Adafruit 6-px glyph columns. Does not pad; the driver clears the panel.
std::string clip_oled_line(std::string_view text);

OledBanner dummy_banner();

OledLivePage live_sensor_lines(bool am_ok, float temperature_c, float humidity_rh, bool bmp_ok,
                               int bmp_address);

}  // namespace atmosmesh

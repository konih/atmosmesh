#pragma once

#include <array>
#include <string>
#include <string_view>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

using OledBanner = std::array<std::string, 2>;
using OledLivePage = std::array<std::string, 3>;

int oled_page_count(int height_px);
int oled_live_line_count(int height_px);
int oled_line_pitch_px(int height_px);
int oled_live_row_y_px(int row);
int oled_telltale_bar_y_px();
int oled_telltale_bar_height_px();
int oled_boot_bar_count();
int oled_boot_bar_y_px(int index);
int oled_boot_bar_hold_ms();

// Truncate to the Adafruit 6-px glyph columns. Does not pad; the driver clears the panel.
std::string clip_oled_line(std::string_view text);

OledBanner dummy_banner();

OledLivePage live_sensor_lines(bool am_ok, float temperature_c, float humidity_rh, bool bmp_ok,
                               float pressure_hpa, bool pm_ok, float pm25_ug_m3, float pm10_ug_m3,
                               int mq135_raw_adc, bool pir_motion = false, bool lux_ok = false,
                               float lux_lx = 0.0F);

}  // namespace atmosmesh

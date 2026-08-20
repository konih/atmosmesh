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

// Which sensor the hPa row's right cell is showing. 21 columns cannot hold lux and MQ135
// at once, so the cell alternates. Lux is slot 0 so a caller that passes no phase keeps
// the pre-rotation layout.
enum class OledRightCell { Lux = 0, Mq = 1 };

// Pure: the caller passes millis(). Slots are kOledRightCellPeriodMs wide.
OledRightCell oled_right_cell_for_ms(unsigned long now_ms);

// Raw ADC count only — never ppm, never CO2. The MQ135 is uncalibrated on this bench and
// a raw of 0 means the AOUT / GND / 5 V heater wiring needs checking, so it reads ERR.
std::string format_mq135_oled(int mq135_raw_adc);

// "MOT" / "idle" rather than a bare flag, so an empty cell is never ambiguous.
std::string format_pir_oled(bool pir_motion);

OledLivePage live_sensor_lines(bool am_ok, float temperature_c, float humidity_rh, bool bmp_ok,
                               float pressure_hpa, bool pm_ok, float pm25_ug_m3, float pm10_ug_m3,
                               int mq135_raw_adc, bool pir_motion = false, bool lux_ok = false,
                               float lux_lx = 0.0F, float bmp_temperature_c = 0.0F,
                               OledRightCell right_cell = OledRightCell::Lux);

}  // namespace atmosmesh

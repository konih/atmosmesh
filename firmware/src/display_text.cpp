#include "atmosmesh/display_text.hpp"
#include "atmosmesh/veml7700_text.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace atmosmesh {

int oled_page_count(int height_px) {
    if (height_px <= 0) {
        return 0;
    }
    return height_px / kOledGlyphHeightPx;
}

int oled_live_line_count(int height_px) {
    return (height_px <= kOledHeightPxAlt) ? 2 : kOledLiveLineCount;
}

int oled_line_pitch_px(int height_px) {
    return (height_px <= kOledHeightPx48) ? kOledLinePitch48Px : kOledLinePitch64Px;
}

int oled_live_row_y_px(int row) {
    if (row < 0 || row >= kOledLiveLineCount) {
        return 0;
    }
    return kOledLiveRowYPx[row];
}

int oled_telltale_bar_y_px() {
    return kOledTelltaleBarYPx;
}

int oled_telltale_bar_height_px() {
    return kOledTelltaleBarHeightPx;
}

int oled_boot_bar_count() {
    return kOledBootBarCount;
}

int oled_boot_bar_y_px(int index) {
    if (index < 0 || index >= kOledBootBarCount) {
        return 0;
    }
    return kOledBootBarYPx[index];
}

int oled_boot_bar_hold_ms() {
    return kOledBootBarHoldMs;
}

namespace {

std::string two_col_width(std::string_view left, std::string_view right, std::size_t left_width) {
    std::string line(left);
    if (line.size() < left_width) {
        line.append(left_width - line.size(), ' ');
    } else if (line.size() > left_width) {
        line.resize(left_width);
    }
    line.append(right);
    return clip_oled_line(line);
}

std::string two_col(std::string_view left, std::string_view right) {
    return two_col_width(left, right, 11);
}

}  // namespace

OledRightCell oled_right_cell_for_ms(unsigned long now_ms) {
    const unsigned long slot = now_ms / kOledRightCellPeriodMs;
    return ((slot % 2UL) == 0UL) ? OledRightCell::Lux : OledRightCell::Mq;
}

std::string format_mq135_oled(int mq135_raw_adc) {
    // Firmware already warns on raw=0 over serial ("check AOUT/GND/5V heater"); surface the
    // same fault on the glass instead of printing a plausible-looking zero.
    if (mq135_raw_adc <= 0) {
        return "MQ ERR";
    }
    // No space after "MQ": a full-scale 4095 must still fit the 6 columns the row can spare.
    char cell[16];
    std::snprintf(cell, sizeof(cell), "MQ%d", mq135_raw_adc);
    return cell;
}

std::string format_pir_oled(bool pir_motion) {
    return pir_motion ? "MOT" : "idle";
}

std::string clip_oled_line(std::string_view text) {
    if (text.size() <= static_cast<std::size_t>(kOledMaxChars)) {
        return std::string(text);
    }
    return std::string(text.substr(0, static_cast<std::size_t>(kOledMaxChars)));
}

OledBanner dummy_banner() {
    return {clip_oled_line("AtmosMesh"), clip_oled_line("OLED bring-up")};
}

OledLivePage live_sensor_lines(bool am_ok, float temperature_c, float humidity_rh, bool bmp_ok,
                               float pressure_hpa, bool pm_ok, float pm25_ug_m3, float pm10_ug_m3,
                               int mq135_raw_adc, bool pir_motion, bool lux_ok, float lux_lx,
                               float bmp_temperature_c, OledRightCell right_cell) {
    char line0[32];
    if (am_ok) {
        std::snprintf(line0, sizeof(line0), "%.1fC  %.0f%% RH", static_cast<double>(temperature_c),
                      static_cast<double>(humidity_rh));
    } else {
        std::snprintf(line0, sizeof(line0), "--C  --%% RH");
    }
    // PIR always reports a word. Worst case "-10.0C  100% RH idle" = 20 of 21 columns.
    {
        const std::string pir_cell = format_pir_oled(pir_motion);
        const std::size_t used = std::strlen(line0);
        if (used + pir_cell.size() + 1 < sizeof(line0) &&
            static_cast<int>(used + pir_cell.size() + 1) <= kOledMaxChars) {
            std::snprintf(line0 + used, sizeof(line0) - used, " %s", pir_cell.c_str());
        }
    }

    // BMP280 contributes both cells here, so one ok flag gates both.
    char hpa_left[24];
    if (bmp_ok) {
        std::snprintf(hpa_left, sizeof(hpa_left), "%.0fhPa %.1fC",
                      static_cast<double>(pressure_hpa), static_cast<double>(bmp_temperature_c));
    } else {
        std::snprintf(hpa_left, sizeof(hpa_left), "--hPa --C");
    }

    const std::string cell = (right_cell == OledRightCell::Mq)
                                 ? format_mq135_oled(mq135_raw_adc)
                                 : format_lux_oled(lux_ok, lux_lx);

    char pm25[16];
    char pm10[16];
    if (pm_ok) {
        std::snprintf(pm25, sizeof(pm25), "PM2.5 %d",
                      static_cast<int>(std::lround(static_cast<double>(pm25_ug_m3))));
        std::snprintf(pm10, sizeof(pm10), "PM10 %d",
                      static_cast<int>(std::lround(static_cast<double>(pm10_ug_m3))));
    } else {
        std::snprintf(pm25, sizeof(pm25), "PM2.5 --");
        std::snprintf(pm10, sizeof(pm10), "PM10 --");
    }

    return {clip_oled_line(line0),
            two_col_width(hpa_left, cell, static_cast<std::size_t>(kOledHpaLeftWidth)),
            two_col(pm25, pm10)};
}

}  // namespace atmosmesh

#include "atmosmesh/display_text.hpp"
#include "atmosmesh/mq135_scale.hpp"

#include <cmath>
#include <cstdio>

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

std::string two_col(std::string_view left, std::string_view right) {
    constexpr std::size_t kLeftWidth = 11;
    std::string line(left);
    if (line.size() < kLeftWidth) {
        line.append(kLeftWidth - line.size(), ' ');
    } else if (line.size() > kLeftWidth) {
        line.resize(kLeftWidth);
    }
    line.append(right);
    return clip_oled_line(line);
}

}  // namespace

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
                               int mq135_raw_adc) {
    char line0[24];
    if (am_ok) {
        std::snprintf(line0, sizeof(line0), "%.1fC  %.0f%% RH", static_cast<double>(temperature_c),
                      static_cast<double>(humidity_rh));
    } else {
        std::snprintf(line0, sizeof(line0), "--C  --%% RH");
    }

    char pressure[16];
    if (bmp_ok) {
        std::snprintf(pressure, sizeof(pressure), "%.0f hPa", static_cast<double>(pressure_hpa));
    } else {
        std::snprintf(pressure, sizeof(pressure), "-- hPa");
    }

    const int gas = mq135_gas_index(mq135_raw_adc);
    char gas_cell[16];
    std::snprintf(gas_cell, sizeof(gas_cell), "g:%d", gas);

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

    return {clip_oled_line(line0), two_col(pressure, gas_cell), two_col(pm25, pm10)};
}

}  // namespace atmosmesh

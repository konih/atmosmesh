#include "atmosmesh/display_text.hpp"
#include "atmosmesh/mq135_scale.hpp"

#include <cstdio>

namespace atmosmesh {

int oled_page_count(int height_px) {
    if (height_px <= 0) {
        return 0;
    }
    return height_px / kOledGlyphHeightPx;
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
                               int mq135_raw_adc) {
    char climate[32];
    if (am_ok) {
        std::snprintf(climate, sizeof(climate), "%4.1fC  %2.0f%% RH",
                      static_cast<double>(temperature_c), static_cast<double>(humidity_rh));
    } else {
        std::snprintf(climate, sizeof(climate), "T --  --%% RH");
    }

    char pressure[32];
    if (bmp_ok) {
        std::snprintf(pressure, sizeof(pressure), "%4.0f hPa", static_cast<double>(pressure_hpa));
    } else {
        std::snprintf(pressure, sizeof(pressure), "-- hPa");
    }

    char pm25[32];
    char pm10[32];
    if (pm_ok) {
        std::snprintf(pm25, sizeof(pm25), "PM2.5 %4.1f", static_cast<double>(pm25_ug_m3));
        std::snprintf(pm10, sizeof(pm10), "PM10 %4.1f", static_cast<double>(pm10_ug_m3));
    } else {
        std::snprintf(pm25, sizeof(pm25), "PM2.5 --");
        std::snprintf(pm10, sizeof(pm10), "PM10 --");
    }

    return {clip_oled_line(climate), clip_oled_line(pressure), clip_oled_line(pm25),
            clip_oled_line(pm10), clip_oled_line(format_mq135_oled_line(mq135_raw_adc))};
}

}  // namespace atmosmesh

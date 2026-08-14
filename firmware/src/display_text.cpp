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
                               int bmp_address, bool pm_ok, float pm25_ug_m3, float pm10_ug_m3,
                               int mq135_raw_adc) {
    char am_line[32];
    if (am_ok) {
        std::snprintf(am_line, sizeof(am_line), "T %4.1fC RH %4.1f%%",
                      static_cast<double>(temperature_c), static_cast<double>(humidity_rh));
    } else {
        std::snprintf(am_line, sizeof(am_line), "AM2302 missing");
    }

    char bmp_line[32];
    if (bmp_ok) {
        std::snprintf(bmp_line, sizeof(bmp_line), "BMP 0x%02X",
                      static_cast<unsigned>(bmp_address) & 0xFFU);
    } else {
        std::snprintf(bmp_line, sizeof(bmp_line), "BMP280 missing");
    }

    char pm_line[32];
    if (pm_ok) {
        std::snprintf(pm_line, sizeof(pm_line), "PM %4.1f/%4.1f", static_cast<double>(pm25_ug_m3),
                      static_cast<double>(pm10_ug_m3));
    } else {
        std::snprintf(pm_line, sizeof(pm_line), "SDS011 missing");
    }

    return {clip_oled_line("AtmosMesh"), clip_oled_line(am_line), clip_oled_line(bmp_line),
            clip_oled_line(pm_line), clip_oled_line(format_mq135_oled_line(mq135_raw_adc))};
}

}  // namespace atmosmesh

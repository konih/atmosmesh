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
    if (am_ok && bmp_ok) {
        std::snprintf(climate, sizeof(climate), "%.1fC %.0f%% %.0fhPa",
                      static_cast<double>(temperature_c), static_cast<double>(humidity_rh),
                      static_cast<double>(pressure_hpa));
    } else if (am_ok) {
        std::snprintf(climate, sizeof(climate), "%.1fC %.0f%% --hPa",
                      static_cast<double>(temperature_c), static_cast<double>(humidity_rh));
    } else if (bmp_ok) {
        std::snprintf(climate, sizeof(climate), "--C --%% %.0fhPa",
                      static_cast<double>(pressure_hpa));
    } else {
        std::snprintf(climate, sizeof(climate), "--C --%% --hPa");
    }

    char particles[32];
    const int gas = mq135_gas_index(mq135_raw_adc);
    if (pm_ok) {
        std::snprintf(particles, sizeof(particles), "2.5:%d 10:%d g:%d",
                      static_cast<int>(std::lround(static_cast<double>(pm25_ug_m3))),
                      static_cast<int>(std::lround(static_cast<double>(pm10_ug_m3))), gas);
    } else {
        std::snprintf(particles, sizeof(particles), "2.5:-- 10:-- g:%d", gas);
    }

    return {clip_oled_line(climate), clip_oled_line(particles)};
}

}  // namespace atmosmesh

#include "atmosmesh/oled_profile.hpp"

#include <cstdio>

#ifndef ATMOSMESH_OLED_CONTROLLER_ID
#define ATMOSMESH_OLED_CONTROLLER_ID 0
#endif

#ifndef ATMOSMESH_OLED_HEIGHT
#define ATMOSMESH_OLED_HEIGHT 64
#endif

namespace atmosmesh {

OledController parse_oled_controller_flag(std::string_view name) {
    if (name == "SH1106") {
        return OledController::Sh1106;
    }
    return OledController::Ssd1306;
}

const char* oled_controller_name(OledController controller) {
    return controller == OledController::Sh1106 ? "SH1106" : "SSD1306";
}

std::uint8_t oled_compins_arg(OledComPins com_pins) {
    return com_pins == OledComPins::Sequential ? 0x02 : 0x12;
}

OledProfile resolve_oled_profile(OledController controller, int height_px) {
    const int height = (height_px == kOledHeightPxAlt) ? kOledHeightPxAlt : kOledHeightPx;
    OledProfile profile{};
    profile.controller = controller;
    profile.width_px = kOledWidthPx;
    profile.height_px = height;
    profile.column_offset_px = (controller == OledController::Sh1106) ? 2 : 0;
    // 128×32 Adafruit init already uses sequential COM; 128×64 clones that miss
    // every other line also need sequential instead of Adafruit's 0x12.
    profile.com_pins = OledComPins::Sequential;
    return profile;
}

OledProfile default_oled_profile() {
    return resolve_oled_profile(OledController::Ssd1306, kOledHeightPx);
}

OledProfile compiled_oled_profile() {
    const auto controller = (ATMOSMESH_OLED_CONTROLLER_ID == 1) ? OledController::Sh1106
                                                                : OledController::Ssd1306;
    return resolve_oled_profile(controller, ATMOSMESH_OLED_HEIGHT);
}

std::string format_oled_init_log(const OledProfile& profile, unsigned address) {
    char line[192];
    std::snprintf(line, sizeof(line),
                  "oled: init ok controller=%s width=%d height=%d addr=0x%02X com=%s i2c_hz=%u",
                  oled_controller_name(profile.controller), profile.width_px, profile.height_px,
                  address & 0xFFU,
                  profile.com_pins == OledComPins::Sequential ? "sequential" : "alternate",
                  static_cast<unsigned>(kOledI2cHz));
    return line;
}

}  // namespace atmosmesh

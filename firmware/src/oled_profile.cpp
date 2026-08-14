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

const char* oled_profile_name(const OledProfile& profile) {
    if (profile.controller == OledController::Sh1106) {
        return "SH1106";
    }
    if (profile.height_px == kOledHeightPxAlt) {
        return "SSD1306_128X32";
    }
    if (profile.com_pins == OledComPins::Sequential) {
        return "SSD1306_ALT0";
    }
    return "SSD1306";
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
    // 0 (default) = SSD1306 ALT0 sequential COM 0x02. 1 = SH1106 compile fallback.
    const auto controller = (ATMOSMESH_OLED_CONTROLLER_ID == 1) ? OledController::Sh1106
                                                                : OledController::Ssd1306;
    return resolve_oled_profile(controller, ATMOSMESH_OLED_HEIGHT);
}

std::string format_oled_init_log(const OledProfile& profile, unsigned address) {
    char line[224];
    std::snprintf(line, sizeof(line),
                  "oled: init ok controller=%s profile=%s width=%d height=%d addr=0x%02X com=%s "
                  "i2c_hz=%u",
                  oled_controller_name(profile.controller), oled_profile_name(profile),
                  profile.width_px, profile.height_px, address & 0xFFU,
                  profile.com_pins == OledComPins::Sequential ? "sequential" : "alternate",
                  static_cast<unsigned>(kOledI2cHz));
    return line;
}

std::string format_oled_display_on_log() {
    return "oled: display on";
}

std::string format_oled_contrast_log() {
    return "oled: contrast 255";
}

std::string format_oled_invert_off_log() {
    return "oled: invert off";
}

std::string format_oled_full_white_log() {
    return "oled: full white";
}

std::string format_oled_text_hi_log() {
    return "oled: text HI";
}

std::string format_oled_mux32_log() {
    return "oled: mux=0x1F (128x32 attempt)";
}

const char* u8g2_hw_i2c_constructor_name(const OledProfile& profile) {
    if (profile.controller == OledController::Sh1106) {
        return "U8G2_SH1106_128X64_NONAME_F_HW_I2C";
    }
    if (profile.height_px == kOledHeightPxAlt) {
        return "U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C";
    }
    return "U8G2_SSD1306_128X64_ALT0_F_HW_I2C";
}

}  // namespace atmosmesh

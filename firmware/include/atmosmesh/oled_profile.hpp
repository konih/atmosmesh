#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

enum class OledController { Ssd1306, Sh1106 };

// Adafruit 128×64 default is Alternate (0x12). Cheap 0.96" modules that drop
// every other row need Sequential (0x02), matching U8g2 SSD1306 ALT0.
enum class OledComPins { Alternate, Sequential };

struct OledProfile {
    OledController controller;
    int width_px;
    int height_px;
    OledComPins com_pins;
    int column_offset_px;
};

OledController parse_oled_controller_flag(std::string_view name);
const char* oled_controller_name(OledController controller);
const char* oled_profile_name(const OledProfile& profile);
std::uint8_t oled_compins_arg(OledComPins com_pins);

OledProfile resolve_oled_profile(OledController controller, int height_px);
OledProfile default_oled_profile();
OledProfile compiled_oled_profile();

std::string format_oled_init_log(const OledProfile& profile, unsigned address);
std::string format_oled_display_on_log();
std::string format_oled_contrast_log();
std::string format_oled_invert_off_log();
std::string format_oled_full_white_log();
std::string format_oled_text_hi_log();
std::string format_oled_mux32_log();
const char* u8g2_hw_i2c_constructor_name(const OledProfile& profile);

inline constexpr std::uint8_t kSsd1306SetMultiplex = 0xA8;
inline constexpr std::uint8_t kSsd1306MuxRatio32 = 0x1F;

}  // namespace atmosmesh

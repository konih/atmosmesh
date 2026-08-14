#include "atmosmesh/display_text.hpp"

namespace atmosmesh {

std::string clip_lcd_line(std::string_view text) {
    if (text.size() <= static_cast<std::size_t>(kLcdColumns)) {
        return std::string(text);
    }
    return std::string(text.substr(0, static_cast<std::size_t>(kLcdColumns)));
}

LcdLines dummy_banner() {
    return {clip_lcd_line("AtmosMesh"), clip_lcd_line("hello, LCD")};
}

}  // namespace atmosmesh

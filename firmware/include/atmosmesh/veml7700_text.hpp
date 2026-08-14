#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace atmosmesh {

bool has_veml7700_address(const std::uint8_t* found, std::size_t found_count);

std::string format_lux_oled(bool present, float lux_lx);
std::string format_veml7700_serial(bool present, float lux_lx);

}  // namespace atmosmesh

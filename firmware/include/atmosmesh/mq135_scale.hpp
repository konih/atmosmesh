#pragma once

#include <string>

#include "atmosmesh/pins.hpp"

namespace atmosmesh {

// GPIO34 millivolts from 12-bit ADC at 11 dB (~3.3 V full scale). Not a gas
// concentration. Never label as CO2 / ppm.
int mq135_gpio_millivolts(int raw_adc);

// Inverse of the 10 kΩ series + 20 kΩ to GND divider (GPIO sees 2/3 of AOUT).
int mq135_aout_millivolts(int gpio_mv);

// Forward map used to warn: 5 V AOUT → 3.33 V on GPIO34 (no ADC headroom).
int mq135_aout_to_gpio_millivolts(int aout_mv);

std::string format_mq135_serial(int raw_adc);
std::string format_mq135_oled_line(int raw_adc);

}  // namespace atmosmesh

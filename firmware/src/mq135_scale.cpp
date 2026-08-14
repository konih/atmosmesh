#include "atmosmesh/mq135_scale.hpp"

#include <algorithm>
#include <cstdio>

namespace atmosmesh {

int mq135_gpio_millivolts(int raw_adc) {
    if (raw_adc < 0) {
        return 0;
    }
    const int raw = std::min(raw_adc, kMq135AdcMax);
    return (raw * kMq135AdcFullScaleMv) / kMq135AdcMax;
}

int mq135_aout_millivolts(int gpio_mv) {
    if (gpio_mv < 0) {
        return 0;
    }
    return gpio_mv * (kMq135SeriesOhms + kMq135GndOhms) / kMq135GndOhms;
}

int mq135_aout_to_gpio_millivolts(int aout_mv) {
    if (aout_mv < 0) {
        return 0;
    }
    return aout_mv * kMq135GndOhms / (kMq135SeriesOhms + kMq135GndOhms);
}

int mq135_gas_index(int raw_adc) {
    if (raw_adc < 0) {
        return 0;
    }
    const int raw = std::min(raw_adc, kMq135AdcMax);
    return (raw * 100) / kMq135AdcMax;
}

std::string format_mq135_serial(int raw_adc) {
    const int gpio_mv = mq135_gpio_millivolts(raw_adc);
    const int aout_mv = mq135_aout_millivolts(gpio_mv);
    char line[192];
    if (raw_adc <= kMq135AdcNearZero) {
        std::snprintf(line, sizeof(line),
                      "mq135: raw=%d gpio_mv=%d aout_mv=%d (check AOUT/GND/5V heater)", raw_adc,
                      gpio_mv, aout_mv);
    } else {
        std::snprintf(line, sizeof(line), "mq135: raw=%d gpio_mv=%d aout_mv=%d", raw_adc, gpio_mv,
                      aout_mv);
    }
    return line;
}

std::string format_mq135_oled_line(int raw_adc) {
    char line[32];
    std::snprintf(line, sizeof(line), "gas %d", mq135_gas_index(raw_adc));
    return line;
}

}  // namespace atmosmesh

#pragma once

#ifndef ATMOSMESH_GROVE_LED_COMMON_ANODE
#define ATMOSMESH_GROVE_LED_COMMON_ANODE 0
#endif

namespace atmosmesh {

struct ProductProfile {
    const char* product_name;
    const char* product_id;
    const char* product_variant;
    const char* station_id;
    int i2c_sda_gpio;
    int i2c_scl_gpio;
    bool i2c_sda_is_bootstrap;
    bool i2c_scl_is_bootstrap;
    int dht_data_gpio;
    int oled_width_px;
    int oled_height_px;
    int light_rc_gpio;
    int status_led_red_gpio;
    int status_led_green_gpio;
    int soil_power_control_gpio;
    int water_power_control_gpio;
    bool status_led_common_anode;
};

// AtmosMesh v1: existing ESP32 station identity and already-established display/DHT pins.
// The legacy ESP32 composition root consumes its existing pin constants; this metadata makes the
// product contract explicit without changing that runtime in the structural migration.
inline constexpr ProductProfile kAtmosMeshV1Profile{
    .product_name = "AtmosMesh",
    .product_id = "atmosmesh-v1",
    .product_variant = "esp32-full-station",
    .station_id = "atmosmesh-0001",
    .i2c_sda_gpio = 5,
    .i2c_scl_gpio = 4,
    .i2c_sda_is_bootstrap = true,
    .i2c_scl_is_bootstrap = false,
    .dht_data_gpio = 18,
    .oled_width_px = 128,
    .oled_height_px = 64,
    .light_rc_gpio = -1,
    .status_led_red_gpio = -1,
    .status_led_green_gpio = -1,
    .soil_power_control_gpio = -1,
    .water_power_control_gpio = -1,
    .status_led_common_anode = false,
};

inline constexpr const ProductProfile& atmosmesh_v1_profile() {
    return kAtmosMeshV1Profile;
}

// AtmosMesh Grove v1.5: D2=GPIO4, D3=GPIO0, D5=GPIO14, D7=GPIO13,
// LED red D6=GPIO12 / green D0=GPIO16, soil PNP base control D1=GPIO5.
// GPIO0 must remain high during reset or the ESP8266 enters ROM download mode.
inline constexpr ProductProfile kGroveProfile{
    .product_name = "AtmosMesh Grove",
    .product_id = "atmosmesh-grove-v1.5",
    .product_variant = "atmosmesh-v1.5",
    .station_id = "atmosmesh-grove-0001",
    .i2c_sda_gpio = 4,
    .i2c_scl_gpio = 0,
    .i2c_sda_is_bootstrap = false,
    .i2c_scl_is_bootstrap = true,
    .dht_data_gpio = 14,
    .oled_width_px = 128,
    .oled_height_px = 32,
    .light_rc_gpio = 13,
    .status_led_red_gpio = 12,
    .status_led_green_gpio = 16,
    .soil_power_control_gpio = 5,
    .water_power_control_gpio = -1,
    .status_led_common_anode = ATMOSMESH_GROVE_LED_COMMON_ANODE != 0,
};

inline constexpr const ProductProfile& grove_profile() {
    return kGroveProfile;
}

// AtmosMesh Aqua: D2=GPIO4 (SDA), D1=GPIO5 (SCL) — the ESP8266 Arduino core's default Wire pins,
// neither a boot-strap pin (unlike Grove's GPIO0). Water-probe PNP base control on D5=GPIO14.
// See D-019/ADR-0002 and story AQ-01; board and probe hardware identity are unconfirmed, so these
// pins are provisional until photographed and reviewed.
inline constexpr ProductProfile kAquaProfile{
    .product_name = "AtmosMesh Aqua",
    .product_id = "atmosmesh-aqua-v1",
    .product_variant = "esp8266-aqua-station",
    .station_id = "atmosmesh-aqua-0001",
    .i2c_sda_gpio = 4,
    .i2c_scl_gpio = 5,
    .i2c_sda_is_bootstrap = false,
    .i2c_scl_is_bootstrap = false,
    .dht_data_gpio = -1,
    .oled_width_px = 128,
    .oled_height_px = 64,
    .light_rc_gpio = -1,
    .status_led_red_gpio = -1,
    .status_led_green_gpio = -1,
    .soil_power_control_gpio = -1,
    .water_power_control_gpio = 14,
    .status_led_common_anode = false,
};

inline constexpr const ProductProfile& aqua_profile() {
    return kAquaProfile;
}

}  // namespace atmosmesh

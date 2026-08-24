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
    bool status_led_common_anode;
};

// AtmosMesh v1: existing ESP32 station identity and already-established display/DHT pins.
// The legacy ESP32 composition root consumes its existing pin constants; this metadata makes the
// product contract explicit without changing that runtime in the structural migration.
inline constexpr ProductProfile kAtmosMeshV1Profile{
    "AtmosMesh",     "atmosmesh-v1", "esp32-full-station",
    "atmosmesh-0001", 5,              4,
    true,             false,          18,
    128,              64,             -1,
    -1,               -1,             -1,
    false,
};

inline constexpr const ProductProfile& atmosmesh_v1_profile() {
    return kAtmosMeshV1Profile;
}

// AtmosMesh Grove v1.5: D2=GPIO4, D3=GPIO0, D5=GPIO14, D7=GPIO13,
// LED red D6=GPIO12 / green D0=GPIO16, soil PNP base control D1=GPIO5.
// GPIO0 must remain high during reset or the ESP8266 enters ROM download mode.
inline constexpr ProductProfile kGroveProfile{
    "AtmosMesh Grove",      "atmosmesh-grove-v1.5", "atmosmesh-v1.5",
    "atmosmesh-grove-0001", 4,                       0,
    false,                    true,                    14,
    128,                      32,                      13,
    12,                       16,                      5,
    ATMOSMESH_GROVE_LED_COMMON_ANODE != 0,
};

inline constexpr const ProductProfile& grove_profile() {
    return kGroveProfile;
}

}  // namespace atmosmesh

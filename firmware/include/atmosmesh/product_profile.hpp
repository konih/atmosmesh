#pragma once

namespace atmosmesh {

struct ProductProfile {
    const char* product_name;
    const char* product_variant;
    const char* station_id;
    int i2c_sda_gpio;
    int i2c_scl_gpio;
    bool i2c_scl_is_bootstrap;
    int dht_data_gpio;
    int oled_width_px;
    int oled_height_px;
};

// AtmosMesh Grove v1.5: NodeMCU labels D2=GPIO4, D3=GPIO0, D5=GPIO14.
// GPIO0 must remain high during reset or the ESP8266 enters ROM download mode.
inline constexpr ProductProfile kGroveProfile{
    "AtmosMesh Grove", "atmosmesh-v1.5", "atmosmesh-grove-0001",
    4,                  0,                  true,
    14,                 128,                32,
};

inline constexpr const ProductProfile& grove_profile() {
    return kGroveProfile;
}

}  // namespace atmosmesh

#pragma once

#include <cstdint>

// AtmosMesh Room bring-up pin map — ideaspark ESP32-WROOM-32 with the 1.14 inch TFT LCD.
//
// Self-contained on purpose. The constants in pins.hpp describe the AtmosMesh v1 bench
// (BMP280 on SDA=21/SCL=19, OLED on 5/4) and v1 depends on them; nothing here may change them.
//
// The reserved-pin list is D-024. The display is SPI, not I2C, and six GPIOs belong to it.
// GPIO12 is the flash-voltage strap and GPIO1/GPIO3 are the CP2102 USB-UART used by flash/monitor.

namespace atmosmesh::room {

// On-board 1.14 inch ST7789, 135x240 native (portrait). Driven over VSPI.
inline constexpr int kTftMosiGpio = 23;
inline constexpr int kTftSclkGpio = 18;
inline constexpr int kTftCsGpio = 15;
inline constexpr int kTftDcGpio = 2;
inline constexpr int kTftRstGpio = 4;
inline constexpr int kTftBacklightGpio = 32;

inline constexpr int kTftNativeWidthPx = 135;
inline constexpr int kTftNativeHeightPx = 240;

// Landscape. If the panel comes up mirrored or shifted, this is the single knob to turn:
// try the other landscape value (1 <-> 3) before touching column/row offsets.
#ifndef ATMOSMESH_ROOM_TFT_ROTATION
#define ATMOSMESH_ROOM_TFT_ROTATION 3
#endif
inline constexpr int kTftRotation = ATMOSMESH_ROOM_TFT_ROTATION;

inline constexpr int kScreenWidthPx = kTftNativeHeightPx;   // 240 after rotation
inline constexpr int kScreenHeightPx = kTftNativeWidthPx;   // 135 after rotation

// Carrier I2C. GPIO21/GPIO22 are free on this board: neither is an LCD, strap or USB-UART pin.
// Note this is NOT pins.hpp's kSensorSclGpio (19) — that is the bench BMP280 bus.
inline constexpr int kI2cSdaGpio = 21;
inline constexpr int kI2cSclGpio = 22;
inline constexpr std::uint32_t kI2cClockHz = 100000;

// VEML7700 ambient light, SHT41 temperature/humidity. Both 3.3 V, both on the one bus.
inline constexpr std::uint8_t kVeml7700Address = 0x10;
inline constexpr std::uint8_t kSht41Address = 0x44;

inline constexpr unsigned long kSampleIntervalMs = 1000UL;
inline constexpr unsigned long kBootSplashHoldMs = 1600UL;

}  // namespace atmosmesh::room

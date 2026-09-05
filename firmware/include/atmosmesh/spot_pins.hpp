#pragma once

#include <cstdint>

// AtmosMesh Spot pin map — ESP32-C3 SuperMini with the 0.42 inch OLED on the 14 x 20 carrier.
//
// Self-contained like room_pins.hpp: nothing here touches the v1 bench constants in pins.hpp.
// The carrier is hardware/kicad/atmosmesh-spot/wiring.md; every number below is the one soldered
// on the first unit (SP-01, photo-verified pin map, bus pins by scan on 2026-09-04, sensors
// confirmed on the soldered board by scan on 2026-09-05).

namespace atmosmesh::spot {

// One 3.3 V I2C bus: the on-board OLED plus J_SHT and J_VEML. 100 kHz, never leaves the board.
inline constexpr int kI2cSdaGpio = 5;
inline constexpr int kI2cSclGpio = 6;
inline constexpr std::uint32_t kI2cClockHz = 100000;

inline constexpr std::uint8_t kOledAddress = 0x3C;
inline constexpr std::uint8_t kSht41Address = 0x44;
inline constexpr std::uint8_t kVeml7700Address = 0x10;

// The OLED is a 72 x 40 SSD1306 panel with a column offset; U8g2 calls it 72X40_ER. Which way
// is up depends on how the module sits in its socket, so the rotation is a build flag: 0..3 map
// to U8G2_R0..R3, and the two odd values give the 40 x 72 portrait layout.
#ifndef ATMOSMESH_SPOT_OLED_ROTATION
#define ATMOSMESH_SPOT_OLED_ROTATION 0
#endif
inline constexpr int kOledRotation = ATMOSMESH_SPOT_OLED_ROTATION;
static_assert(kOledRotation >= 0 && kOledRotation <= 3, "OLED rotation is 0..3");

// HLK-LD2410S radar on J_RAD. OT2 is the presence pin (HIGH = somebody); OT1 is the module's
// UART TX despite the name and lands on the board's marked RX (GPIO20); the board's marked TX
// (GPIO21) feeds the module's RX. That is UART0 -- free on this board because the console is the
// chip's native USB. The ROM boot log the C3 prints on TX at reset reaches the radar and is
// harmless: a radar command needs the FD FC FB FA header and a valid length.
inline constexpr int kRadarPresenceGpio = 3;
inline constexpr int kRadarRxGpio = 20;   // C3 UART0 RX  <- radar OT1 (UART_TX)
inline constexpr int kRadarTxGpio = 21;   // C3 UART0 TX  -> radar RX
inline constexpr unsigned long kRadarBaud = 115200UL;

// DS18B20 probe on the J_1W terminal, powered mode, R_1W 4.7 kOhm to 3V3 on the carrier.
inline constexpr int kOneWireGpio = 4;

// On the module: BOOT button (GPIO9, strapping pin, has its own pull-up) doubles as the display
// page button; the blue IO8 LED is the heartbeat. Polarity of that LED is not verified, so the
// firmware only toggles it.
inline constexpr int kBootButtonGpio = 9;
inline constexpr int kStatusLedGpio = 8;

// GPIO0/1 (ADC) stay free on purpose; GPIO2 is a strapping pin; GPIO18/19 are USB D-/D+.
constexpr bool pin_is_reserved(int gpio) {
    return gpio == kI2cSdaGpio || gpio == kI2cSclGpio || gpio == kStatusLedGpio ||
           gpio == kBootButtonGpio || gpio == 2 || gpio == 18 || gpio == 19;
}

static_assert(!pin_is_reserved(kRadarPresenceGpio), "radar OT2 collides with a reserved pin");
static_assert(!pin_is_reserved(kRadarRxGpio), "radar UART RX collides with a reserved pin");
static_assert(!pin_is_reserved(kRadarTxGpio), "radar UART TX collides with a reserved pin");
static_assert(!pin_is_reserved(kOneWireGpio), "1-Wire collides with a reserved pin");
static_assert(kRadarRxGpio != kRadarTxGpio, "the radar UART must be crossed onto two pins");
static_assert(kRadarPresenceGpio != kOneWireGpio && kRadarPresenceGpio != kRadarRxGpio &&
                  kRadarPresenceGpio != kRadarTxGpio && kOneWireGpio != kRadarRxGpio &&
                  kOneWireGpio != kRadarTxGpio,
              "two Spot signals share a GPIO");
static_assert(kRadarRxGpio <= 21 && kRadarTxGpio <= 21 && kRadarPresenceGpio <= 21 &&
                  kOneWireGpio <= 21,
              "the ESP32-C3 has GPIO0..21");

// Cadence. The SHT41 conversion and a VEML7700 auto-ranging read are slow; 2 s keeps the loop
// responsive for the radar UART and the page button. The DS18B20 needs 750 ms at 12 bits and is
// read on the pass after it was asked.
inline constexpr unsigned long kSampleIntervalMs = 2000UL;
inline constexpr unsigned long kProbeConversionMs = 800UL;
inline constexpr unsigned long kProbeSearchIntervalMs = 10000UL;
// A reading older than this is unavailable, whatever the last value was. One publish interval
// after a breakout is pulled the entity must go unavailable, not keep repeating the last number.
inline constexpr unsigned long kReadingStaleMs = 10000UL;

// Radar. The module reports its status at 0.5 Hz by default (every 2 s) in the minimal frame;
// no frame for 10 s means "no radar", not "nobody". The first seconds after power-up are its own
// self-check and are not trusted. Presence follows OT2 through a short debounce and a hold so a
// one-frame dropout does not flicker the entity; the module's own "no one" delay (40 s default)
// is the real hold.
inline constexpr unsigned long kRadarWarmupMs = 10000UL;
inline constexpr unsigned long kRadarStaleMs = 10000UL;
inline constexpr unsigned long kPresenceDebounceMs = 50UL;
inline constexpr unsigned long kPresenceHoldMs = 5000UL;

inline constexpr unsigned long kMqttPublishIntervalMs = 5000UL;
inline constexpr unsigned long kLogIntervalMs = 5000UL;
inline constexpr unsigned long kPageDwellMs = 3000UL;
inline constexpr unsigned long kButtonDebounceMs = 30UL;
inline constexpr unsigned long kBootSplashHoldMs = 1500UL;

}  // namespace atmosmesh::spot

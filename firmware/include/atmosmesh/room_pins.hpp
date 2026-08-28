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

// D-SUN PIR occupancy input.
//
// The carrier design (wiring.md) routes PIR OUT through Q_PIR, an NPN that both protects the
// pin and inverts the logic, so GPIO33 LOW means motion *there*. That transistor lives on the
// perfboard, which is not built. On the bare dev board the module's OUT reaches the pin
// directly and motion is HIGH, so active-high is the bring-up default and the flag below is
// what switches to the carrier's polarity once the carrier exists.
#ifndef ATMOSMESH_ROOM_PIR_GPIO
#define ATMOSMESH_ROOM_PIR_GPIO 33
#endif
inline constexpr int kPirGpio = ATMOSMESH_ROOM_PIR_GPIO;

#ifdef ATMOSMESH_ROOM_PIR_ACTIVE_LOW
inline constexpr bool kPirActiveLow = true;
#else
inline constexpr bool kPirActiveLow = false;
#endif

// A D-SUN/HC-SR501 pyroelectric element settles for 30-60 s after power-up and emits spurious
// transitions until it does. Naming that window is the difference between "warming" and "broken".
inline constexpr unsigned long kPirWarmupMs = 60000UL;

// A pyroelectric output is slow and clean, so this only has to reject contact bounce and noise
// picked up on a jumper lead, not shape the signal.
inline constexpr int kPirDebounceMs = 50;

// D-024's reserved list, as code rather than a comment nobody reads: the six GPIOs the on-board
// TFT owns, the GPIO12 flash-voltage strap, and the GPIO1/GPIO3 CP2102 pair. The carrier I2C
// pair is included because a PIR landing on either would take both sensors down with it.
constexpr bool pin_is_reserved(int gpio) {
    return gpio == kTftMosiGpio || gpio == kTftSclkGpio || gpio == kTftCsGpio ||
           gpio == kTftDcGpio || gpio == kTftRstGpio || gpio == kTftBacklightGpio ||
           gpio == 12 || gpio == 1 || gpio == 3 || gpio == kI2cSdaGpio || gpio == kI2cSclGpio;
}

static_assert(!pin_is_reserved(kPirGpio),
              "ATMOSMESH_ROOM_PIR_GPIO names a reserved pin: one of the TFT's six, the GPIO12 "
              "flash strap, the GPIO1/GPIO3 CP2102 pair, or the carrier I2C pair");

// GPIO34-39 are input-only and carry no internal pulldown, so a disconnected PIR would float
// there instead of reading clear -- the failure that looks most like a working sensor.
static_assert(kPirGpio >= 0 && kPirGpio < 34,
              "the PIR pin needs an internal pulldown; GPIO34-39 have none");

// Beeper SIG. v1 drives GPIO25 straight from the GPIO with a 50 ms HIGH and firmware/README.md
// records that as working, so this reuses the proven arrangement rather than inventing one.
// The carrier adds R_BEEP_IN 2.2 kOhm into a driver base and R_BEEP_S 100 Ohm in series with the
// can; neither changes the signal this pin has to produce.
inline constexpr int kBeeperGpio = 25;
inline constexpr unsigned long kBeeperPulseMs = 60UL;
inline constexpr unsigned long kBeeperGapMs = 90UL;
inline constexpr int kBeeperAlarmPulses = 3;

// SDS011 particulate sensor on UART2. 9600 8N1, 3.3 V TTL, and it reports autonomously at 1 Hz,
// so bring-up only has to listen -- no command set, and no dependency on the control protocol.
//
// The UART is CROSSED and wiring.md is emphatic about why: on 2026-08-17 the sensor's TXD was
// landed on the ESP32's TX2 and two push-pull outputs fought on one net every second.
inline constexpr int kSdsRxGpio = 16;   // ESP32 RX2  <- sensor TXD
inline constexpr int kSdsTxGpio = 17;   // ESP32 TX2  -> sensor RXD
inline constexpr unsigned long kSdsBaud = 9600UL;

// The fan and laser need to spin up, and the first frames after power-on are not trustworthy.
inline constexpr unsigned long kSdsWarmupMs = 30000UL;
// No valid frame for this long means "no sensor", not "clean air".
inline constexpr unsigned long kSdsStaleMs = 5000UL;

static_assert(!pin_is_reserved(kBeeperGpio), "the beeper pin collides with a reserved GPIO");
static_assert(!pin_is_reserved(kSdsRxGpio), "the SDS011 RX pin collides with a reserved GPIO");
static_assert(!pin_is_reserved(kSdsTxGpio), "the SDS011 TX pin collides with a reserved GPIO");
static_assert(kSdsRxGpio != kSdsTxGpio, "the SDS011 UART must be crossed onto two distinct pins");
static_assert(kPirGpio != kBeeperGpio && kPirGpio != kSdsRxGpio && kPirGpio != kSdsTxGpio,
              "the PIR pin collides with the beeper or the SDS011 UART");

inline constexpr unsigned long kSampleIntervalMs = 1000UL;
inline constexpr unsigned long kBootSplashHoldMs = 1600UL;

}  // namespace atmosmesh::room

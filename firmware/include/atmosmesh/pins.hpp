#pragma once

#include <cstdint>

namespace atmosmesh {

// Mini I²C OLED (D-001): D5 = SDA, D4 = SCL. Default is SSD1306 128×64 sequential COM (U8g2 ALT0).
// GPIO5 has an internal pull-up; idle-high is OK for boot.
inline constexpr int kOledSdaGpio = 5;
inline constexpr int kOledSclGpio = 4;

// GY-BMP280 (operator 2026-08-14): SDA = GPIO21, SCL = GPIO19.
inline constexpr int kSensorSdaGpio = 21;
inline constexpr int kSensorSclGpio = 19;

// AM2302 / DHT22 data (operator 2026-08-14): GPIO18. Idle-high matches 3.3 V flash-voltage strap.
inline constexpr int kAm2302DataGpio = 18;

// SDS011 UART2 only: sensor TX → ESP32 RX2 (GPIO16), ESP32 TX2 (GPIO17) → sensor RX.
// Never GPIO1/TX0 or GPIO3/RX0 — those are the USB-UART (CP2102) used by flash/monitor.
// Sensor VCC is 5 V; UART must stay 3.3 V. Do not put MQ135 analog on these pins.
inline constexpr int kSds011RxGpio = 16;
inline constexpr int kSds011TxGpio = 17;
inline constexpr int kSds011Baud = 9600;

// MQ135 analog — ADC1 input-only. Never UART, never 5 V into the pin.
// Bench divider (operator 2026-08-14): 10 kΩ series AOUT→GPIO34, 20 kΩ GPIO34→GND.
// GPIO sees 2/3 of AOUT. At 5 V AOUT → 3.33 V on GPIO34: no headroom vs 3.3 V max.
inline constexpr int kMq135AdcGpio = 34;
inline constexpr int kMq135SeriesOhms = 10000;
inline constexpr int kMq135GndOhms = 20000;
inline constexpr int kMq135AdcMax = 4095;
inline constexpr int kMq135AdcFullScaleMv = 3300;
inline constexpr int kMq135AdcNearZero = 16;

// Extra 3-pin modules (operator 2026-08-14, live breadboard). VCC/GND/SIG.
// Beeper SIG GPIO25. PIR D-SUN SIG GPIO33 (not the old GPIO27 reserve).
// HC-20 / DC-20 mic SIG GPIO22. GPIO22 is NOT an ADC pin — digital DO only.
inline constexpr int kBeeperGpio = 25;
inline constexpr int kPirGpio = 33;
inline constexpr int kMicGpio = 22;
inline constexpr int kDigitalDebounceMs = 50;
inline constexpr int kBeeperPulseMs = 50;

inline constexpr bool gpio_is_adc1(int gpio) {
    return gpio == 32 || gpio == 33 || gpio == 34 || gpio == 35 || gpio == 36 || gpio == 39;
}

// 6×10 font, 128 px → 21 columns. Pages: 8 (64), 6 (48), 4 (32). Default glass is 64 rows.
inline constexpr int kOledWidthPx = 128;
inline constexpr int kOledHeightPx = 64;
inline constexpr int kOledHeightPx48 = 48;
inline constexpr int kOledHeightPxAlt = 32;
inline constexpr int kOledLiveLineCount = 3;
// Pack three 6×10 rows into the lower 32 px (RAM y≈32–63). Upper pages were
// blank on this module after ALT0; PM at y=44 and the bar at y=62 were visible.
inline constexpr int kOledLiveRowYPx[] = {34, 46, 58};
inline constexpr int kOledLinePitch48Px = 16;
inline constexpr int kOledLinePitch64Px = 12;
inline constexpr int kOledTelltaleBarYPx = 62;
inline constexpr int kOledTelltaleBarHeightPx = 2;
inline constexpr int kOledBootBarCount = 5;
inline constexpr int kOledBootBarYPx[] = {0, 16, 32, 48, 62};
inline constexpr int kOledBootBarHoldMs = 1500;
inline constexpr std::uint32_t kOledI2cHz = 100000;
inline constexpr int kOledGlyphWidthPx = 6;
inline constexpr int kOledGlyphHeightPx = 8;
inline constexpr int kOledMaxChars = kOledWidthPx / kOledGlyphWidthPx;

inline constexpr unsigned kOledI2cAddresses[] = {0x3C, 0x3D};

inline constexpr unsigned kBmp280AddressGnd = 0x76;  // SDO tied to GND
inline constexpr unsigned kBmp280AddressVdd = 0x77;  // SDO tied to 3V3
inline constexpr unsigned kBmp280ChipId = 0x58;
inline constexpr unsigned kBme280ChipId = 0x60;

inline constexpr int kSerialBaud = 115200;
inline constexpr int kAm2302MinIntervalMs = 2500;

}  // namespace atmosmesh

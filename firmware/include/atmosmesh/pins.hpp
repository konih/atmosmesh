#pragma once

#include <cstdint>

namespace atmosmesh {

// Mini I²C OLED (D-001): D5 = SDA, D4 = SCL. Default is SSD1306 128×32 Univision.
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

// Adafruit 5×7 glyph + 1 px pad; 128 px → 21 columns. 8 px rows → 8 pages (64) or 4 (32).
inline constexpr int kOledWidthPx = 128;
inline constexpr int kOledHeightPx = 64;
inline constexpr int kOledHeightPxAlt = 32;
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

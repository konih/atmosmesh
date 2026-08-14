#pragma once

namespace atmosmesh {

// Mini I²C SSD1306 (D-001, serial-proven 2026-08-14): D5 = SDA, D4 = SCL.
// GPIO5 has an internal pull-up; idle-high is OK for boot.
inline constexpr int kOledSdaGpio = 5;
inline constexpr int kOledSclGpio = 4;

// GY-BMP280 (operator 2026-08-14): SDA = GPIO21, SCL = GPIO19.
inline constexpr int kSensorSdaGpio = 21;
inline constexpr int kSensorSclGpio = 19;

// AM2302 / DHT22 data (operator 2026-08-14): GPIO18. Idle-high matches 3.3 V flash-voltage strap.
inline constexpr int kAm2302DataGpio = 18;

// Adafruit 5×7 glyph + 1 px pad; 128 px → 21 columns. 8 px rows → 8 pages (64) or 4 (32).
inline constexpr int kOledWidthPx = 128;
inline constexpr int kOledHeightPx = 64;
inline constexpr int kOledHeightPxAlt = 32;
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

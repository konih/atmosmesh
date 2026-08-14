#pragma once

namespace atmosmesh {

// Bench LCD (D-006): D5 = SDA, D4 = SCL. GPIO5 has an internal pull-up; idle-high is OK for boot.
inline constexpr int kLcdSdaGpio = 5;
inline constexpr int kLcdSclGpio = 4;

// GY-BMP280 (operator 2026-08-14): SDA = GPIO21, SCL = GPIO19.
inline constexpr int kSensorSdaGpio = 21;
inline constexpr int kSensorSclGpio = 19;

// AM2302 / DHT22 data (operator 2026-08-14): GPIO18. Idle-high matches 3.3 V flash-voltage strap.
inline constexpr int kAm2302DataGpio = 18;

inline constexpr int kLcdColumns = 16;
inline constexpr int kLcdRows = 2;

inline constexpr unsigned kLcdI2cAddresses[] = {0x27, 0x3F, 0x20, 0x3E, 0x38};

inline constexpr unsigned kBmp280AddressGnd = 0x76;  // SDO tied to GND
inline constexpr unsigned kBmp280AddressVdd = 0x77;  // SDO tied to 3V3
inline constexpr unsigned kBmp280ChipId = 0x58;
inline constexpr unsigned kBme280ChipId = 0x60;

inline constexpr int kSerialBaud = 115200;
inline constexpr int kAm2302MinIntervalMs = 2500;

}  // namespace atmosmesh

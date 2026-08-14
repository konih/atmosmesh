# Firmware

PlatformIO + Arduino on `esp32dev` (ESP32-WROOM-32). Host tests run on the `native` environment
and do not need the board.

## Commands

Prefer `task` from the repository root so the shared agent venv is on `PATH` (`python` +
`pyserial` + `esptool`). Do not use Homebrew or Xcode `python -m esptool`.

From `firmware/` (only after `scripts/with-agent-python` or the venv `bin` is first on `PATH`):

```bash
pio test -e native          # host unit tests (required)
pio run -e esp32dev         # build device image
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-0001
pio device monitor --port /dev/cu.usbserial-0001 --baud 115200
```

## Bench OLED wiring (D-001)

Mini I²C SSD1306 on the DevBoard (serial-proven 0x3C):

| OLED | ESP32 |
| --- | --- |
| VCC | **3V3 only** |
| GND | GND |
| SDA | D5 / GPIO5 (firmware retries GPIO4 if this mapping is silent) |
| SCL | D4 / GPIO4 |

Boot banner: `AtmosMesh` / `OLED bring-up`. The loop then shows AM2302 T/RH and BMP280 address.
Firmware prefers I²C **0x3C**, then **0x3D**. LCD backpack addresses (0x27/0x3F) are not the display.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO19 | Not GPIO22 |
| AM2302 DATA | GPIO18 | VDD=3V3. GPIO18 high matches the 3.3 V flash-voltage strap; keep idle-high |

Two I²C buses: OLED on Wire (GPIO5/4), BMP280 on Wire1 (GPIO21/19).

GPIO5 (OLED SDA) has an internal pull-up and wants idle-high at boot; that is usually compatible
with I²C. GPIO18 (AM2302) idle-high is OK (3.3 V flash voltage).

## Layout

| Path | Role |
| --- | --- |
| `include/atmosmesh/` | Shared headers (pins, banner, I²C address pick) |
| `src/display_text.cpp` | Host-testable OLED string clipping (128×64 / 128×32 pages) |
| `src/oled_address.cpp` | Host-testable SSD1306 address selection (0x3C then 0x3D) |
| `src/bmp_address.cpp` | Host-testable BMP280 address pick |
| `src/am2302_frame.cpp` | Host-testable AM2302 checksum/parse |
| `src/i2c_bus.cpp` | ESP32 I²C scan |
| `src/main.cpp` | Bring-up: SSD1306, BMP280 chip-id, AM2302 |
| `test/test_native/` | Unity tests compiled with `pio test -e native` |

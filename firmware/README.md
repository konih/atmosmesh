# Firmware

PlatformIO + Arduino on `esp32dev` (ESP32-WROOM-32). Host tests run on the `native` environment
and do not need the board.

## Commands

From `firmware/`:

```bash
pio test -e native          # host unit tests (required)
pio run -e esp32dev         # build device image
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-0001
pio device monitor --port /dev/cu.usbserial-0001 --baud 115200
```

## Bench LCD wiring (D-006)

I²C 1602 (or similar backpack) on the DevBoard:

| LCD | ESP32 |
| --- | --- |
| VCC | **3V3 only** — never 5 V (backpack pull-ups would drive GPIOs to 5 V) |
| GND | GND |
| SDA | D2 / GPIO2 (firmware retries GPIO4 if this mapping is silent) |
| SCL | D4 / GPIO4 |

Dummy text on boot: line 1 `AtmosMesh`, line 2 `hello, LCD`. Then the loop shows AM2302 and BMP280 status.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO18 | Not GPIO22 |
| AM2302 DATA | GPIO5 | VDD=3V3. GPIO5 is a strapping pin; keep idle-high (pull-up to 3V3) |

Two I²C buses: LCD on Wire (GPIO2/4), BMP280 on Wire1 (GPIO21/18).

If `esptool` reports flash IO errors or boot mode `0xf`, **unplug the LCD from D2/GPIO2**,
flash, then reconnect. GPIO2 is a strapping pin; an I²C pull-up holds it high and blocks
download mode.

## Layout

| Path | Role |
| --- | --- |
| `include/atmosmesh/` | Shared headers (pins, banner, I²C address pick) |
| `src/display_text.cpp` | Host-testable LCD string clipping |
| `src/lcd_address.cpp` | Host-testable backpack address selection |
| `src/bmp_address.cpp` | Host-testable BMP280 address pick |
| `src/am2302_frame.cpp` | Host-testable AM2302 checksum/parse |
| `src/lcd_bus.cpp` | ESP32 I²C scan |
| `src/main.cpp` | Bring-up: LCD, BMP280 chip-id, AM2302 |
| `test/test_native/` | Unity tests compiled with `pio test -e native` |

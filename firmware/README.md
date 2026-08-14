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

## Bench LCD wiring (D-006)

I²C 1602 (or similar backpack) on the DevBoard:

| LCD | ESP32 |
| --- | --- |
| VCC | **3V3 only** — never 5 V (backpack pull-ups would drive GPIOs to 5 V) |
| GND | GND |
| SDA | D5 / GPIO5 (firmware retries GPIO4 if this mapping is silent) |
| SCL | D4 / GPIO4 |

Dummy text on boot: line 1 `AtmosMesh`, line 2 `hello, LCD`. Then the loop shows AM2302 and BMP280 status.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO19 | Not GPIO22 |
| AM2302 DATA | GPIO18 | VDD=3V3. GPIO18 high matches the 3.3 V flash-voltage strap; keep idle-high |

Two I²C buses: LCD on Wire (GPIO5/4), BMP280 on Wire1 (GPIO21/19).

GPIO5 (LCD SDA) has an internal pull-up and wants idle-high at boot; that is usually compatible
with I²C. GPIO18 (AM2302) idle-high is OK (3.3 V flash voltage). LCD is **not** on GPIO2.

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

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

Boot banner: `AtmosMesh` / `OLED bring-up`. The loop then shows a **2×3** page on 128×32:
T/RH/hPa on row 1, PM2.5/PM10/gas index on row 2. Serial still prints full BMP T/P, AM2302,
SDS011 PM, and MQ135 **raw ADC / GPIO volts** (never CO₂). Firmware prefers I²C **0x3C**, then
**0x3D**. LCD backpack addresses (0x27/0x3F) are not the display.

The glass is driven by **one** renderer: U8g2 full-frame
(`U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C` by default). 128×64 ALT0 sequential COM is
`-DATMOSMESH_OLED_HEIGHT=64`. SH1106 is a compile fallback (`-DATMOSMESH_OLED_CONTROLLER_ID=1`).
After `begin()` the firmware turns the panel on, sets contrast 255, and draws a brief **HI** so
the live 2×3 page is readable. Serial includes
`constructor=U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C`. I²C clock is **100 kHz**. **OLED VCC = 3V3**;
5 V on VCC with pull-ups to VCC can kill GPIO5/4.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO19 | Not GPIO22 |
| AM2302 DATA | GPIO18 | VDD=3V3. GPIO18 high matches the 3.3 V flash-voltage strap; keep idle-high |
| SDS011 TX | GPIO16 / RX2 | Sensor TX → **D16/RX2**. VCC=**5 V**. UART **3.3 V**. **Not** TX2, **not** RX0/GPIO3 |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX2 → sensor RX (commands). Do not put sensor TX on TX2. **Not** TX0/GPIO1 |
| MQ135 AOUT | GPIO34 via divider | Analog, not UART. Neither RX2/TX2 nor RX0/TX0 |

Two I²C buses: OLED on Wire (GPIO5/4), BMP280 on Wire1 (GPIO21/19). UART2 is SDS011 only.
**Do not wire SDS011 (d011v2) to RX0/TX0** (GPIO3/GPIO1). Those pins are the CP2102 USB-UART used
by `task flash` / `task monitor`. Firmware uses `Serial2` on GPIO16/17 and will not move to UART0.

**MQ135 is analog, not UART, and not a CO₂ sensor.** Heater power is **5 V**, never `3V3`. Analog
belongs on GPIO34 after the divider. Operator bench (2026-08-14): **10 kΩ series** AOUT→GPIO34,
**20 kΩ** GPIO34→GND. GPIO sees **2/3** of AOUT. At 5 V AOUT the pin sits at **3.33 V** — at the
ESP32 3.3 V max there is **no headroom**. Firmware logs `mq135: raw=… gpio_mv=… aout_mv=…` (estimated
AOUT from the inverse ratio). If raw stays ~0, check AOUT/GND/5 V heater wiring. **Never apply 5 V
to GPIO34.** Serial also warns; do not label the reading `CO2` or `ppm`.

GPIO5 (OLED SDA) has an internal pull-up and wants idle-high at boot; that is usually compatible
with I²C. GPIO18 (AM2302) idle-high is OK (3.3 V flash voltage).

## Layout

| Path | Role |
| --- | --- |
| `include/atmosmesh/` | Shared headers (pins, banner, I²C address pick) |
| `src/display_text.cpp` | Host-testable OLED string clipping (128×64 / 128×32 pages) |
| `src/oled_profile.cpp` | Host-testable controller/geometry/COM selection (SSD1306 vs SH1106) |
| `src/oled_address.cpp` | Host-testable SSD1306 address selection (0x3C then 0x3D) |
| `src/bmp_address.cpp` | Host-testable BMP280 address pick |
| `src/am2302_frame.cpp` | Host-testable AM2302 checksum/parse |
| `src/sds011_frame.cpp` | Host-testable SDS011 `AA C0 … AB` checksum/parse |
| `src/i2c_bus.cpp` | ESP32 I²C scan |
| `src/mq135_scale.cpp` | Host-testable ADC→mV and 2/3-divider inverse (never CO₂) |
| `src/main.cpp` | Bring-up: U8g2 OLED prove-life, BMP280 chip-id, AM2302, SDS011 UART2 RX2, MQ135 ADC |
| `test/test_native/` | Unity tests compiled with `pio test -e native` |

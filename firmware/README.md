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

Boot banner: `AtmosMesh` / `OLED bring-up`. The loop then shows AM2302 T/RH, BMP280 address, and
SDS011 PM2.5/PM10 when UART2 frames parse. Firmware prefers I²C **0x3C**, then **0x3D**. LCD
backpack addresses (0x27/0x3F) are not the display.

Default panel programming is **SSD1306 128×64** at **100 kHz** with **sequential COM** (`0xDA 0x02`).
That is the usual fix when a 0.96" module shows pixels but drops every other line (Adafruit's
128×64 default uses alternate COM `0x12`). If the glass is still shifted, rebuild with
`-DATMOSMESH_OLED_CONTROLLER_ID=1` (SH1106, 2-pixel column offset). If only the top half is used,
`-DATMOSMESH_OLED_HEIGHT=32`. Serial logs `oled: init ok controller=… width=… height=… addr=…`.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO19 | Not GPIO22 |
| AM2302 DATA | GPIO18 | VDD=3V3. GPIO18 high matches the 3.3 V flash-voltage strap; keep idle-high |
| SDS011 TX | GPIO16 / RX2 | Sensor TX → ESP32 RX. VCC=**5 V**. UART **3.3 V** only |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX → sensor RX. Do not drive 5 V into GPIO16 |
| MQ135 AOUT | GPIO34 via divider | Analog, not UART. RX2/TX2 is the wrong connector |

Two I²C buses: OLED on Wire (GPIO5/4), BMP280 on Wire1 (GPIO21/19). UART2 is SDS011 only.

**MQ135 is not UART.** Wiring the gas board (often labelled MQ13/MQ135) to RX2/TX2 cannot produce
`AA C0` frames and can put heater/AO **5 V** on ESP32 GPIOs. Heater power is **5 V**, never `3V3`.
Analog belongs on GPIO34 after a measured divider (KiCad J5).

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
| `src/main.cpp` | Bring-up: OLED, BMP280 chip-id, AM2302, SDS011 UART2 |
| `test/test_native/` | Unity tests compiled with `pio test -e native` |

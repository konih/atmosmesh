# Firmware

PlatformIO + Arduino builds two product variants from one project:

- **AtmosMesh v1** on `esp32dev`: the full ESP32 station hardware stack.
- **AtmosMesh Grove v1.5** on `esp8266-grove`: a focused ESP8266 OLED/BMP180/DHT11 variant.

Each target has a thin hardware entrypoint selected by PlatformIO source filters. Pure profiles,
health states and display formatting live under `include/atmosmesh/` and are exercised by the same
`native` host tests; Grove is not a wholesale copy of the ESP32 application.

## Commands

Prefer `task` from the repository root so the shared agent venv is on `PATH` (`python` +
`pyserial` + `esptool`). Do not use Homebrew or Xcode `python -m esptool`.

From `firmware/` (only after `scripts/with-agent-python` or the venv `bin` is first on `PATH`):

```bash
pio test -e native          # host unit tests (required)
pio run -e esp32dev         # build device image
pio run -e esp8266-grove    # build Grove v1.5 image
pio run -e esp32dev -t upload --upload-port /dev/cu.usbserial-0001
pio device monitor --port /dev/cu.usbserial-0001 --baud 115200
```

From the repository root use `task build` for ESP32 and `task build-grove` for Grove. The explicit
`task flash-grove` command exists for later bring-up, but **must not be run without operator
authorization**: uploading AtmosMesh replaces the board's currently working AT firmware.

## AtmosMesh Grove v1.5 wiring

The default ID is `atmosmesh-grove-0001`; IDs remain device-based, never room names. A read-only
probe found an ESP8266EX with a 26 MHz crystal and 4 MB flash. The NodeMCU-style profile is:

| Device | Module pin | ESP8266MOD | Notes |
| --- | --- | --- | --- |
| SSD1306 128×32 | SDA | `D2` / GPIO4 | I²C 0x3C, then 0x3D fallback |
| SSD1306 128×32 | SCL | `D3` / GPIO0 | Shared bus; boot-strap caveat below |
| BMP180 | SDA | `D2` / GPIO4 | I²C 0x77 |
| BMP180 | SCL | `D3` / GPIO0 | Shared bus |
| DHT11 | DATA | `D5` / GPIO14 | Agreed profile assumption; verify the physical joint |
| All modules | VCC/GND | `3V3` / GND | Never power their GPIO pull-ups from 5 V |

Firmware deliberately calls `Wire.begin(4, 0)`; it does not silently rewrite the physical wiring.
`D3` is GPIO0, an ESP8266 boot strap. It must stay **HIGH during reset**. If a module or fault holds
SCL low, the board enters the ROM downloader rather than starting AtmosMesh. Moving SCL to D1/GPIO5
is a possible later hardware revision, not the v1.5 wiring implemented here.

The 32-pixel display uses four compact rows: product, DHT temperature/humidity, BMP pressure, and
explicit DHT/BMP health. Missing or failed samples render as dashes plus `ERR`, never numeric zero.
Serial startup identifies the product/station, exact pins, GPIO0 warning and every init/read state.

YL-69/YL-38 soil, the LDR and MAX4466 are deferred. They are not claimed as fitted or working.
There is one ADC channel, and the exact board-level A0 divider is unverified, so no analog output
should be attached until V15-04 approves its voltage and channel-sharing design.

## Bench OLED wiring (D-001)

Mini I²C SSD1306 on the DevBoard (serial-proven 0x3C):

| OLED | ESP32 |
| --- | --- |
| VCC | **3V3 only** |
| GND | GND |
| SDA | D5 / GPIO5 (firmware retries GPIO4 if this mapping is silent) |
| SCL | D4 / GPIO4 |

Boot diagnostic: five 2 px bars at y=0, 16, 32, 48, 62 for ~1.5 s (`oled: bars y=0,16,32,48,62 —
say which you see`), then a **3-row** live page packed into the **lower 32 px** (glyph tops at
y=34/46/58): T/RH, hPa/gas index, PM2.5/PM10. Serial prints `oled: flip=0` (rebuild with
`-DATMOSMESH_OLED_FLIP=1` if COM is inverted). Serial still prints full BMP T/P, AM2302, SDS011
PM, and MQ135 **raw ADC / GPIO volts** (never CO₂). Firmware prefers I²C **0x3C**, then **0x3D**.
LCD backpack addresses (0x27/0x3F) are not the display.

**Default constructor is `U8G2_SSD1306_128X64_ALT0_F_HW_I2C` (sequential COM).** Do not send
mux `0x2F`. Do not use `U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C` as the default — that only
lights the top ~32 px of 128×64 glass. I²C clock is **100 kHz**. **OLED VCC = 3V3**; 5 V on
VCC with pull-ups to VCC can kill GPIO5/4.

## Sensor wiring (operator 2026-08-14)

| Device | ESP32 | Notes |
| --- | --- | --- |
| GY-BMP280 SDA | GPIO21 | VCC=3V3, CSB=3V3, SDO=GND → 0x76 |
| GY-BMP280 SCL | GPIO19 | Not GPIO22 |
| AM2302 DATA | GPIO18 | VDD=3V3. GPIO18 high matches the 3.3 V flash-voltage strap; keep idle-high |
| SDS011 TX | GPIO16 / RX2 | Sensor TX → **D16/RX2**. VCC=**5 V**. UART **3.3 V**. **Not** TX2, **not** RX0/GPIO3 |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX2 → sensor RX (commands). Do not put sensor TX on TX2. **Not** TX0/GPIO1 |
| MQ135 AOUT | GPIO34 via divider | Analog, not UART. Neither RX2/TX2 nor RX0/TX0 |
| Beeper SIG | GPIO25 | 3-pin VCC/GND/SIG. 50 ms HIGH at boot; 50 ms on PIR rising edge |
| PIR D-SUN OUT | GPIO33 | 3-pin. Digital. Was reserved 27 — **use 33** |
| VEML7700 lux | GPIO21/19 (Wire1) | I²C **0x10** with BMP280. VCC 3V3. **Not fitted yet.** No extra GPIO |

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

**VEML7700 lux** shares **Wire1** with BMP280 (SDA=GPIO21, SCL=GPIO19), I²C **0x10**, VCC **3V3**.
The part is **not fitted yet**: boot logs `veml7700: not found (ok until fitted)` and the OLED shows
`-- lx` on the hPa line. When present: serial `veml7700: lux=…` and e.g. `1013 hPa   123 lx`.
**No microphone. No clap.** GPIO22 and GPIO35 are unused.

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
| `src/digital_edge.cpp` | Host-testable PIR debounce + serial labels |
| `src/veml7700_text.cpp` | Host-testable VEML7700 0x10 / lux OLED+serial formatters |
| `src/mqtt_contract.cpp` | Host-testable MQTT topics, state JSON, HA discovery payloads |
| `src/mqtt_session.cpp` | Host-testable reconnect backoff and publish sequencing |
| `src/mqtt_runtime.cpp` | ESP32-only async Wi-Fi + `esp_mqtt` (excluded from native) |
| `include/atmosmesh/product_profile.hpp` | Host-tested Grove product identity and explicit pin/geometry profile |
| `src/grove_status.cpp` | Shared, host-tested Grove missing/value/health formatting |
| `include/atmosmesh/secrets.hpp.example` | Copy to gitignored `secrets.hpp` for Wi-Fi/MQTT |
| `src/main.cpp` | Thin ESP32 v1 entrypoint: full station hardware stack |
| `src/grove_main.cpp` | Thin ESP8266 Grove v1.5 entrypoint: shared I²C OLED/BMP180 and DHT11 |
| `test/test_native/` | Unity sensor/OLED tests (`pio test -e native`) |
| `test/test_mqtt/` | Unity MQTT contract/session tests |

## MQTT / Wi-Fi credentials

Preferred: put secrets in a **gitignored** `.envrc` at the main checkout (see `.envrc.example`):

```bash
export WIFI_SSID="your-ssid"
export WIFI_PASSWORD="your-wifi-password"
export MQTT_HOST="kum3-lan-address"
export MQTT_PORT="1883"
export MQTT_USER="homeassistant"
export MQTT_PASSWORD="from-kumulus-sops"
```

Then `direnv allow` and `task build` / `task flash`. Those tasks run
`scripts/gen-secrets-from-env`, which writes gitignored
`firmware/include/atmosmesh/secrets.hpp` (also works from a git worktree by reading the
main-checkout `.envrc`).

Alternative: copy `include/atmosmesh/secrets.hpp.example` → `secrets.hpp` by hand.
Without Wi-Fi + MQTT credentials, the image still samples sensors and drives the OLED;
networking is skipped. Topics and HA discovery are in `docs/architecture.md` (D-007).

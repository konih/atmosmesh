# Firmware

PlatformIO + Arduino builds two independent, first-class products from one project:

| Product | Stable product ID | Product variant | Composition root | Canonical environment |
| --- | --- | --- | --- | --- |
| **AtmosMesh v1** — full ESP32 station | `atmosmesh-v1` | `esp32-full-station` | `src/products/atmosmesh_v1.cpp` | `atmosmesh-v1` |
| **AtmosMesh Grove v1.5** — compact ESP8266 node | `atmosmesh-grove-v1.5` | `atmosmesh-v1.5` | `src/products/atmosmesh_grove_v1_5.cpp` | `atmosmesh-grove-v1_5` |

Neither product replaces the other. The composition model and version semantics are defined by the
accepted [ADR-0001](../docs/adr/0001-multi-product-firmware-composition.md).

PlatformIO source filters select the existing ESP32 runtime from its named composition root and a
separate thin, profiled Grove root. Grove health states and display formatting live under
`include/atmosmesh/` and are exercised by the same `native` host tests; Grove is not a wholesale
copy of the ESP32 application. This slice does not refactor or describe the legacy ESP32 entrypoint
as thin/profile-driven.

## Commands

Prefer `task` from the repository root so the shared agent venv is on `PATH` (`python` +
`pyserial` + `esptool`). Do not use Homebrew or Xcode `python -m esptool`.

From `firmware/` (only after `scripts/with-agent-python` or the venv `bin` is first on `PATH`):

```bash
pio test -e native          # host unit tests (required)
pio run -e atmosmesh-v1             # canonical ESP32 product
pio run -e atmosmesh-grove-v1_5     # canonical Grove product
pio run -e esp32dev                  # compatibility environment
pio run -e esp8266-grove             # compatibility environment
pio device monitor --port /dev/cu.usbserial-0001 --baud 115200
```

From the repository root use `task build-v1`, `task build-v1-5`, or `task build-all`. Canonical
device actions are `flash-v1`/`monitor-v1` and `flash-v1-5`/`monitor-v1-5`; the existing
`build`/`flash`/`monitor` and `*-grove` tasks remain compatibility aliases.

After explicit authorization and independent review, the coordinator flashed AtmosMesh Grove on
2026-08-24. The former AT firmware was replaced. Reviewed head `c880afe`, including RC light, MQTT,
LED and soil support, was later flashed successfully. Serial/raw sampling evidence is described
below; visual, electrical and broker-side validation remains incomplete.

## AtmosMesh Grove v1.5 wiring

The default ID is `atmosmesh-grove-0001`; IDs remain device-based, never room names. A read-only
probe found an ESP8266EX with a 26 MHz crystal and 4 MB flash. The NodeMCU-style profile is:

Current firmware emits all identity concepts separately:

```text
product=AtmosMesh Grove product_id=atmosmesh-grove-v1.5 variant=atmosmesh-v1.5 station_id=atmosmesh-grove-0001
```

| Device | Module pin | ESP8266MOD | Notes |
| --- | --- | --- | --- |
| SSD1306 128×32 | SDA | `D2` / GPIO4 | I²C 0x3C, then 0x3D fallback |
| SSD1306 128×32 | SCL | `D3` / GPIO0 | Shared bus; boot-strap caveat below |
| BMP180 | SDA | `D2` / GPIO4 | I²C 0x77 |
| BMP180 | SCL | `D3` / GPIO0 | Shared bus |
| DHT11 | DATA | `D5` / GPIO14 | Valid communication observed; readings remain uncalibrated |
| Bare LDR RC | Measurement node | `D7` / GPIO13 | `3V3 → LDR → 1 kΩ → node`; 100 nF node-to-GND |
| Bi-color LED red | Color channel | `D6` / GPIO12 | Separate ~330 Ω resistor |
| Bi-color LED green | Color channel | `D0` / GPIO16 | Separate ~330 Ω resistor |
| YL-38 high-side switch | 2N3906 PNP base | `D1` / GPIO5 | Active LOW via 2.2 kΩ; external 100 kΩ base-emitter pull-up |
| YL-38 | AO | `A0` through 47 kΩ / 15 kΩ | 100 nF (`104`) A0-to-GND; DO unused |
| All modules | VCC/GND | `3V3` / GND | Never power their GPIO pull-ups from 5 V |

Firmware deliberately calls `Wire.begin(4, 0)`; it does not silently rewrite the physical wiring.
`D3` is GPIO0, an ESP8266 boot strap. It must stay **HIGH during reset**. If a module or fault holds
SCL low, the board enters the ROM downloader rather than starting AtmosMesh. Moving SCL to D1/GPIO5
would now conflict with the V15-06 soil control pin and requires an explicit hardware/profile
redesign; it is not the v1.5 wiring implemented here.

The 32-pixel display uses all four rows for labelled measurements, for example:

```text
T:25.0C RH:36%
P:982.7hPa
Light:401us
Soil:214
```

The page has no product row or anonymous health bits; identity and detailed health remain on serial
and the bi-color LED. Missing values retain their labels (`T:--`, `RH:--`, `P:ERR`, `Light:--`,
`Soil:--`), while valid numeric zero remains visible. Temperature is DHT11 `temperature_c`; BMP
temperature is not added to the public display/MQTT contract. Light stays raw microseconds and soil
stays raw ADC, never lux or moisture percent. Each line is bounded to 21 characters for the 5×7
font on the 128-pixel display.
Every sensor cycle probes BMP180 address 0x77: loss immediately invalidates the prior sample, while
return triggers reinitialization before a value can become valid again. Serial startup identifies
the product/station, exact pins, GPIO0 warning and every init/read state.

The LDR is uncalibrated digital RC timing: firmware discharges D7 for 1 ms, releases it as an input,
then advances a cooperative time-to-high state machine with a 200 ms hard timeout. A valid
`light_charge_us` value is raw microseconds and **lower means brighter**. Immediate/saturated,
timeout or disconnected states are unavailable. MAX4466 remains dropped.

The firmware samples D7 synchronously immediately after changing it from driven-low to input. If
the released line is already HIGH, that cycle is saturated/unavailable rather than a small plausible
charge time. Normal charge timing continues cooperatively on subsequent loop ticks.

### Bi-color health LED

Default compiled wiring is common-cathode: red D6/GPIO12 and green D0/GPIO16 are active HIGH.
Each channel requires its own approximately 330 Ω resistor. Define
`ATMOSMESH_GROVE_LED_COMMON_ANODE=1` in the Grove build flags to invert both channels for a
common-anode LED; startup prints the exact compiled polarity and HIGH/LOW levels. No WS2812 library
is used.

Common-anode verification build (does not flash):

```bash
PLATFORMIO_BUILD_FLAGS="-DATMOSMESH_GROVE_LED_COMMON_ANODE=1" task build-v1-5
```

| Color | Meaning |
| --- | --- |
| Red | DHT/BMP invalid, or an explicit light/soil acquisition timeout/failure |
| Amber (red + green) | Core local sensors valid; MQTT offline or unconfigured |
| Green | Core local sensors valid and MQTT connected |

A light/soil value that is merely uncalibrated, missing before its first sample, or immediately
saturated does not by itself turn the LED red.

### YL-38 raw ADC and switched power

The operator confirmed and physically wired a 2N3906 PNP high-side switch: emitter=3V3,
collector=YL-38 VCC, base=D1/GPIO5 through 2.2 kΩ, with an external 100 kΩ base-to-emitter pull-up.
GPIO5 drives only the base and is active LOW—never power the YL board from a GPIO. If YL VCC is
wired directly to 3V3, firmware cannot prevent continuous power and makes no duty-cycle claim.

YL AO reaches A0 through a conservative 47 kΩ top / 15 kΩ bottom divider; 100 nF (`104`) from A0
to GND filters noise. Grounds are common and YL DO is unused. This divider keeps 3.3 V AO
near 0.80 V for a bare 1.0 V ESP8266 ADC. Some NodeMCU boards already divide A0, so the combined
attenuation and raw counts depend on the exact board. Firmware therefore exposes only
`soil_adc_raw`, never calibrated moisture or percent.

Firmware latches D1 HIGH before enabling OUTPUT, waits 30 seconds between cycle starts, then drives
the PNP base control LOW, settles 100 ms, takes five samples 5 ms apart, averages them, and returns
the control pin HIGH immediately. Normal power-on time is 120 ms / 30 s = 0.4%; the 250 ms fail-off
deadline bounds the cooperative policy to 0.84% (rounded-up documented bound 0.9%). MQTT/DNS/TCP
work is not started while YL power is active. Before the first completed cycle the field is
unavailable/omitted; raw ADC zero after a completed cycle is a valid numeric reading.

### Grove MQTT contract

With generated credentials present, the Grove transport uses ESP8266WiFi and pinned PubSubClient
2.8 over the shared host-tested MQTT contract/session. Without credentials, networking is disabled
and sensors/OLED continue.

| Piece | Grove value |
| --- | --- |
| State | `home/air/atmosmesh-grove-0001/state` (not retained) |
| Availability/LWT | `home/air/atmosmesh-grove-0001/availability` (`online`/`offline`, retained) |
| Discovery | `homeassistant/sensor/atmosmesh_grove_0001/<object_id>/config` (retained) |
| Entities | `temperature_c`, `humidity_pct`, `pressure_hpa`, `light_charge_us`, `soil_adc_raw` |

State omits invalid values, so missing is distinct from a valid numeric zero. The light entity is
named **Uncalibrated RC Light Charge Time**, uses `µs`, and has no illuminance device class.
The soil entity is **Soil Probe ADC Raw**, uses `ADC count`, and has no moisture device class,
percentage or calibration claim. Every connect replays discovery, followed by retained online
availability and any pending state. Broker
retries back off from 1 to 30 seconds. DNS lookup and TCP connection share a 1000 ms transport
budget instead of the ESP8266 core's roughly five-second default. PubSubClient's subsequent MQTT
response wait is separately bounded to one second, so a complete failed attempt can occupy roughly
two seconds before the reconnect backoff resumes local work.

### Controlled hardware result (2026-08-24)

- OLED: controller initialization passed at 0x3C as 128×32; pixels remain visually unconfirmed.
- BMP180: runtime passed repeatedly; latest paired observation was 25.6 °C / 983.9–984.0 hPa.
- DHT11: later reported 32.0 °C / 32% RH then 31.0 °C / 32% RH on D5/GPIO14. This proves
  communication, not accuracy or calibration.
- RC light: serial showed uncalibrated 389–452 µs values; controlled bright/dark/saturation/timeout
  response remains pending. MQTT broker/HA receipt and reconnect behavior are not yet validated.
- YL-38: serial showed two cycles about 30 seconds apart, each
  `soil: ok adc_raw=214 samples=5 power=off`. This proves raw acquisition and the firmware OFF action,
  not calibration, switched-rail voltage/current or physical power-off.
- Bi-color LED colors and polarity remain visually unconfirmed.
- The first captured banner from reviewed head `a681990` contained product name, variant and station
  ID but no separate `product_id`. A second reviewed flash of final head `50ca2f3` captured the exact
  four-field banner documented above.

## AtmosMesh v1 bench OLED wiring (D-001)

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

## AtmosMesh v1 sensor wiring (operator 2026-08-14)

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
| `include/atmosmesh/` | Shared headers, product metadata and host-testable contracts |
| `src/products/atmosmesh_v1.cpp` | AtmosMesh v1 composition root; existing ESP32 runtime moved without behavior changes |
| `src/products/atmosmesh_grove_v1_5.cpp` | Thin AtmosMesh Grove v1.5 composition root |
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
| `src/grove_mqtt_runtime.cpp` | Thin ESP8266WiFi/PubSubClient Grove transport (excluded from native) |
| `include/atmosmesh/product_profile.hpp` | Host-tested identity and explicit pin/geometry metadata for both products |
| `src/grove_status.cpp` | Shared, host-tested Grove missing/value/health formatting |
| `src/rc_light.cpp` | Host-tested cooperative RC discharge/time-to-high policy |
| `src/soil_sampler.cpp` | Host-tested cooperative active-low power/sample/fail-off policy |
| `src/status_led.cpp` | Host-tested health/color/polarity mapping |
| `include/atmosmesh/secrets.hpp.example` | Copy to gitignored `secrets.hpp` for Wi-Fi/MQTT |
| `test/test_native/` | Unity sensor/OLED tests (`pio test -e native`) |
| `test/test_mqtt/` | Unity MQTT contract/session tests |

## MQTT / Wi-Fi credentials for both products

Preferred: put secrets in a **gitignored** `.envrc` at the main checkout (see `.envrc.example`):

```bash
export WIFI_SSID="your-ssid"
export WIFI_PASSWORD="your-wifi-password"
export MQTT_HOST="kum3-lan-address"
export MQTT_PORT="1883"
export MQTT_USER="homeassistant"
export MQTT_PASSWORD="from-kumulus-sops"
```

Then `direnv allow` and use `task build-v1`, `task build-v1-5`, or `task build-all` (and the matching
reviewed flash command only when authorized). Both product build/flash tasks run
`scripts/gen-secrets-from-env`, which writes gitignored
`firmware/include/atmosmesh/secrets.hpp` (also works from a git worktree by reading the
main-checkout `.envrc`).

Alternative: copy `include/atmosmesh/secrets.hpp.example` → `secrets.hpp` by hand.
Without Wi-Fi + MQTT credentials, either image still samples sensors and drives the OLED;
networking is skipped. Topics and HA discovery are in `docs/architecture.md` (D-007/D-013).

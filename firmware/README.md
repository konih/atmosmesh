# Firmware

PlatformIO + Arduino builds two independent, first-class products from one project:

| Product | Stable product ID | Product variant | Composition root | Canonical environment |
| --- | --- | --- | --- | --- |
| **AtmosMesh v1** — full ESP32 station | `atmosmesh-v1` | `esp32-full-station` | `src/products/atmosmesh_v1.cpp` | `atmosmesh-v1` |
| **AtmosMesh Grove v1.5** — compact ESP8266 node | `atmosmesh-grove-v1.5` | `atmosmesh-v1.5` | `src/products/atmosmesh_grove_v1_5.cpp` | `atmosmesh-grove-v1_5` |
| **AtmosMesh Spot** — ESP32-C3 SuperMini OLED presence node | `atmosmesh-spot-v1` | `atmosmesh-spot-v1` | `src/products/atmosmesh_spot_v1.cpp` | `atmosmesh-spot-v1` |

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
`build`/`flash`/`monitor` and `*-grove` tasks remain compatibility aliases. The Room product adds
`build-room`/`flash-room`/`monitor-room`/`clean-room`.

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

### Temporary OLED/LED visual diagnostic

The canonical `task build-v1-5` image always keeps the four live sensor rows above. For an operator
measurement of the physical active area and installed bi-color LED, an explicit temporary image
compiles `ATMOSMESH_GROVE_VISUAL_DIAGNOSTIC=1`:

```bash
task build-grove-visual-diagnostic
# After independent review and explicit authorization only:
ESP_PORT=/dev/cu.usbserial-0001 task flash-grove-visual-diagnostic
ESP_PORT=/dev/cu.usbserial-0001 task monitor-grove-visual-diagnostic
```

The selected diagnostic and logical geometry are announced before OLED probing, so even a missing
or failed display cannot hide which image is running:

```text
visual-diagnostic: selected oled=full-area geometry=128x32 led=red-green-amber-off phase=2000ms
```

After successful OLED initialization, the image fills the complete logical 128×32 framebuffer with
ON pixels and suppresses every later live-page redraw while sensors, LED and networking keep
running. Serial separately reports `oled-diagnostic: fill applied geometry=128x32 pixels=all-on`.
Missing-address and initialization failures instead report `fill not-applied` with
`reason=oled-not-found` or `reason=oled-init-failed`; the selection banner still appears.

At the same time, the installed D6 red / D0 green LED repeats two seconds red, two seconds green,
two seconds amber (both channels), then two seconds off. Each transition reports the expected color
and actual polarity-aware pin levels. The installed/default common-anode red phase is:

```text
visual-diagnostic-led: color=red D6=LOW D0=HIGH
```

The explicit common-cathode override inverts those levels while preserving the visible sequence.

This is a reversible diagnostic, not a product default. Leave it running until the operator
explicitly asks to restore normal firmware, then use a reviewed `task flash-v1-5`. A filled logical
framebuffer and commanded LED levels do not by themselves prove the size/condition of the physical
lit area or the visible LED colors.

The LDR is uncalibrated digital RC timing: firmware discharges D7 for 1 ms, releases it as an input,
then advances a cooperative time-to-high state machine with a 200 ms hard timeout. A valid
`light_charge_us` value is raw microseconds and **lower means brighter**. Immediate/saturated,
timeout or disconnected states are unavailable. MAX4466 remains dropped.

The firmware samples D7 synchronously immediately after changing it from driven-low to input. If
the released line is already HIGH, that cycle is saturated/unavailable rather than a small plausible
charge time. Normal charge timing continues cooperatively on subsequent loop ticks.

### Bi-color health LED

The installed and canonical Grove profile is common-anode: red D6/GPIO12 and green D0/GPIO16 are
active LOW. Each channel requires its own approximately 330 Ω resistor. Startup prints the exact
compiled polarity and HIGH/LOW levels. No WS2812 library is used. Alternative common-cathode
hardware remains an explicit build-only override:

```bash
task build-grove-common-cathode
```

The normal LED priority is deterministic:

| Priority | Color | Meaning |
| ---: | --- | --- |
| 1 | Red | DHT/BMP or explicit light/soil acquisition error |
| 2 | Red | MQTT is configured but offline; intentionally unconfigured MQTT is neutral |
| 3 | Red | Valid calibration classifies the raw soil value as dry |
| 4 | Amber (red + green) | Soil sample missing, calibration disabled/invalid, or calibrated needs-watering band |
| 5 | Green | Soil is calibrated acceptable and every higher-priority condition is healthy |

A boot-time DHT/BMP value is unknown until the first acquisition attempt and is not itself an
error: with MQTT intentionally unconfigured, the missing pre-soil state is amber. Once a DHT/BMP
cycle has actually run, a missing core reading is red. Configured-but-offline MQTT retains its
higher red priority even during startup.

A light value that is merely uncalibrated or immediately saturated does not by itself turn the LED
red. Before the first soil sample, and after a valid raw sample without validated calibration, the
LED stays amber rather than guessing moisture.

Soil calibration is compile-time metadata. The four macros are
`ATMOSMESH_GROVE_SOIL_CALIBRATION_ENABLED` (`0`/`1`),
`ATMOSMESH_GROVE_SOIL_RAW_DIRECTION` (`1` higher-is-wetter, `2` lower-is-wetter),
`ATMOSMESH_GROVE_SOIL_DRY_CUTOFF_RAW`, and
`ATMOSMESH_GROVE_SOIL_ACCEPTABLE_CUTOFF_RAW`. Both cutoffs must be raw ADC values in `0..1023`.
Higher-is-wetter requires `dry < acceptable`; lower-is-wetter requires `dry > acceptable`.
Unknown direction, out-of-range/equal cutoffs, or direction-inconsistent ordering fails safe to
uncalibrated amber. Serial reports `soil-led-status`, reason, raw value, validation, direction and
cutoffs. MQTT and Home Assistant remain raw-only.

The canonical `atmosmesh-grove-v1_5` environment (and its `esp8266-grove`,
`-visual-diagnostic`, and `-common-cathode` variants) compiles with calibration **enabled**,
from operator dry/wet observation, 2026-08-26: `ATMOSMESH_GROVE_SOIL_RAW_DIRECTION=2`
(lower-is-wetter), `ATMOSMESH_GROVE_SOIL_DRY_CUTOFF_RAW=200`,
`ATMOSMESH_GROVE_SOIL_ACCEPTABLE_CUTOFF_RAW=100` — raw ADC 200+ reads dry (red), raw ADC 100
or below reads well-watered (green), the band between reads needs-watering (amber). The
`native` host-test environment keeps calibration disabled, so its default-disabled unit tests
still exercise the fail-safe path.

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

### Room MQTT contract

The Room image is ESP32, so it uses `esp-mqtt` over the same host-tested contract/session as
AtmosMesh v1 — not Grove's PubSubClient. Without credentials, networking is disabled and the
sensors and TFT continue.

| Piece | Room value |
| --- | --- |
| State | `home/air/atmosmesh-room-0001/state` (not retained, every 5 s) |
| Availability/LWT | `home/air/atmosmesh-room-0001/availability` (`online`/`offline`, retained) |
| Discovery | `homeassistant/{sensor,binary_sensor}/atmosmesh_room_0001/<object_id>/config` (retained) |
| Entities | `temperature_c`, `humidity_pct`, `illuminance_lx`, `pm25`, `pm10`, `motion`, `pm_alarm` |

Room uses the nested `{value, unit, valid, age_ms}` payload shape and `value_template`s of the
form `… if value_json.x.valid else none`, so an entity goes **unavailable** rather than reporting
a number the sensor did not produce. This matters more here than on Grove: an SDS011 that has
stopped sending frames must not appear in Home Assistant as clean air, and a PIR still inside its
60 s warm-up must not publish "no motion" as though it were a measurement. Particulates are
likewise invalid until the sensor's 30 s spin-up has elapsed.

`motion` is a `binary_sensor` with device class `occupancy`. `pm_alarm` is a `binary_sensor` with
device class `problem` and carries the same latch that sounds the beeper, so an automation can
react to what the room heard. State is published every 5 s, and immediately whenever `motion` or
`pm_alarm` changes, so neither waits out the interval.

The client sets an explicit client id (`atmosmesh-room-0001`), a 45 s keepalive and a 30 s
network timeout. The stock 15 s/10 s pair tore the session down about every ten seconds at the
roughly −75 dBm this board sees; `expire_after` stays 90 s, so Home Assistant still marks entities
unavailable long before a dead board could look live. Wi-Fi association is retried with a fresh
`WiFi.begin()` every 30 s while unassociated — a single `begin()` can sit in `WL_DISCONNECTED`
indefinitely on this board.

### Controlled hardware result (2026-08-24)

- Latest reviewed head `4e4a820` flashed successfully: esptool wrote 310,224 bytes, verified the
  hash and hard-reset the ESP8266EX/26 MHz board. More than 30 seconds of serial reported
  `mqtt: connected`, logical LED status `green/healthy`, DHT11 25.0 °C / 36% RH, BMP180
  25.3–25.7 °C / 982.1–982.4 hPa, raw light 420–492 µs and
  `soil: ok adc_raw=214 samples=5 power=off`. These are firmware/runtime diagnostics only: broker
  receipt/HA entities, visible LED colour/polarity and the visible four-row OLED layout remain
  operator-unconfirmed.
- OLED: controller initialization passed at 0x3C as 128×32; pixels remain visually unconfirmed.
- BMP180: runtime passed repeatedly; latest paired observation was 25.6 °C / 983.9–984.0 hPa.
- DHT11: later reported 32.0 °C / 32% RH then 31.0 °C / 32% RH on D5/GPIO14. This proves
  communication, not accuracy or calibration.
- RC light: serial showed uncalibrated 389–452 µs values; controlled bright/dark/saturation/timeout
  response remains pending. MQTT broker/HA receipt and reconnect behavior are not yet validated.
- YL-38: serial showed two cycles about 30 seconds apart, each
  `soil: ok adc_raw=214 samples=5 power=off`. This proves raw acquisition and the firmware OFF action,
  not calibration, switched-rail voltage/current or physical power-off.
- Bi-color LED wiring was subsequently confirmed by the operator as D6 red / D0 green,
  common-anode: only the active-LOW/inverted diagnostic produced the intended colors. Soil
  calibration and its threshold-driven colors remain unvalidated.
- Independently approved canonical head `8fca62d` was then flashed successfully. Serial reported
  raw soil ADC 213 and logical amber with common-anode output levels `red=LOW green=LOW`, reason
  `soil-calibration-needed`, calibration disabled, direction unknown and both thresholds unset.
  This is logical policy/pin-drive evidence, not visible colour, physical power-off or calibration
  evidence; those operator checks remain pending.
- The first captured banner from reviewed head `a681990` contained product name, variant and station
  ID but no separate `product_id`. A second reviewed flash of final head `50ca2f3` captured the exact
  four-field banner documented above.

## AtmosMesh Spot (SP-01 / SP-02)

ESP32-C3 SuperMini with the 0.42 inch OLED on the 14 × 20 carrier
([wiring](../hardware/kicad/atmosmesh-spot/wiring.md)). One 3.3 V domain, USB powered. Needs
`espressif32` 7.x (Arduino core 3.x, RISC-V toolchain); the shared agent venv does not carry
PlatformIO on Linux, see `tools/c3scan/README.md` for the venv recipe and two gotchas.

```bash
pio run -e atmosmesh-spot-v1                                   # build
pio run -e atmosmesh-spot-v1 -t upload --upload-port /dev/ttyACM0   # flash over native USB
pio device monitor --port /dev/ttyACM0 --baud 115200
```

or `task build-spot` / `ESP_PORT=/dev/ttyACM0 task flash-spot` / `task monitor-spot`. The board
enumerates as `Espressif USB JTAG/serial debug unit` (303a:1001); the user needs `dialout`.

| Device | SuperMini pin | GPIO | Notes |
| --- | --- | --- | --- |
| OLED SSD1306 72×40 | on board | SDA 5 / SCL 6 | `0x3C`, U8g2 `72X40_ER`, 100 kHz |
| SHT41 (`SHT4X` board) | `5` / `6` | same bus | `0x44`, raw I²C, host-tested CRC |
| VEML7700 (`HW-900`) | `5` / `6` | same bus | `0x10`, Adafruit driver, auto-ranging lux |
| HLK-LD2410S `OT2` | `3` | GPIO3 | presence, HIGH = somebody, `INPUT_PULLDOWN` |
| HLK-LD2410S `OT1` (its TX) | `RX` | GPIO20 | UART0 RX, 115200 8N1, minimal + standard frames |
| HLK-LD2410S `RX` | `TX` | GPIO21 | UART0 TX |
| DS18B20 probe | `4` | GPIO4 | 1-Wire, powered mode, 4.7 kΩ on the carrier |
| BOOT button / IO8 LED | on board | GPIO9 / GPIO8 | next display page / heartbeat toggle |

**U8g2 on Arduino core 3.x:** construct the display **without** pin numbers and start `Wire`
yourself. Given pins, U8g2's GPIO init calls `pinMode()` on SDA and SCL, the core's peripheral
manager hands the pins back from the I²C driver, and `oled.begin()` never returns — observed on
the first unit on 2026-09-05 (sensors answered before the OLED init, nothing after).

**The radar's UART, four things learned the hard way (2026-09-05):**

1. **Never leave a pull-down on the RX pin.** The C3 attaches UART0 RX to GPIO20 through its
   direct mux and does not touch the pull resistors, so a `pinMode(20, INPUT_PULLDOWN)` from a
   wiring check stays on the line. The LD2410S's `OT1` is a weak driver: it reads LOW against
   that pull-down and HIGH against a pull-up, and its frames only decode with the pull-up. The
   firmware reads the line against both pulls at boot (`pull-down->LOW pull-up->HIGH` is the
   healthy reading on this module; LOW under the pull-up means a joint), then leaves
   `GPIO_PULLUP_ONLY` on RX after `Serial0.begin()`.
2. **The module needs a break to start streaming.** After a reset it sends two or three frames
   and waits; a 200 ms low on its RX (our TX) starts the continuous report. The boot kick is
   break plus end-configuration, and the same nudge repeats if the stream stops; a full
   enable/read-version/output-mode/end sequence follows if that fails. Every ACK is logged.
3. **IDF logging is silenced** (`esp_log_level_set("*", ESP_LOG_NONE)`): the IDF console is
   UART0, i.e. the radar's RX, and the module answers log lines with an enable-config ACK every
   time esp-mqtt retried. The ROM and bootloader lines at reset still reach it; they are what
   produced the "ACK at every reset" that first proved the transmitter alive.
4. **The VEML7700 is read at a fixed gain (1/4, 100 ms) without waiting.** `VEML_LUX_AUTO`
   took 5.1 s per sample in a dark room and stalled the radar drain, the presence poll and the
   button. Every 5 s line ends with the slowest loop stage; it is now the 43 ms OLED redraw.

`radar: OT1 line … pull-down->LOW pull-up->LOW - HELD LOW` is the one boot line that means a
wiring fault; RX LOW with TX HIGH under the pull-down means `OT1` landed on the TX pin (the UART
is then run crossed for that boot and the log says so).

### Spot MQTT contract

Same `esp-mqtt` transport as Room, through `esp32_mqtt_runtime.cpp`, which takes the product
contract as a parameter (Room still runs its own file). Without credentials the sensors and OLED
continue and networking stays off.

| Piece | Spot value |
| --- | --- |
| State | `home/air/atmosmesh-spot-0001/state` (not retained, every 5 s, immediately on a presence flip) |
| Availability/LWT | `home/air/atmosmesh-spot-0001/availability` (`online`/`offline`, retained) |
| Discovery | `homeassistant/{sensor,binary_sensor}/atmosmesh_spot_0001/<object_id>/config` (retained) |
| Entities | `temperature_c`, `humidity_pct`, `illuminance_lx`, `probe_temperature_c`, `presence_distance_cm`, `presence_state`, `wifi_rssi_dbm`, `presence` |

Nested `{value, unit, valid, age_ms}` payloads as on Room. `presence` is a `binary_sensor` with
device class `occupancy` from `OT2` through a 50 ms debounce and a 5 s hold, valid once a radar
is known to be attached (frames seen, or `OT2` ever high) and the 10 s warm-up is over.
`presence_distance_cm` (device class `distance`) and `presence_state` (the radar's raw byte:
0/1 nobody, 2/3 somebody; the manual defines nothing finer) come from the UART report and are
valid only while frames keep arriving. `probe_temperature_c` goes unavailable after three bad
scratchpads and the probe is searched for again every 10 s. Wi-Fi TX power is limited to
8.5 dBm at start-up (the SuperMini's ceramic antenna drops off the network at full power).

### First unit (2026-09-05)

Soldered carrier: bus scan `0x10 0x3C 0x44`. Product image: SHT41 27.7 °C / 61 %RH, VEML7700
9–68 lx, DS18B20 probe 24.9 °C, presence occupied from `OT2`, Wi-Fi −72 dBm channel 11 with the
limit applied, `mqtt: connected` as `atmosmesh-spot-0001`; RAM 12.6 %, flash 72.7 %. The radar's
`OT1` was genuinely open at first (the pin floated, no edge on any free GPIO, no ACK to a version
command); the operator reflowed the module's chip, after which the four points above were found
one by one. End state: `<- ack end-config status=0` at boot and a continuous minimal-frame stream
(about 3–4 frames/s, distance and state on the display and in MQTT). The SHT41 sits about 3 K
above the probe and climbs as the board warms. The radar kept reporting a target at ~71 cm after
the operator said they had left the room — the empty-room test in SP-01 is next.

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
| `src/grove_visual_diagnostic.cpp` | Host-tested temporary OLED-fill and LED-cycle policy/logging |
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

Then `direnv allow` and use `task build-v1`, `task build-v1-5`, `task build-room`, or
`task build-all` (and the matching
reviewed flash command only when authorized). Both product build/flash tasks run
`scripts/gen-secrets-from-env`, which writes gitignored
`firmware/include/atmosmesh/secrets.hpp` (also works from a git worktree by reading the
main-checkout `.envrc`).

Alternative: copy `include/atmosmesh/secrets.hpp.example` → `secrets.hpp` by hand.
Without Wi-Fi + MQTT credentials, either image still samples sensors and drives the OLED;
networking is skipped. Topics and HA discovery are in `docs/architecture.md` (D-007/D-013).

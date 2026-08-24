# Known hardware and safety status

This file contains only project-relevant hardware. Exact module variants remain unverified until
RLS-01 records front/back photos and markings. Manufacturer PDFs and a chip-level comparison live
in [datasheets/](datasheets/README.md) and [spec-comparison.md](spec-comparison.md).

| Component | Intended role | Supply | Interface | Verification status |
| --- | --- | --- | --- | --- |
| ESP-WROOM-32 devboard | Controller, Wi-Fi, MQTT | Station: `VIN`/`5V` from the 5 V rail. Bench: USB until PSU is enclosed and measured | 3.3-V GPIO | USB probe 2026-08-14: ESP32-D0WDQ6 rev 1.0, 4 MB flash, CP2102, MAC `ac:67:b2:37:26:78`. `VIN`/`5V` silkscreen still pending |
| ESP8266EX / ESP8266MOD, NodeMCU-style D labels | **AtmosMesh Grove v1.5** controller | USB; fitted modules from `3V3` | 3.3-V GPIO | Probe: ESP8266EX, 26 MHz, 4 MB. Reviewed AtmosMesh Grove image flashed and booted 2026-08-24; former AT firmware replaced under authorization |
| SDS011 (Nova; board often labelled d011v2) | PM2.5 and PM10 | 5 V (4.7–5.3 V, > 1 W) | UART2 9600 8N1, 3.3-V TTL | **GPIO16/17 only.** Never RX0/TX0 (GPIO3/1) |
| DHT22 / AM2302 | Temperature and humidity | **3V3** | Single-wire data on GPIO18 | Operator: data = D18/GPIO18. Idle-high; 10 kΩ pull-up to 3V3 if the module has none |
| GY-BMP280 | Pressure and temperature | **3V3** (chip 1.71–3.6 V) | I²C (6-pin module) | Operator: VCC, GND, SCL, SDA, CSB, SDO. Straps below. Regulator/5 V still unconfirmed |
| Mini OLED, 4 pins | Station local status (D-001) | **3V3** | I²C **0x3C** (fallback 0x3D) | Serial 2026-08-14: ACK at 0x3C, SDA=GPIO5, SCL=GPIO4. Cheap 0.96" SSD1306: alternate COM 0x12 skips rows; firmware default is U8g2 SSD1306 ALT0 (COM 0x02 sequential). SH1106 is compile fallback ID=1 |
| LCD 1602 I²C | Not the product display | **3V3 only** if reused | I²C backpack 0x27/0x3F | Spare. Station firmware drives the OLED, not this panel |
| MQ135 module | Experimental gas trend | 5 V heater | Analog through divider | Bench 10 kΩ series + 20 kΩ to GND on GPIO34. Never label as CO₂ |
| Open AC/DC `5V07 / 12V04` | Candidate station 5 V rail | 230 V AC primary | DC output unverified | **Must measure** before use. Family is 5 V/700 mA *or* 12 V/~400 mA. Open mains PCB — enclose first. See [power.md](power.md) |
| SANMIM SM-PLG06A / SM-104-3.3V-02 | Spare 3.3 V AC/DC | 230 V AC primary | 3.3 V | Not required for MVP; do not parallel with ESP32 `3V3`. Open mains PCB |

## AtmosMesh Grove v1.5 — fitted 3.3 V slice (2026-08-24)

This is a separate ESP8266 product variant, not a replacement for the ESP32 station above.

| Component | Interface | Grove wiring | Status |
| --- | --- | --- | --- |
| 128×32 SSD1306 OLED | I²C, normally 0x3C (0x3D fallback) | SDA=`D2`/GPIO4, SCL=`D3`/GPIO0, VCC=`3V3` | **Initialization pass:** `oled: ok addr=0x3C geometry=128x32`; pixels not visually confirmed |
| BMP180 | I²C, 0x77 | Shares SDA=`D2`/GPIO4 and SCL=`D3`/GPIO0, VCC=`3V3` | **Runtime pass:** repeated; latest four samples, 24.6–24.7 °C and 984.3 hPa |
| Blue DHT11 | Single-wire | DATA=`D5`/GPIO14, VCC=`3V3` | **Communication pass:** valid frames observed (32.0→31.0 °C, 32% RH); not accuracy/calibration evidence |
| Bare LDR RC | Digital RC timing | `3V3 → LDR → 1 kΩ → D7`/GPIO13 node; 100 nF node-to-GND | **Runtime communication pass:** 389–452 µs observed. Bright/dark/saturation/timeout response pending; raw µs only, lower means brighter, never lux/percent |
| Bi-color LED | Two digital channels | Red=`D6`/GPIO12, green=`D0`/GPIO16; separate ~330 Ω resistors | Installed per operator correction. Default common-cathode; compiled common-anode inversion available. Hardware color/polarity validation pending |
| YL-38 + probe | Switched analog | AO→47 kΩ→A0, 15 kΩ A0→GND; 100 nF (`104`) A0→GND; DO unused | **Runtime raw pass:** two cycles about 30 s apart, each ADC 214 averaged from five samples. No calibration claim |
| 2N3906 PNP high-side switch | YL-38 VCC control | Emitter=3V3, collector=YL VCC, base=`D1`/GPIO5 through 2.2 kΩ, 100 kΩ base-emitter pull-up | Confirmed alternative physically wired by operator; active LOW, fail-safe OFF |

### Controlled Grove flash and boot (2026-08-24)

With explicit authorization and independent approval of head `a681990`, the coordinator ran
`ESP_PORT=/dev/cu.usbserial-0001 task flash-v1-5`. Esptool identified the same ESP8266EX, 26 MHz
crystal and 4 MB flash recorded earlier, wrote 287,920 bytes, verified the hash and hard-reset the
board. Boot reported:

```text
product=AtmosMesh Grove variant=atmosmesh-v1.5 station_id=atmosmesh-grove-0001
i2c: SDA=D2/GPIO4 SCL=D3/GPIO0 clock=100000 Hz
boot-warning: D3/GPIO0 must remain HIGH during reset
```

This replaced the working AT firmware under authorization. AtmosMesh v1 and its ESP32 were not
flashed or otherwise changed. For DHT11, do not infer a cause from the error alone: verify actual
DATA joint/pin, 3.3 V/GND orientation and a 4.7–10 kΩ pull-up (or resistor on the module). A serial
OLED init line proves controller communication, not visible pixels.

The captured `a681990` banner above had no separate stable product ID. After independent approval
of final head `50ca2f3`, the coordinator ran `task flash-v1-5` again. Esptool identified the same
ESP8266EX/4 MB board, wrote 287,952 bytes, verified the hash, hard-reset it and captured:

```text
product=AtmosMesh Grove product_id=atmosmesh-grove-v1.5 variant=atmosmesh-v1.5 station_id=atmosmesh-grove-0001
```

The I²C configuration and power/boot warnings were correct. OLED initialization again passed at
0x3C/128×32, but pixels remain visually unconfirmed. BMP180 passed four samples at 24.6–24.7 °C /
984.3 hPa. DHT11 remained unavailable in all four observed cycles.

`D3` is ESP8266 GPIO0, a boot strap. It must remain high during reset; if the I²C bus or a
module pulls it low, the board enters ROM download mode. Firmware preserves the actual wiring with
`Wire.begin(4, 0)` and logs the constraint rather than silently assigning another pin.

The DHT11 later returned valid frames on D5/GPIO14 while BMP180 reported 25.6 °C and
983.9–984.0 hPa. This is a communication pass, not DHT11 accuracy proof.

MAX4466 will not be used. The installed LDR uses bounded D7 RC timing; V15-06 assigns A0 only to the
divided YL-38 AO path and exposes raw ADC counts. GPIO never powers the YL board. If its VCC is tied
directly to 3V3, firmware cannot limit probe duty and must not claim corrosion control.

After independent approval of the light/MQTT product diff at `e5d83e1` and green CI, an authorized
follow-up upload rebuilt the image and generated secrets successfully. Before esptool opened the
port, `/dev/cu.usbserial-0001` disappeared; upload ended with `FileNotFoundError`. No chip connection
or write began, so zero bytes from `e5d83e1` reached the board and its prior firmware is unchanged.
At that point `ls /dev/cu.*` showed no USB serial device and ioreg showed hubs only. Reconnect and
confirm the serial port before retrying light/MQTT validation.

The USB serial device later reappeared. After independent approval of head `c880afe`, the
coordinator ran `ESP_PORT=/dev/cu.usbserial-0001 task flash-v1-5`. Esptool identified ESP8266EX,
26 MHz crystal and MAC `2c:3a:e8:43:7c:16`, wrote 310,288 bytes, verified the hash and hard-reset.
More than 60 seconds of 115200-baud serial contained two soil cycles about 30 seconds apart, both
exactly `soil: ok adc_raw=214 samples=5 power=off`. DHT11 was stable at 25.0 °C / 36–37% RH,
BMP180 at 24.4–24.5 °C / 982.6–982.8 hPa, uncalibrated light at 389–452 µs and core sensor health
was okay. The OFF text proves the firmware action, not switched-rail voltage or current. LED
colors/polarity, physical power-off, probe calibration, MQTT broker/HA receipt and OLED pixels
remain unconfirmed.

## USB identity of the connected controller (2026-08-14)

Read-only `esptool` probe on `/dev/cu.usbserial-0001`. Nothing was flashed.

| Fact | Value |
| --- | --- |
| USB-UART | Silicon Labs CP2102 (`10c4:ea60`), serial `0001` |
| Chip | ESP32-D0WDQ6 revision v1.0 |
| Flash | 4 MB, 3.3 V |
| Crystal | 40 MHz |
| MAC | `ac:67:b2:37:26:78` |
| Auto-reset | RTS works (BOOT button not required for esptool) |

This matches the ESP32-WROOM-32 module datasheet (embedded ESP32-D0WDQ6, 4 MB flash, 40 MHz).
The **devboard** name and pin labels are still unconfirmed.

## GY-BMP280 — 6-pin module (operator, 2026-08-14)

Confirmed labels: **VCC, GND, SCL, SDA, CSB, SDO**. That is the usual GY-BMP280 (I²C + SPI).
Bosch sheet: I²C vs SPI is selected by CSB; the 7-bit address LSB is SDO. Neither strap may float.

| Module pin | Role | Bench wiring for I²C |
| --- | --- | --- |
| VCC | Module supply | ESP32 **3V3** (not 5 V until a regulator on the breakout is photographed) |
| GND | Ground | ESP32 GND |
| SCL | I²C clock | **GPIO19** (operator, 2026-08-14). Not GPIO22 |
| SDA | I²C data | **GPIO21** |
| CSB | Interface select | **3V3** — high = I²C. Low = SPI. Leave-open is invalid |
| SDO | I²C address LSB | **GND** → address **0x76**. Tie to 3V3 → 0x77 |

CSB and SDO are local straps to 3V3/GND. They do not go to ESP32 GPIOs for I²C.

If VCC is 5 V and the module’s I²C pull-ups sit on VCC, SDA/SCL become 5 V and will damage the ESP32. Until the back of the board is photographed, **3V3 only**.

## Provisional signal map — not approved for construction

| Signal | ESP32 pin | Notes |
| --- | --- | --- |
| I²C SDA (GY-BMP280) | GPIO21 | Operator, 2026-08-14 |
| I²C SCL (GY-BMP280) | GPIO19 | Operator, 2026-08-14. Not GPIO22 or GPIO18 |
| BMP280 CSB | 3V3 | I²C mode; not an ESP32 GPIO |
| BMP280 SDO | GND | Address 0x76; not an ESP32 GPIO |
| I²C SDA (SSD1306 OLED) | GPIO5 / D5 | D-001; serial-proven 0x3C. Idle-high pull-up is usually OK for boot |
| I²C SCL (SSD1306 OLED) | GPIO4 / D4 | Power OLED VCC from 3V3, never 5 V |
| AM2302 data | GPIO18 / D18 | Operator, 2026-08-14. Idle-high OK (3.3 V flash-voltage strap) |
| SDS011 TX | GPIO16 / RX2 | Sensor TX → ESP32 RX2. **Not** RX0 / GPIO3 |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX2 → sensor RX. **Not** TX0 / GPIO1 |
| USB console (CP2102) | GPIO1 TX0, GPIO3 RX0 | `task flash` / `task monitor` only. Leave the USB cable as the only UART0 user |
| MQ135 analog | GPIO34 / ADC1 | Input-only. Bench divider **10 kΩ series + 20 kΩ to GND** (GPIO = 2/3 AOUT). 5 V AOUT → **3.33 V** on GPIO34 — **no headroom**. Never 5 V on the pin. Not UART. Firmware logs raw/mV, never CO₂ |

## Bench mix-up (2026-08-14) — SDS011 on RX0/TX0

Operator wired **d011v2** (SDS011 v2) to **RX0/TX0** (GPIO3/GPIO1). That is the USB-UART used for
`task flash` / `task monitor`. Firmware talks to SDS011 on **UART2 only** (`Serial2` on GPIO16/17);
it will not be moved to UART0. Unplug the sensor from RX0/TX0 immediately. Correct: sensor TX →
GPIO16, GPIO17 → sensor RX, **5 V on sensor VCC only**, UART **3.3 V**. USB cable stays on the
DevKit as the only UART0 user. A live SDS011 on UART0 garbles the boot log, can stall the app
(OLED looks “dead”), and fights the CP2102.

## Bench mix-up (2026-08-14) — MQ135 vs SDS011 on UART2

Operator wired a module called “MQ13” to RX2/TX2 (GPIO16/17). **MQ135 is analog AOUT + 5 V heater;
it has no UART.** SDS011 is the UART sensor on those pins (sensor TX → GPIO16, ESP TX → GPIO17).
Firmware does not bit-bang analog on GPIO16/17. If the gas board is still on RX2/TX2, unplug it
before applying 5 V. OLED blanking with a live `oled: init ok` is not explained by UART2; a 5 V
short onto GPIO can brown out `3V3` and blank the glass — serial on 2026-08-14 showed **no**
brownout and I²C 0x3C/0x76 still present.

## Bench bring-up (2026-08-17) — 3V3 sensors pass, both 5 V sensors fail

First capture with everything soldered, 20 s window, board stable (`rst:0x1 (POWERON_RESET)`,
one boot banner, no loop). Follows the LDO replacement in
[incident-2026-08-17-ldo.md](incident-2026-08-17-ldo.md).

| Sensor | Rail | Result |
| --- | --- | --- |
| AM2302 | 3V3 | **Pass** — `t=28.4C rh=39.2–41.4%`, stable |
| BMP280 | 3V3 | **Pass** — `t=29.7C p=973.2 hPa` |
| SSD1306 OLED | 3V3 | **I²C pass** — `init ok … addr=0x3C 128x64`. Pixels unconfirmed |
| PIR | 3V3 | Inconclusive — `pir: idle` proves the pull-down only |
| VEML7700 | 3V3 | Not fitted — `not found (ok until fitted)` |
| **SDS011** | **5 V** | **Fail** — 1546 no-frame lines / 20 s, **0 valid frames** |
| **MQ135** | **5 V** | **Fail** — erratic `raw=` 1097, 119, 0, 38, 724, 123 |

**The failures partition exactly along the supply rail:** every 3.3 V device works, both 5 V
devices do not. Per [power.md](power.md), SDS011 and MQ135 are the only 5 V loads.

This is a **lead, not a conclusion**. At least two stories fit equally well:

1. A common 5 V feed is dead or marginal.
2. Two unrelated faults — MQ135 strapped to 3V3 via `3V3_5V_SEL`, *and* SDS011 UART wired
   straight-through instead of crossed. The bench schematic has an independent `3V3_5V_SEL`
   strap, so the two rails are not necessarily one net.

Parsimony is not evidence. Discriminate physically before rewiring anything:

- **Is the SDS011 fan spinning?** Spinning → it has 5 V, so its fault is the UART and the
  rail story collapses to MQ135 alone. Still → no 5 V.
- **Is the MQ135 warm after a minute?** A powered heater is unmistakably warm to the touch.
  Cold means unpowered whatever a meter reads at the header.

Supporting note on MQ135: it read ~1250 in a narrow band *before* soldering (floating pin) and
now reads mostly 0–123 with occasional spikes. With the 10 k series + 20 k-to-GND divider fitted
and nothing driving AOUT, the pin is pulled toward ground — *consistent with* an undriven AOUT.
Not proof; a faulty ADC would look similar.

### SDS011 UART crossing (the rule the current wiring may violate)

From the pin table above — the sensor connector is **crossed**, not straight-through:

| SDS011 pin | Goes to | Never |
| --- | --- | --- |
| **TX** (datasheet pin 7) | **GPIO16 / RX2** | not TX2/GPIO17, not RX0/GPIO3 |
| **RX** (datasheet pin 6) | **GPIO17 / TX2** | not RX2/GPIO16, not TX0/GPIO1 |

Wiring sensor TX → TX2 also puts two push-pull outputs on one net (ESP32 TX2 drives, sensor TX
drives). That is contention, not just silence.

Firmware says this itself on every failed poll: `sds011: no AA C0 frame (listening GPIO16/RX2;
sensor TX must not sit on TX2/GPIO17)`.

**Listen-only test if desoldering is expensive:** flash with `Serial2.begin(9600, SERIAL_8N1,
17, -1)`. Frames appear → sensor TX is on GPIO17 and the pair is straight-through. Use `-1` for
TX so GPIO16 is never driven into the sensor's TX if the wiring was in fact correct.

### SDS011 confirmed wired straight-through — and GPIO17 has two drivers on it

Operator verified 2026-08-17: **SDS011 TX → TX2/GPIO17, SDS011 RX → RX2/GPIO16.** That is
straight-through. The pin table above requires it **crossed**: sensor TX → GPIO16, GPIO17 →
sensor RX. This fully explains 0 valid frames — the ESP32 listens on GPIO16, where only the
sensor's *receive* pin is connected, so nothing ever arrives.

> **Contention hazard, live right now.** The firmware configures GPIO17 as UART2 **TX**, a
> push-pull output. The SDS011's TX is also a push-pull output. Both are soldered to the same
> net and fight whenever they drive opposite levels — roughly 10 ms in every second, each time
> the sensor transmits its 10-byte frame at 9600 baud. ESP32 GPIO absolute max is 40 mA and a
> driver fight can exceed it. Fix this before leaving the bench powered for long runs.

Two fixes, and they are not equivalent:

| Fix | Effort | Consequence |
| --- | --- | --- |
| **A. Re-solder crossed** | Desolder two joints | Bench matches the pin table, the KiCad board and every doc. Contention gone. |
| **B. Swap firmware constants** — `kSds011RxGpio = 17`, `kSds011TxGpio = 16` | One-line, no iron | Works, and also ends the contention (GPIO17 becomes an input). But the bench now **diverges** from the routed PCB and the pin table, so both must be annotated or the next board repeats the fault. |

B is the faster confirmation that the diagnosis is right, since UART2 remaps freely through the
ESP32 GPIO matrix and both pins are ordinary I/O. A is the correct end state for a board whose
schematic is already routed. Either way the ESP32 stops driving GPIO17.

Note the SDS011 reports autonomously at 1 Hz and needs no commands, so the ESP32→sensor
direction is optional; only "sensor TX → an ESP32 receive pin" actually matters.

### Flashing is intermittent, not blocked by a dead pin

Attempting `pio run -t upload` now fails, twice, identically:

```text
Download mode successfully detected, but getting no sync reply:
The serial TX path seems to be down.
```

Read that carefully — it is **directional**:

| Direction | Path | State |
| --- | --- | --- |
| ESP32 → host | GPIO1 / TX0 → CP2102 | **Works** — 52 kB of boot + app log read back |
| Host → ESP32 | CP2102 → GPIO3 / RX0 | **Intermittent** — 12 kB written, then no sync reply |
| Auto-reset | DTR/RTS → EN, IO0 | Works — download mode *is* entered |

The chip and its reset circuit are healthy. The wording "TX path seems to be down" is esptool's
guess after one lost reply and should not be read as a dead pin — see the correction below.

**Corrected 2026-08-17: this is not a dead GPIO3.** An earlier revision of this note blamed a
loaded RX0. Two pieces of evidence refute it:

1. Operator verified the SDS011 joints: its TX is on **TX2/GPIO17**, its RX on **RX2/GPIO16**.
   Nothing is on RX0/GPIO3.
2. The first upload attempt logged `Writing at 0x00001000... (100 %)` — a **fully successful
   host→chip write of 12 kB** — before losing the connection. A dead receive pin cannot do that.

So the link is **intermittent, not broken**. Prime suspect is the USB path, consistent with the
`tcsetattr: Invalid argument` wedge seen on the same port earlier the same day: `ioreg` showed
the CP2102 sitting behind a **VIA Labs USB2.0 hub chain**. Try a direct port on the Mac, a
different cable, and a lower `upload_speed` before suspecting the board.

**The old app survived this attempt** — by luck of ordering, not by design. esptool announced
`Flash will be erased from 0x00010000 to 0x00062fff`, but the erase evidently never executed:
the board still boots clean (`POWERON_RESET`) and `bmp280`, `am2302`, `mq135` and
`oled: init ok` all still report. Note that this was verified over serial, which is the very
channel now proven half-broken — treat it as good evidence, not proof.

> **Improve the USB link before the next flash attempt.** The hazard is the retry itself: an
> upload that gets one step further erases the app region and can then still fail to write it,
> leaving the board with no application. That is recoverable — the link is intermittent, not
> dead — but only by flashing again, so do not spend the working app on a link that is known
> flaky. Direct USB port, known-good cable, lower `upload_speed` first; then flash once.

### Firmware issues found in the same capture

- `firmware/src/products/atmosmesh_v1.cpp` — the SDS011 no-frame log is emitted every `loop()`
  iteration with no rate limit: **~93 lines/s, 96.6 % of all serial output**. It buries every other
  subsystem.
  MQ135 (gated at 2 s by `kAm2302MinIntervalMs`) appears at roughly 1 line per 300. Needs a
  `millis()` throttle.
- `firmware/src/display_text.cpp:85` — `(void)mq135_raw_adc;` discards the MQ135 value, so it
  **cannot** appear on the OLED regardless of wiring.

## Bench MQ135 divider (operator 2026-08-14)

Wired: **10 kΩ series** between MQ135 AOUT and GPIO34, **20 kΩ** from GPIO34 to GND.

`V_GPIO34 = V_AOUT * 20k / (10k+20k) = 2/3`. At 5 V AOUT the ESP32 pin sees **3.33 V**, which is
the 3.3 V absolute max with **no headroom**. Do not apply 5 V to GPIO34. If the ADC reads ~0,
AOUT / GND / 5 V heater wiring may be wrong. Firmware estimates AOUT millivolts by inverting
that 2/3 ratio; it does **not** claim CO₂ or ppm.

KiCad J5 still documents 10 kΩ / 15 kΩ (3.0 V at 5.0 V in). The live bench is 10 k / 20 k.

Datasheet support for these electrical choices is recorded in [spec-comparison.md](spec-comparison.md).
Photos still block construction.

## Non-negotiable limits

- Never wire SDS011 (d011v2) to RX0/TX0 (GPIO3/GPIO1). Those pins are USB console only.
- Never apply 5 V to an ESP32 GPIO or `3V3` pin. GPIO VIH max is VDD + 0.3 V. Station 5 V may enter only a confirmed `USB` or `VIN`/`5V` pad.
- Do not trust generic diagrams for connector order.
- Do not power the full assembly before RLS-01 approves an evidence-backed wiring table.
- Do not put mains on a breadboard. Enclose AC/DC primaries before energising.
- The earlier AI-generated pictorial diagram from the planning conversation is rejected.
- A later deterministic concept diagram is still only conceptual until actual modules are verified.
- MQ135 is not a CO₂ sensor.

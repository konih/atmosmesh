# Known hardware and safety status

This file contains only project-relevant hardware. Exact module variants remain unverified until
RLS-01 records front/back photos and markings. Manufacturer PDFs and a chip-level comparison live
in [datasheets/](datasheets/README.md) and [spec-comparison.md](spec-comparison.md).

| Component | Intended role | Supply | Interface | Verification status |
| --- | --- | --- | --- | --- |
| ESP-WROOM-32 devboard | Controller, Wi-Fi, MQTT | Station: `VIN`/`5V` from the 5 V rail. Bench: USB until PSU is enclosed and measured | 3.3-V GPIO | USB probe 2026-08-14: ESP32-D0WDQ6 rev 1.0, 4 MB flash, CP2102, MAC `ac:67:b2:37:26:78`. `VIN`/`5V` silkscreen still pending |
| SDS011 (Nova; board often labelled d011v2) | PM2.5 and PM10 | 5 V (4.7–5.3 V, > 1 W) | UART2 9600 8N1, 3.3-V TTL | **GPIO16/17 only.** Never RX0/TX0 (GPIO3/1) |
| DHT22 / AM2302 | Temperature and humidity | **3V3** | Single-wire data on GPIO18 | Operator: data = D18/GPIO18. Idle-high; 10 kΩ pull-up to 3V3 if the module has none |
| GY-BMP280 | Pressure and temperature | **3V3** (chip 1.71–3.6 V) | I²C (6-pin module) | Operator: VCC, GND, SCL, SDA, CSB, SDO. Straps below. Regulator/5 V still unconfirmed |
| Mini OLED, 4 pins | Station local status (D-001) | **3V3** | I²C **0x3C** (fallback 0x3D) | Serial 2026-08-14: ACK at 0x3C, SDA=GPIO5, SCL=GPIO4. Cheap 0.96" SSD1306: alternate COM 0x12 skips rows; firmware default is U8g2 SSD1306 ALT0 (COM 0x02 sequential). SH1106 is compile fallback ID=1 |
| LCD 1602 I²C | Not the product display | **3V3 only** if reused | I²C backpack 0x27/0x3F | Spare. Station firmware drives the OLED, not this panel |
| MQ135 module | Experimental gas trend | 5 V heater | Analog through divider | Bench 10 kΩ series + 20 kΩ to GND on GPIO34. Never label as CO₂ |
| Open AC/DC `5V07 / 12V04` | Candidate station 5 V rail | 230 V AC primary | DC output unverified | **Must measure** before use. Family is 5 V/700 mA *or* 12 V/~400 mA. Open mains PCB — enclose first. See [power.md](power.md) |
| SANMIM SM-PLG06A / SM-104-3.3V-02 | Spare 3.3 V AC/DC | 230 V AC primary | 3.3 V | Not required for MVP; do not parallel with ESP32 `3V3`. Open mains PCB |

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

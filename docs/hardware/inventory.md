# Known hardware and safety status

This file contains only project-relevant hardware. Exact module variants remain unverified until
RLS-01 records front/back photos and markings. Manufacturer PDFs and a chip-level comparison live
in [datasheets/](datasheets/README.md) and [spec-comparison.md](spec-comparison.md).

| Component | Intended role | Supply | Interface | Verification status |
| --- | --- | --- | --- | --- |
| ESP-WROOM-32 devboard | Controller, Wi-Fi, MQTT | Station: `VIN`/`5V` from the 5 V rail. Bench: USB until PSU is enclosed and measured | 3.3-V GPIO | USB probe 2026-08-14: ESP32-D0WDQ6 rev 1.0, 4 MB flash, CP2102, MAC `ac:67:b2:37:26:78`. `VIN`/`5V` silkscreen still pending |
| SDS011 | PM2.5 and PM10 | 5 V (4.7–5.3 V, > 1 W) | UART 9600 8N1, 3.3-V TTL | Datasheet pin functions known; connector/adapter order pending |
| DHT22 / AM2302 | Temperature and humidity | **3V3** | Single-wire data on GPIO5 | Operator: data = pin 5. Idle-high; 10 kΩ pull-up to 3V3 if the module has none |
| GY-BMP280 | Pressure and temperature | **3V3** (chip 1.71–3.6 V) | I²C (6-pin module) | Operator: VCC, GND, SCL, SDA, CSB, SDO. Straps below. Regulator/5 V still unconfirmed |
| Mini OLED, 4 pins | Station local status (D-001) | 3.3 V planned | I²C 0x3C/0x3D expected | SSD1306 vs SH1106 and pin order pending |
| LCD 1602 I²C | Bench dummy-text bring-up (D-006) | **3V3 only** | I²C on D2/D4 | Operator wired D2+D4. Firmware: SDA=GPIO2, SCL=GPIO4, swap retry |
| MQ135 module | Experimental gas trend | 5 V heater | Analog through divider | Output range pending; never label as CO₂ |
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
| SCL | I²C clock | **GPIO18** (operator, 2026-08-14). Not GPIO22 |
| SDA | I²C data | **GPIO21** |
| CSB | Interface select | **3V3** — high = I²C. Low = SPI. Leave-open is invalid |
| SDO | I²C address LSB | **GND** → address **0x76**. Tie to 3V3 → 0x77 |

CSB and SDO are local straps to 3V3/GND. They do not go to ESP32 GPIOs for I²C.

If VCC is 5 V and the module’s I²C pull-ups sit on VCC, SDA/SCL become 5 V and will damage the ESP32. Until the back of the board is photographed, **3V3 only**.

## Provisional signal map — not approved for construction

| Signal | ESP32 pin | Notes |
| --- | --- | --- |
| I²C SDA (GY-BMP280) | GPIO21 | Operator, 2026-08-14 |
| I²C SCL (GY-BMP280) | GPIO18 | Operator, 2026-08-14. Not the original GPIO22 plan |
| BMP280 CSB | 3V3 | I²C mode; not an ESP32 GPIO |
| BMP280 SDO | GND | Address 0x76; not an ESP32 GPIO |
| I²C SDA (bench LCD) | GPIO2 / D2 | D-006; unplug to flash |
| I²C SCL (bench LCD) | GPIO4 / D4 | Power LCD VCC from 3V3, never 5 V |
| AM2302 data | GPIO5 | Operator, 2026-08-14. Strapping pin; keep idle-high with pull-up to 3V3 |
| SDS011 TX | GPIO16 / RX2 | Sensor TX into ESP32 RX |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX into sensor RX |
| MQ135 analog | GPIO34 / ADC1 | Input-only; divider required; measure before connection |

Datasheet support for these electrical choices is recorded in [spec-comparison.md](spec-comparison.md).
Photos still block construction.

## Non-negotiable limits

- Never apply 5 V to an ESP32 GPIO or `3V3` pin. GPIO VIH max is VDD + 0.3 V. Station 5 V may enter only a confirmed `USB` or `VIN`/`5V` pad.
- Do not trust generic diagrams for connector order.
- Do not power the full assembly before RLS-01 approves an evidence-backed wiring table.
- Do not put mains on a breadboard. Enclose AC/DC primaries before energising.
- The earlier AI-generated pictorial diagram from the planning conversation is rejected.
- A later deterministic concept diagram is still only conceptual until actual modules are verified.
- MQ135 is not a CO₂ sensor.

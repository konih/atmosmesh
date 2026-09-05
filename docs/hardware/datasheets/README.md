# Datasheets

Manufacturer PDFs retrieved on **2026-08-14** for the AtmosMesh planned bill of materials.
Use these, not vendor-blog summaries, when comparing voltages, logic levels, and current.

Chip-level sheets describe the **silicon**. Breakout modules (GY-BMP280, 4-pin OLED, MQ135
board) add regulators, pull-ups, and pin order that these files cannot confirm. RLS-01 photos
still win for connector orientation.

Start with [spec-comparison.md](../spec-comparison.md).

## Files

| File | Part | Issuer | Document |
| --- | --- | --- | --- |
| [espressif-esp32-wroom-32.pdf](espressif-esp32-wroom-32.pdf) | ESP32-WROOM-32 module (ESP32-D0WDQ6, 4 MB flash) | Espressif | Datasheet v3.7, NRND |
| [espressif-esp32-soc.pdf](espressif-esp32-soc.pdf) | ESP32 series SoC | Espressif | Series datasheet v5.3 |
| [silabs-cp2102.pdf](silabs-cp2102.pdf) | CP2102/9 USB-UART | Silicon Labs | CP2102/9 data sheet |
| [nova-sds011.pdf](nova-sds011.pdf) | SDS011 laser PM sensor | Nova Fitness | Specification V1.3 |
| [aosong-am2302-dht22.pdf](aosong-am2302-dht22.pdf) | DHT22 / AM2302 | Aosong (SparkFun copy) | DHT22 product sheet |
| [aosong-am2302.pdf](aosong-am2302.pdf) | AM2302 wired module | Aosong (Adafruit copy) | AM2302 product sheet |
| [bosch-bmp280.pdf](bosch-bmp280.pdf) | BMP280 pressure sensor | Bosch Sensortec | BST-BMP280-DS001-26 |
| [winsen-mq135.pdf](winsen-mq135.pdf) | MQ135 semiconductor gas sensor | Winsen | Manual v1.4 |
| [solomon-ssd1306.pdf](solomon-ssd1306.pdf) | SSD1306 OLED controller | Solomon Systech | Candidate for the 4-pin OLED |
| [sino-wealth-sh1106.pdf](sino-wealth-sh1106.pdf) | SH1106 OLED controller | Sino Wealth | Candidate for the 4-pin OLED |
| [hi-link-hlk-ld2410s-user-manual.pdf](hi-link-hlk-ld2410s-user-manual.pdf) | HLK-LD2410S 24 GHz low-power presence radar | Shenzhen Hi-Link | User manual V1.3 (retrieved 2026-09-04) |
| [hi-link-hlk-ld2410s-serial-protocol.pdf](hi-link-hlk-ld2410s-serial-protocol.pdf) | HLK-LD2410S | Shenzhen Hi-Link | Serial communication protocol V1.00, 2024-08-23 (retrieved 2026-09-04) |
| [hi-link-hlk-ld2410s-operation-guide.pdf](hi-link-hlk-ld2410s-operation-guide.pdf) | HLK-LD2410S | Shenzhen Hi-Link | Operation Guide 1.0: wiring to a CH340 board, default report, health check by command, tool FAQ (retrieved 2026-09-05) |

### HLK-LD2410S — what the manual settles (read 2026-09-04)

Table 2-1 and Table 3-2 of the user manual, confirmed identical in section 1 of the protocol
document:

| Fact | Value | Where |
| --- | --- | --- |
| Supply | 3.0–3.6 V, typ. 3.3 V, on J2 pin 1 `3V3` | Table 2-1, Table 3-2 |
| Current | average 0.04 / 0.12 / 0.6 mA (min / typ / max, "office scene"); the tool screenshots on page 4 show peaks near 118 mA | Table 2-1, section 2 |
| J2 pin order, as printed on the silkscreen | 1 `3V3`, 2 `GND`, 3 `OT1` = UART_TX, 4 `RX` = UART_RX, 5 `OT2` = "IO which is used to report the detection status: the high level is manned, and the low level is unmanned" | Table 3-2 |
| Logic level of `OT1`, `RX`, `OT2` | 0–3.3 V | Table 3-2 |
| J1 | SWD (GND, DIO, CLK, 3V3), not for the carrier | Table 3-1 |
| UART | 115200 baud, 1 stop bit, no parity, little-endian | Protocol section 2 |
| Minimal report frame (default) | `6E` · state (0/1 nobody, 2/3 somebody) · distance 2 bytes cm · `62` | Protocol Table 2-1 |
| Standard frame | head `F4 F3 F2 F1`, length, type `0x01`, state, distance, reserved, 64 energy bytes, tail `F8 F7 F6 F5` | Protocol Table 2-1 |
| Range | 10 m moving, 4 m stationary (wall-mounted at 1.5 m); refresh cycle 1–60 s configurable | Table 2-1 |
| Size, temperature | 20 × 20 mm, −40 to 85 °C; 10 dBm EIRP; 24–24.25 GHz | Table 2-1 |

**Consequence:** the module is a 3.3 V part end to end. It may be fed from a 3.3 V rail and its
`OT2` and `OT1` may drive a 3.3 V GPIO directly; nothing on it can put more than 3.3 V on a pin.
The presence pin is `OT2`, not `OT1` — `OT1` is the UART transmit line. Header pitch is not stated;
the photographs show a standard five-pin header, to be measured on the part.

### HLK-LD2410S — what the Operation Guide adds (read 2026-09-05)

- **Reports continuously by default**, not on change: the guide's serial-tool capture shows
  `6E 02 23 00 62` repeating at the report rate, with `6E 00 00 00 62` once the target leaves.
- **Health check:** send enable-configuration `FD FC FB FA 04 00 FF 00 01 00 04 03 02 01`;
  a working module replies and stops reporting for its 3 s configuration window. No reply on a
  correctly wired module is a hardware fault, not a mode.
- **No low-power switch, no jumper setting.** Every parameter (distances, report rates, response
  speed, no-one delay, thresholds) is written over UART. Nothing is selected by a pad.
- **The pads on the module** (wiring photo, page 1): three small footprints in a column between
  the chip and the header at the `RX`/`OT1` height, under the `HLK-LD2410S` silkscreen. On
  Hi-Link's reference unit the top and bottom carry a resistor and the middle one is empty, and
  that unit works — so those are the series links of the UART lines (and one option left open),
  and a missing or open link in the `OT1` position leaves the header pin floating. That is the
  first thing to meter on a module whose `OT2` works and whose `OT1` is silent (SP-01, first
  unit, 5 September 2026).

## Provenance

| File | Retrieved from | SHA-256 |
| --- | --- | --- |
| espressif-esp32-wroom-32.pdf | https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf | `a88f0a4376106498732580d8371009b4e6260358db2e9f3ab2deb0ee3e4fa5b6` |
| espressif-esp32-soc.pdf | https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf | `a7917e6b47528c9dcab06837a49d452e582751335797db879f1cf2d17cd29adf` |
| silabs-cp2102.pdf | https://www.silabs.com/documents/public/data-sheets/CP2102-9.pdf | `f025d9c738e4906544bbae493d5ff4a8d9746df247c92a329f4ed94799220e59` |
| nova-sds011.pdf | https://www-sd-nf.oss-cn-beijing.aliyuncs.com/官网下载/SDS011 laser PM2.5 sensor specification-V1.4.pdf | `4bf53f81b5eb5978c12e923d3e44f71782367c193c8d1033c6c4b007cd73bac7` |
| aosong-am2302-dht22.pdf | https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf | `c91441a385cdf58f5f0ae50b194f294d5d1f0faeaac9a010efacb7ecf21be448` |
| aosong-am2302.pdf | https://cdn-shop.adafruit.com/datasheets/Digital+humidity+and+temperature+sensor+AM2302.pdf | `e2827d9b41ae5e3d578580bc4f9c13865169a2ea6336f5a85d8d13d0279b2b43` |
| bosch-bmp280.pdf | https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf | `473ff27d9df698b4757e36b36209f83b9f637b592c999d5fabe2a9453a488da6` |
| winsen-mq135.pdf | https://www.winsen-sensor.com/d/files/PDF/Semiconductor Gas Sensor/MQ135 (Ver1.4) - Manual.pdf | `a881e0d8211f87b40d271f44d73d1dd9d2f8e7c6d516bc7ee92265f1d12a19c1` |
| solomon-ssd1306.pdf | https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf | `d55f875357de96d8c0e92153a389acc57e8bab4db7a0687f2e0bd3362f0036f6` |
| sino-wealth-sh1106.pdf | https://www.displayfuture.com/Display/datasheet/controller/SH1106.pdf | `d872cf078afa1b3df7412bbd6f7aac3aa3db3236f747096d898df28c0b8af2cc` |
| hi-link-hlk-ld2410s-user-manual.pdf | Hi-Link's official Drive folder for the LD2410S (linked from https://www.hlktech.net/index.php?id=1176), file id `1eX8C0SujX_pARyRFToD1Ohh2PwLNKFlS`, fetched via https://drive.usercontent.google.com/download?id=1eX8C0SujX_pARyRFToD1Ohh2PwLNKFlS&export=download. The tinytronics mirror was behind a bot check | `6c4c2fd31c9c53fbc2d8d2552614cd997a559079ab38896080780d50603369a6` |
| hi-link-hlk-ld2410s-serial-protocol.pdf | Same folder, file id `1LFyf6w9nOxW7b5z0rg5I3mPkk2KjQviE`, fetched via https://drive.usercontent.google.com/download?id=1LFyf6w9nOxW7b5z0rg5I3mPkk2KjQviE&export=download | `8d120eccdb3b0a03ca3eaaa92c95d4f01623ee429d038ea1eeb2880908238e98` |
| hi-link-hlk-ld2410s-operation-guide.pdf | Same folder ("HLK-LD2410S Operation Guide1.0 .pdf"), downloaded by the operator on 2026-09-05 | `1839747e9098ed91d1f12f5aa77db2daa5fba598561d96ca9b75a3ad5275159a` |

The SDS011 URL is labelled V1.4; the PDF itself is **Version V1.3** (2015-10-09). Treat it as V1.3.

Adafruit’s file named `DHT22.pdf` is the **AM2303** sheet (DS18B20 temperature element, −40…125 °C). It was discarded. Do not use it for this project.

## Not included

- ESP32 hardware design guidelines PDF — Espressif’s former direct PDF URL now returns HTML.
- Exact ESP32 **devboard** schematic — USB probe identified a CP2102 + ESP32-D0WDQ6, not a named DevKit revision.
- GY-BMP280 / OLED / MQ135 **breakout** schematics — unknown until photos.
- 5 V bench supply — not selected.

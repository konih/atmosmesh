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

The SDS011 URL is labelled V1.4; the PDF itself is **Version V1.3** (2015-10-09). Treat it as V1.3.

Adafruit’s file named `DHT22.pdf` is the **AM2303** sheet (DS18B20 temperature element, −40…125 °C). It was discarded. Do not use it for this project.

## Not included

- ESP32 hardware design guidelines PDF — Espressif’s former direct PDF URL now returns HTML.
- Exact ESP32 **devboard** schematic — USB probe identified a CP2102 + ESP32-D0WDQ6, not a named DevKit revision.
- GY-BMP280 / OLED / MQ135 **breakout** schematics — unknown until photos.
- 5 V bench supply — not selected.

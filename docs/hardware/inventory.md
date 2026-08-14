# Known hardware and safety status

This file contains only project-relevant hardware. Exact module variants remain unverified until
RLS-01 records front/back photos and markings.

| Component | Intended role | Supply | Interface | Verification status |
| --- | --- | --- | --- | --- |
| ESP-WROOM-32 devboard | Controller, Wi-Fi, MQTT | USB / confirmed board input | 3.3-V GPIO | Exact board pending |
| SDS011 | PM2.5 and PM10 | 5 V | UART, 3.3-V logic expected | Connector/pins pending |
| DHT22 / AM2302 | Temperature and humidity | 3.3 V planned | Single-wire data | Pin order pending |
| GY-BMP280 | Pressure and temperature | 3.3 V planned | I²C | Breakout details pending |
| Mini OLED, 4 pins | Local status | 3.3 V planned | I²C expected | Controller/pin order pending |
| MQ135 module | Experimental gas trend | 5 V heater/module | Analog through divider | Output range pending |
| Regulated 5-V supply | SDS011 and MQ135 power | Input unknown | 5-V output | Candidate pending |

## Provisional signal map — not approved for construction

| Signal | ESP32 pin | Notes |
| --- | --- | --- |
| I²C SDA | GPIO21 | Shared by OLED and BMP280 |
| I²C SCL | GPIO22 | Shared by OLED and BMP280 |
| DHT22 data | GPIO27 | External pull-up to 3.3 V if module lacks one |
| SDS011 TX | GPIO16 / RX2 | Sensor TX into ESP32 RX |
| SDS011 RX | GPIO17 / TX2 | ESP32 TX into sensor RX |
| MQ135 analog | GPIO34 / ADC1 | Input-only; divider required; measure before connection |

## Non-negotiable limits

- Never apply 5 V to an ESP32 GPIO or `3V3` pin.
- Do not trust generic diagrams for connector order.
- Do not power the full assembly before RLS-01 approves an evidence-backed wiring table.
- The earlier AI-generated pictorial diagram from the planning conversation is rejected.
- A later deterministic concept diagram is still only conceptual until actual modules are verified.

# Spec comparison from manufacturer datasheets

Values below are taken from the PDFs in [datasheets/](datasheets/README.md) on 2026-08-14.
They apply to the **named silicon or sensor**, not automatically to a cheap breakout.

USB probe of the connected board (same day): ESP32-D0WDQ6 rev 1.0, 4 MB flash, 40 MHz
crystal, Silicon Labs CP2102, MAC `ac:67:b2:37:26:78`, port `/dev/cu.usbserial-0001`.

## Electrical comparison

| Part | Supply | Signal / logic | Typical current | Absolute / hard limit | Interface |
| --- | --- | --- | --- | --- | --- |
| ESP32-WROOM-32 | 3.0–3.6 V on module `3V3` (typ. 3.3 V) | GPIO VIH max **VDD + 0.3 V** (~3.6 V at 3.3 V) | Wi-Fi TX 180–240 mA; RX ~95–100 mA; recommended external supply **≥ 500 mA** | Module VDD33 max **3.6 V** | UART, I²C, ADC1/ADC2 |
| CP2102 (on this board) | 3.0–3.6 V (on-chip 3.3 V regulator from USB) | UART I/O at VDD (~3.3 V) | USB full-speed bridge | VDD max 4.2 V abs.; I/O can tolerate 5.8 V when VDD > 3.0 V — **that does not make ESP32 GPIOs 5 V tolerant** | USB CDC serial |
| SDS011 | **4.7–5.3 V**, ripple < 20 mV, supply **> 1 W** | UART TTL **@ 3.3 V** | Rated 70 mA ± 10 mA; sleep < 4 mA | Do not feed 5 V into RX/TX | UART 9600 8N1, 1 Hz |
| DHT22 / AM2302 | 3.3–6 V DC (AM2302 sheet: 3.3–5.5 V) | Single-bus, idle high, needs pull-up to VDD | Measuring 1–1.5 mA | Collecting period **> 2 s** | 1-wire-like single bus |
| BMP280 (chip) | VDD 1.71–3.6 V; VDDIO 1.2–3.6 V | I²C/SPI, pins must not exceed **VDDIO + 0.3 V** | 2.8 µA typ. @ 1 Hz; peak 0.72–1.12 mA | Any supply pin max **4.25 V**; do not hold I²C high if VDDIO is off | I²C 0x76 (SDO=GND) or 0x77 (SDO=VDDIO) |
| MQ135 (sensor element) | Heater **5.0 V ± 0.1 V**; Vc ≤ 24 V DC | Analog across RL; Vs **2.0–4.0 V** in 400 ppm H₂ (RL = 4.7 kΩ) | Heater **≤ 950 mW** (~190 mA at 5 V) | Preheat **≥ 48 h** after storage; not CO₂ | Analog; module AO often near 5 V — **divider required** |
| SSD1306 (candidate) | Logic VDD **1.65–3.3 V**; panel VCC 7–15 V (usually onboard pump) | I²C 0x3C / 0x3D (SA0) | Module-dependent; chip logic is 3.3 V max | Do not put 5 V on chip VDD | I²C, 128×64 RAM |
| SH1106 (candidate) | Logic VDD1 **1.65–3.5 V** (abs max 3.6 V) | I²C 0x3C / 0x3D (SA0); input max **VDD1 + 0.3 V** | Charge-pump module-dependent | Same 3.3 V GPIO rule | I²C, 132×64 RAM (not drop-in identical to SSD1306) |

## What may connect to an ESP32 GPIO

Safe **only if** the signal stays ≤ ~3.3 V (VIH max = VDD + 0.3 V).

| Signal | Datasheet level | Direct to ESP32? |
| --- | --- | --- |
| SDS011 TX / RX | 3.3 V TTL (pins 7 / 6) | Yes, after pin order is confirmed from the adapter |
| DHT22 DATA at 3.3 V VDD | 3.3 V bus | Yes, with pull-up to **3.3 V** (not 5 V) |
| BMP280 SDA/SCL at 3.3 V VDDIO | 3.3 V I²C | Yes, if the breakout is actually 3.3 V |
| OLED SDA/SCL at 3.3 V | 3.3 V I²C | Yes, if the module VCC is 3.3 V |
| MQ135 AO / Vs | 2–4 V typical in the Winsen test; **up to ~5 V** on many modules | **No** — ADC divider, measure before GPIO34 |

Never join the 5 V rail to ESP32 `3V3` or any GPIO. Station 5 V may enter only `VIN`/`5V` (or USB). SDS011 UART is 3.3 V; MQ135 analog is not.

## Power budget (planning numbers)

Station target is one 5 V rail (D-005). Bench still splits USB vs 5 V sensors until the AC/DC module is enclosed and measured. Full write-up: [power.md](power.md).

| Rail | Loads | Planning current |
| --- | --- | --- |
| 5 V (station, shared) | ESP32 Wi-Fi TX 180–240 mA + OLED/BMP280/DHT22 on the LDO + SDS011 headroom ~200 mA + MQ135 heater ~190 mA | **~650 mA peak coincidence** |
| 5 V (bench, sensors only) | SDS011 + MQ135 | **~390 mA** with headroom; 700 mA is adequate *here* |
| ESP32 `3V3` pin | OLED + BMP280 + DHT22 | tens of mA — onboard LDO is enough |

The `5V07 / 12V04` candidate is typically **5 V / 700 mA / 3.5 W** *or* **12 V**. 700 mA is **not** comfortable once the ESP32 shares that rail. SDS011 ripple spec is **< 20 mV**; cheap modules often quote ~60 mV.

## Measurement ranges (do not treat as certified)

| Sensor | Quantity | Range / notes | Stated accuracy |
| --- | --- | --- | --- |
| SDS011 | PM2.5, PM10 | 0.0–999.9 µg/m³; 0.3 µm min particle | ±15% and ±10 µg/m³ at 25 °C, 50 % RH |
| DHT22 | RH, T | 0–100 % RH; −40…80 °C | ±2 % RH (max ±5 %); ±0.5 °C |
| BMP280 | P, T | 300–1100 hPa; −40…85 °C (full accuracy 0…65 °C) | Rel. P ±0.12 hPa (700–900 hPa, 25–40 °C); T ±0.5 °C @ 25 °C |
| MQ135 | NH₃, sulfides, benzene-series, smoke | 10–1000 ppm of those gases in the Winsen sheet | **Relative trend only.** Not CO₂, not ppm on our MQTT contract |

SDS011 service life is given as **8000 h** of laser-on time. Duty-cycling (e.g. 30 s per minute) is the datasheet’s own suggestion for monitors.

## I²C expectations (unverified on the physical modules)

| Device | 7-bit address | Notes |
| --- | --- | --- |
| BMP280 | 0x76 (SDO=GND) or 0x77 (SDO=3V3) | CSB must be high for I²C; SDO must not float |
| SSD1306 | 0x3C (`0111100`) or 0x3D (`0111101`) | SA0 / D/C# pin |
| SH1106 | same 0x3C / 0x3D | 132-column RAM; init sequence differs from SSD1306 |

A scan that shows 0x3C plus 0x76/0x77 is the expected bench result once wiring is approved.

## Implications for the provisional pin map

The map in [inventory.md](inventory.md) is still **not approved**, but the sheets support these electrical choices:

- GPIO21/19 for GY-BMP280 I²C (operator bench; GPIO21/22 are also valid defaults); 3.3 V only.
- GPIO16/17 (U2RXD/U2TXD) for SDS011: matches the WROOM-32 pin mux; keep SDS011 on **5 V power**, UART on **3.3 V**.
- GPIO34 (ADC1_CH6, input-only) for MQ135: correct domain while Wi-Fi is on (avoid ADC2). Espressif notes ADC accuracy degrades above ~2.45 V at atten=3, so the divider should target **well below 2.5 V**, not merely below 3.3 V.
- GPIO18 for AM2302 data (operator bench; GPIO27 is also a valid GPIO). Pull-up to 3.3 V. Sample slower than 0.5 Hz.

## Still blocked without photos / measurement

- ESP32 **devboard** silkscreen (VIN vs 5V vs USB-only).
- SDS011 connector vs USB2TT004 adapter pin order (datasheet pin 1 = NC, 3 = 5 V, 6 = RX, 7 = TX).
- DHT22 vs wired AM2302 pin/colour order.
- GY-BMP280 6-pin: CSB→3V3 and SDO→GND for I²C 0x76. Breakout regulator / 5 V VCC still unconfirmed.
- OLED: SSD1306 at 0x3C on GPIO5/GPIO4 proven on the bench; SH1106 still possible on a second module. VCC 3.3 V.
- MQ135 module AO unloaded voltage (must measure; Winsen Vs is not the module AO).
- 5 V supply rating and ripple.

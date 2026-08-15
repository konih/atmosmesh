<p align="center">
  <img src="docs/assets/atmosmesh-mark.svg" alt="AtmosMesh mark" width="128" height="128">
</p>

<h1 align="center">AtmosMesh</h1>

<p align="center"><em>Local air sensing, connected as a mesh.</em></p>

<p align="center">
  <a href="https://github.com/konih/atmosmesh/actions/workflows/firmware.yml"><img src="https://github.com/konih/atmosmesh/actions/workflows/firmware.yml/badge.svg" alt="Firmware CI"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/konih/atmosmesh" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/ESP32-WROOM--32-000000?logo=espressif&logoColor=white" alt="ESP32">
  <img src="https://img.shields.io/badge/PlatformIO-Arduino-orange" alt="PlatformIO">
  <img src="https://img.shields.io/badge/MQTT-planned-41BDF5?logo=homeassistant&logoColor=white" alt="MQTT planned">
  <img src="https://img.shields.io/badge/status-bench%20bring--up-yellow" alt="Status: bench bring-up">
</p>

An ESP32 indoor air-quality station: particulate matter and room climate on the
bench today, a small local OLED, and a path to MQTT, Home Assistant, Prometheus,
and Grafana on Kubernetes.

This is a **working bench**, not a finished product. Wiring is not approved for
unattended or mains-powered use until RLS-01 photos and a measured 5 V rail are
on record. It is **not** a certified health, fire, gas, or life-safety instrument.

## What it measures

| Sensor | Quantity | Notes |
| --- | --- | --- |
| SDS011 | PM2.5, PM10 | UART2 only (GPIO16/17). 5 V heater, 3.3 V UART |
| AM2302 / DHT22 | Temperature, humidity | GPIO18, 3V3 |
| BMP280 | Pressure, temperature | I²C 0x76 on GPIO21/19, 3V3 |
| Mini OLED (D-001) | Local live page | I²C 0x3C, SDA GPIO5, SCL GPIO4, **3V3 only** |
| MQ135 | Relative gas trend | Analog GPIO34 through a divider. **Never labelled CO₂ or ppm** |
| VEML7700 | Illuminance | Optional, I²C 0x10 on the BMP280 bus. Not fitted yet |
| PIR + beeper | Occupancy cue | GPIO33 / GPIO25 |

```text
SDS011 ─┐
AM2302 ─┤
BMP280 ─┼─> ESP32 ── OLED (local)
MQ135  ─┤       └─ MQTT (planned) ──> Mosquitto ──> Home Assistant
VEML   ─┘                         └─ metrics ─> Prometheus ─> Grafana
```

## Safety (read this first)

- ESP32 GPIOs and the `3V3` pin are **not 5 V tolerant**.
- SDS011 and MQ135 take **5 V power**; their *signals* stay at 3.3 V.
- SDS011 is **UART2 only**. Never GPIO1/TX0 or GPIO3/RX0 — those are the USB-UART
  used by `task flash` / `task monitor`.
- MQ135 analog goes through a measured divider before GPIO34. The live breadboard
  is 10 kΩ series + 20 kΩ to GND.
- Open mains PCBs stay off the breadboard and must be enclosed before they are
  energised. Mains switching is out of scope.
- Confirm every module's markings before you copy a pin table from this repo.

Full rules: [`AGENTS.md`](AGENTS.md) and [`docs/hardware/power.md`](docs/hardware/power.md).

## Quick start

Host tests need PlatformIO and Python 3.11 or 3.12 (not Homebrew 3.14):

```bash
task bootstrap-agent-python   # creates .venv with pyserial + esptool
task test                     # native Unity tests (no hardware)
task build                    # ESP32 image
task flash                    # ESP_PORT defaults to /dev/cu.usbserial-0001
task monitor
```

Override the serial port with `ESP_PORT=/dev/…`. Do not run `python -m esptool`
with system Python. Direct wrapper: `./scripts/esp-tool version`.

Firmware details, OLED constructor, and the live pin map:
[`firmware/README.md`](firmware/README.md).

## Hardware

| Path | What |
| --- | --- |
| [`docs/hardware/inventory.md`](docs/hardware/inventory.md) | Confirmed parts and open questions |
| [`docs/hardware/power.md`](docs/hardware/power.md) | 5 V / 3.3 V domains |
| [`hardware/kicad/`](hardware/kicad/README.md) | Bench carrier (KiCad 10): OLED, sensors, extras strip, fully routed. **Do not fabricate** until DevKit pin order is photographed |

The carrier board uses a **10 k / 15 k** MQ135 divider (3.0 V max on GPIO34).
Firmware still matches the **breadboard 10 k / 20 k** until that board replaces
the breadboard.

## Repository layout

| Path | Purpose |
| --- | --- |
| `firmware/` | PlatformIO + Arduino firmware and native tests |
| `hardware/kicad/` | Bench-station carrier schematic and PCB |
| `docs/` | Architecture, hardware facts, datasheets |
| `deploy/` | Kubernetes / home-automation config (not packaged yet) |
| `tests/` | Host-side checks |
| `agent-context/` | Stories, decisions, and live session notes |
| `AGENTS.md` | Safety boundaries and workflow for humans and agents |

## Status

Bench firmware on `main` drives the OLED (U8g2 SSD1306 ALT0), BMP280, AM2302,
SDS011 UART2, MQ135 ADC, PIR, and beeper. MQTT, Home Assistant, and the cluster
path are still ahead ([roadmap](agent-context/roadmap.md)).

## License and contributors

[MIT](LICENSE). People: [CONTRIBUTORS.md](CONTRIBUTORS.md).

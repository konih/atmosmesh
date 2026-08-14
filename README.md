# AtmosMesh

An ESP32-based indoor air-quality station built from the hardware already on hand. The station
will measure particulate matter and room climate, show useful local status on a small OLED, and
publish measurements to a Kubernetes-hosted home-automation platform.

> **Local air sensing, connected as a mesh.**

## Target outcome

- Measure PM2.5 and PM10 with an SDS011.
- Measure temperature and humidity with a DHT22.
- Measure pressure and a second temperature value with a BMP280.
- Display current values and health locally on a small I²C OLED.
- Publish stable, unit-bearing measurements and availability over MQTT.
- Visualize current state in Home Assistant and history in Grafana.
- Operate safely and recover from sensor, Wi-Fi, broker, and power interruptions.

The MQ135 may be evaluated as a **relative gas/air-quality trend only**. It is not an NDIR CO₂
sensor and its value must never be labelled as CO₂ or ppm.

## Current status

Firmware lives in `firmware/` (PlatformIO, D-006). Host tests: `task test`. Bench LCD
bring-up writes dummy text on the I²C 1602 wired to D5/D4. RLS-01 photos and 5 V PSU measurement
are still required before SDS011/MQ135/mains.

Start at [agent-context/README.md](agent-context/README.md) and [firmware/README.md](firmware/README.md).

## Planned system

```text
SDS011 ─┐
DHT22  ─┤
BMP280 ─┼─> ESP32 ── MQTT ──> Mosquitto ──> Home Assistant
MQ135  ─┤       │                         └─> metrics bridge ─> Prometheus ─> Grafana
OLED   <┘       └─ local display and health state
```

The firmware toolchain is PlatformIO + Arduino (`esp32dev`). Kubernetes packaging is still open
(OQ-002).

## Repository layout

| Path | Purpose |
| --- | --- |
| `AGENTS.md` | Stable instructions and safety boundaries for coding agents |
| `agent-context/README.md` | Live status, active story, blockers, and next actions |
| `agent-context/roadmap.md` | Ordered delivery plan and story status |
| `agent-context/stories/` | One implementation contract per story |
| `agent-context/decisions.md` | Accepted decisions and open technical questions |
| `docs/architecture.md` | Intended system boundaries and data flow |
| `docs/hardware/` | Verified hardware facts, power architecture, datasheets, spec comparison, later approved wiring |
| `firmware/` | ESP32 firmware after the framework decision |
| `deploy/` | Kubernetes and home-automation configuration |
| `tests/` | Host-side, configuration, and end-to-end validation |

## Firmware

See [firmware/README.md](firmware/README.md). From the repository root:

```bash
task test
task build
task flash          # ESP_PORT defaults to /dev/cu.usbserial-0001
task monitor
task run            # flash then monitor
./scripts/esp-tool version   # not: python -m esptool
```

LCD SDA is GPIO5 (not GPIO2). GPIO5 wants idle-high at boot; the I²C pull-up is usually compatible.

## Working agreement

1. Read `AGENTS.md`, then the live dashboard and active story.
2. Claim only one story at a time in `agent-context/roadmap.md`.
3. Treat story acceptance criteria as the contract.
4. Validate before connecting power or applying cluster changes.
5. Record evidence and a dated handoff in `agent-context/handoffs/`.

## Safety

- ESP32 GPIOs are 3.3-V signals and are **not 5-V tolerant**.
- Station 5 V may enter the DevBoard only through a confirmed `USB` or `VIN`/`5V` input, never `3V3`.
- SDS011 and MQ135 take 5 V power; UART/ADC signals stay at 3.3 V (divider on MQ135 AO).
- All grounds must be common when signals cross supplies; do not tie 5 V to 3.3 V.
- Open mains PCBs stay off the breadboard and must be enclosed before they are energised.
- Mains *switching* and relays are outside this project (D-004).
- The station is experimental and is not a certified health, fire, gas, or life-safety instrument.

## Local planning source

The project originated in the Obsidian vault at
`/Users/A242168/Projects/Obsidian/Notes/Personal/AtmosMesh.md`. This repository is now
the implementation source of truth; the vault remains the personal overview.

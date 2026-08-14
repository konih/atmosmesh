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

The repository is scaffolded and **RLS-01 — Hardware identification and wiring approval** is the
first active story. No firmware or deployment implementation is approved until the actual module
variants, pin labels, power requirements, and logic levels have been verified from photos and,
where needed, measurements.

Start at [agent-context/README.md](agent-context/README.md).

## Planned system

```text
SDS011 ─┐
DHT22  ─┤
BMP280 ─┼─> ESP32 ── MQTT ──> Mosquitto ──> Home Assistant
MQ135  ─┤       │                         └─> metrics bridge ─> Prometheus ─> Grafana
OLED   <┘       └─ local display and health state
```

The final firmware framework and Kubernetes packaging approach are intentionally undecided. They
will be recorded in `agent-context/decisions.md` once the hardware and cluster constraints are
known.

## Repository layout

| Path | Purpose |
| --- | --- |
| `AGENTS.md` | Stable instructions and safety boundaries for coding agents |
| `agent-context/README.md` | Live status, active story, blockers, and next actions |
| `agent-context/roadmap.md` | Ordered delivery plan and story status |
| `agent-context/stories/` | One implementation contract per story |
| `agent-context/decisions.md` | Accepted decisions and open technical questions |
| `docs/architecture.md` | Intended system boundaries and data flow |
| `docs/hardware/` | Verified hardware facts and, later, approved wiring |
| `firmware/` | ESP32 firmware after the framework decision |
| `deploy/` | Kubernetes and home-automation configuration |
| `tests/` | Host-side, configuration, and end-to-end validation |

## Working agreement

1. Read `AGENTS.md`, then the live dashboard and active story.
2. Claim only one story at a time in `agent-context/roadmap.md`.
3. Treat story acceptance criteria as the contract.
4. Validate before connecting power or applying cluster changes.
5. Record evidence and a dated handoff in `agent-context/handoffs/`.

## Safety

- ESP32 GPIOs are 3.3-V signals and are **not 5-V tolerant**.
- External 5 V may power SDS011 and MQ135 only after their exact pins are confirmed.
- All grounds must be common when signals cross supplies; positive supply rails must not be tied
  together casually.
- Mains voltage and mains relay work are outside this project.
- The station is experimental and is not a certified health, fire, gas, or life-safety instrument.

## Local planning source

The project originated in the Obsidian vault at
`/Users/A242168/Projects/Obsidian/Notes/Personal/AtmosMesh.md`. This repository is now
the implementation source of truth; the vault remains the personal overview.

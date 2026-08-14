# Agent context dashboard

Committed live context for humans and coding agents. Stable constraints live in `../AGENTS.md`;
this dashboard records what changes between sessions.

**Last updated:** 2026-08-14

## Active work

| Story | Status | Objective | Next action |
| --- | --- | --- | --- |
| [RLS-01](stories/RLS-01.md) | Ready | Identify exact hardware and approve a safe wiring table | Photos + enclosed 5 V measurement |
LCD VCC must be **3.3 V**. Unplug D2 while flashing (GPIO2 blocks download mode).

## Current blockers

- Exact ESP32 **devboard** silkscreen (`VIN`/`5V` vs `3V3`) is not confirmed (chip is ESP32-D0WDQ6).
- OLED controller, resolution, I²C address, and pin order are not confirmed.
- BMP280 breakout is 6-pin (VCC GND SCL SDA CSB SDO); onboard regulator / 5 V VCC still unconfirmed.
- Candidate 5 V AC/DC `5V07 / 12V04` is unverified (5 V/700 mA vs 12 V) and still an open mains PCB.
- Shared-rail peak current (~650 mA) vs 700 mA module rating is unresolved.
- MQ135 module output range has not been measured.
- Firmware framework: PlatformIO (D-006). Cluster packaging still open (OQ-002).

## Next operator inputs

1. Photograph the front and back of the ESP32 (including `VIN`/`5V`/`3V3` labels), both mini-OLEDs,
   BMP280, DHT22, SDS011 plus adapter/cable, MQ135, and the `5V07 / 12V04` AC/DC (AC pads vs DC pads).
2. Enclose the AC/DC primary **before** applying 230 V. Measure DC out with no ESP32 attached.
3. Record whether a dummy-load measurement is possible and the observed voltage/ripple.

Do not connect the proposed complete circuit before these checks. Do not put mains on a breadboard.

## Key context

| Topic | File |
| --- | --- |
| Delivery order | [roadmap.md](roadmap.md) |
| Decisions and open questions | [decisions.md](decisions.md) |
| Story contracts | [stories/](stories/) |
| Hardware facts | [../docs/hardware/inventory.md](../docs/hardware/inventory.md) |
| Power architecture | [../docs/hardware/power.md](../docs/hardware/power.md) |
| Datasheets / spec comparison | [../docs/hardware/datasheets/](../docs/hardware/datasheets/), [../docs/hardware/spec-comparison.md](../docs/hardware/spec-comparison.md) |
| System architecture | [../docs/architecture.md](../docs/architecture.md) |
| Firmware | [../firmware/README.md](../firmware/README.md) |

## Workflow

1. Read `../AGENTS.md`, this dashboard, and the active story.
2. Claim a story only when all dependencies are done.
3. Update the story with evidence as criteria are verified.
4. Update the roadmap and dashboard together when status changes.
5. Add a dated handoff when stopping mid-story.

## Status legend

- `Ready`: dependencies satisfied; safe to claim.
- `Blocked`: waiting on another story, evidence, or operator input.
- `In flight`: actively being worked; owner/session recorded.
- `Done`: all acceptance criteria have evidence.
- `Optional`: outside the MVP unless explicitly promoted.

# Agent context dashboard

Committed live context for humans and coding agents. Stable constraints live in `../AGENTS.md`;
this dashboard records what changes between sessions.

**Last updated:** 2026-08-24

## Active work

| Story | Status | Objective | Next action |
| --- | --- | --- | --- |
| [RLS-01](stories/RLS-01.md) | Ready | Identify exact hardware and approve a safe wiring table | Photos + enclosed 5 V measurement |
| [V15-03](stories/V15-03.md) | Blocked | Finish Grove OLED/BMP180/DHT11 validation | Fix DHT; confirm OLED pixels |
OLED VCC must be **3.3 V**. OLED SDA is GPIO5, SCL GPIO4 (0x3C proven). GPIO5 idle-high is usually OK for flash.

## Current blockers

- Exact ESP32 **devboard** silkscreen (`VIN`/`5V` vs `3V3`) is not confirmed (chip is ESP32-D0WDQ6).
- OLED: SSD1306 at **0x3C** on SDA=GPIO5 SCL=GPIO4 (serial-proven). Default firmware is U8g2 **SSD1306 ALT0** (sequential COM). SH1106 remains a compile fallback.
- BMP280 breakout is 6-pin (VCC GND SCL SDA CSB SDO); onboard regulator / 5 V VCC still unconfirmed.
- Candidate 5 V AC/DC `5V07 / 12V04` is unverified (5 V/700 mA vs 12 V) and still an open mains PCB.
- Shared-rail peak current (~650 mA) vs 700 mA module rating is unresolved.
- MQ135 module output range has not been measured.
- Firmware framework: PlatformIO (D-006). Cluster packaging still open (OQ-002).
- Grove DHT11 returns unavailable on every observed cycle; DATA joint/pin, rail orientation and
  pull-up/module resistor need physical verification.
- Grove OLED initialization passes at 0x3C/128×32, but visible pixels are not yet confirmed.

## Next operator inputs

1. Photograph the front and back of the ESP32 (including `VIN`/`5V`/`3V3` labels), both mini-OLEDs,
   BMP280, DHT22, SDS011 plus adapter/cable, MQ135, and the `5V07 / 12V04` AC/DC (AC pads vs DC pads).
2. Enclose the AC/DC primary **before** applying 230 V. Measure DC out with no ESP32 attached.
3. Record whether a dummy-load measurement is possible and the observed voltage/ripple.
4. For Grove, inspect DHT11 DATA=D5/GPIO14, 3.3 V/GND and 4.7–10 kΩ pull-up/module resistor;
   visually confirm whether the OLED shows the four-line page.

Do not connect the proposed complete circuit before these checks. Do not put mains on a breadboard.

The separate **AtmosMesh Grove v1.5** board is an ESP8266EX with 4 MB flash. After authorization
and independent review, the coordinator replaced its AT firmware with AtmosMesh Grove on
2026-08-24. A second reviewed flash confirmed the explicit stable product ID at runtime. BMP180
passed runtime sampling; OLED initialized at 0x3C but pixels need visual proof; DHT11 returned
unavailable on every observed cycle. Details and next checks are in V15-03.

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
| Architecture decisions | [../docs/adr/](../docs/adr/) |
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

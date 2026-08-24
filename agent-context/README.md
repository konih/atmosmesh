# Agent context dashboard

Committed live context for humans and coding agents. Stable constraints live in `../AGENTS.md`;
this dashboard records what changes between sessions.

**Last updated:** 2026-08-24

## Active work

| Story | Status | Objective | Next action |
| --- | --- | --- | --- |
| [RLS-01](stories/RLS-01.md) | Ready | Identify exact hardware and approve a safe wiring table | Photos + enclosed 5 V measurement |
| [V15-03](stories/V15-03.md) | Blocked | Finish Grove OLED/BMP180/DHT11 validation | Confirm OLED pixels |
| [V15-04](stories/V15-04.md) | Blocked | Validate bounded uncalibrated D7 RC light response | Bright/dark/saturation/timeout evidence |
| [V15-05](stories/V15-05.md) | Blocked | Validate Grove MQTT and HA discovery | Broker/HA/reconnect evidence |
| [V15-06](stories/V15-06.md) | Blocked | Add bi-color status and duty-cycled raw soil ADC | LED and switched-rail electrical validation |
| [V15-07](stories/V15-07.md) | Blocked | Replace cryptic Grove OLED rows with labelled measurements | Operator visual confirmation |
| [V15-08](stories/V15-08.md) | Blocked | Add reversible Grove OLED/LED visual diagnostic | Independent review, then operator inspection |
| [V15-09](stories/V15-09.md) | Blocked | Add calibration-safe Grove soil/system LED status | Visual confirmation and controlled dry/wet thresholds |
OLED VCC must be **3.3 V**. OLED SDA is GPIO5, SCL GPIO4 (0x3C proven). GPIO5 idle-high is usually OK for flash.

## Current blockers

- Exact ESP32 **devboard** silkscreen (`VIN`/`5V` vs `3V3`) is not confirmed (chip is ESP32-D0WDQ6).
- OLED: SSD1306 at **0x3C** on SDA=GPIO5 SCL=GPIO4 (serial-proven). Default firmware is U8g2 **SSD1306 ALT0** (sequential COM). SH1106 remains a compile fallback.
- BMP280 breakout is 6-pin (VCC GND SCL SDA CSB SDO); onboard regulator / 5 V VCC still unconfirmed.
- Candidate 5 V AC/DC `5V07 / 12V04` is unverified (5 V/700 mA vs 12 V) and still an open mains PCB.
- Shared-rail peak current (~650 mA) vs 700 mA module rating is unresolved.
- MQ135 module output range has not been measured.
- Firmware framework: PlatformIO (D-006). Cluster packaging still open (OQ-002).
- Grove OLED initialization passes at 0x3C/128×32, but visible pixels are not yet confirmed.
- Reviewed Grove head `4e4a820` is flashed. Serial proves uncalibrated D7 values of 420–492 µs, but
  bright/dark/saturation/timeout behavior and MQTT broker/HA receipt remain unvalidated.
- Grove V15-06 serial again reports raw ADC 214 with `samples=5 power=off`. The operator has now
  confirmed the fitted D6 red / D0 green LED is common-anode; only active-LOW diagnostic output
  produced the intended colors. Soil direction/threshold calibration, broker/HA receipt,
  switched-rail voltage/current, physical power-off and OLED pixels remain unconfirmed.
- Reviewed V15-09 head `8fca62d` is now flashed. Serial proves the canonical profile drove logical
  common-anode amber (`red=LOW green=LOW`) for raw soil ADC 213 with calibration disabled. Visible
  colour for this canonical run, physical power-off and dry/wet calibration references remain
  operator-unconfirmed.

## Next operator inputs

1. Photograph the front and back of the ESP32 (including `VIN`/`5V`/`3V3` labels), both mini-OLEDs,
   BMP280, DHT22, SDS011 plus adapter/cable, MQ135, and the `5V07 / 12V04` AC/DC (AC pads vs DC pads).
2. Enclose the AC/DC primary **before** applying 230 V. Measure DC out with no ESP32 attached.
3. Record whether a dummy-load measurement is possible and the observed voltage/ripple.
4. For Grove, visually confirm the flashed V15-07 four-labelled-measurement-row layout.
5. Exercise the Grove LDR under bright/dark and saturation/timeout conditions.
6. Measure controlled dry/wet raw soil values to establish direction and thresholds; verify the YL
   switched-rail voltage/current and physical power-off, then confirm MQTT broker/HA receipt and
   reconnect behavior.

Do not connect the proposed complete circuit before these checks. Do not put mains on a breadboard.

The separate **AtmosMesh Grove v1.5** board is an ESP8266EX with 4 MB flash. After authorization
and independent review, the coordinator replaced its AT firmware with AtmosMesh Grove on
2026-08-24. A second reviewed flash confirmed the explicit stable product ID at runtime. BMP180
passed runtime sampling; OLED initialized at 0x3C but pixels need visual proof. DHT11 later produced
valid frames on D5/GPIO14 (communication pass only, not accuracy proof). Details are in V15-03.

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

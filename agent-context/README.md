# Agent context dashboard

Committed live context for humans and coding agents. Stable constraints live in `../AGENTS.md`;
this dashboard records what changes between sessions.

**Last updated:** 2026-08-14

## Active work

| Story | Status | Objective | Next action |
| --- | --- | --- | --- |
| [RLS-01](stories/RLS-01.md) | Ready | Identify exact hardware and approve a safe wiring table | Collect clear front/back photos and power-supply details |

No implementation story is in flight yet.

## Current blockers

- Exact ESP32 development-board variant and pin labelling are not confirmed.
- OLED controller, resolution, I²C address, and pin order are not confirmed.
- BMP280 breakout-board regulator and level shifting are not confirmed.
- The available regulated 5-V supply has not been selected or measured.
- MQ135 module output range has not been measured.
- Firmware framework and cluster deployment approach are intentionally undecided.

## Next operator inputs

1. Photograph the front and back of the ESP32, both mini-OLEDs, BMP280, DHT22, SDS011 plus
   adapter/cable, MQ135, and candidate 5-V power supply.
2. Include one photo showing connector labels and one showing the complete module.
3. Record the 5-V supply label and whether a multimeter is available.

Do not connect the proposed complete circuit before these checks.

## Key context

| Topic | File |
| --- | --- |
| Delivery order | [roadmap.md](roadmap.md) |
| Decisions and open questions | [decisions.md](decisions.md) |
| Story contracts | [stories/](stories/) |
| Hardware facts | [../docs/hardware/inventory.md](../docs/hardware/inventory.md) |
| System architecture | [../docs/architecture.md](../docs/architecture.md) |
| Session handoffs | [handoffs/](handoffs/) |

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

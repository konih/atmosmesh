# RLS-10 — Evaluate the 480×320 TFT

- **Status:** Optional
- **Priority:** P3
- **Milestone:** Post-MVP
- **Depends on:** RLS-08

## User story

As the builder, I want to evaluate the existing Raspberry Pi TFT so that we can decide whether a
larger local display adds enough value to justify its complexity.

## Outcome

The exact display and touch controller are identified, ESP32 feasibility is measured, and a clear
decision compares ESP32 drive against a Raspberry Pi kiosk dashboard.

## In scope

- Controller and pinout identification, voltage-level review, memory/performance experiment, and written decision.

## Out of scope

- Replacing the MVP OLED before the station is complete or forcing an unsupported Pi HAT onto ESP32.

## Acceptance criteria

- [ ] Display and touch-controller markings and pinout are confirmed from evidence.
- [ ] Voltage levels and power draw are verified before connection.
- [ ] An ESP32 test records refresh rate, memory use, Wi-Fi coexistence, and UI limitations.
- [ ] The result is compared with a Raspberry Pi Home Assistant/Grafana kiosk.
- [ ] The decision and consequences are added to `agent-context/decisions.md`.

## Validation

- Automated: build and memory report where supported.
- Manual: display photo/video and measured responsiveness/power evidence.

## Evidence

Optional and intentionally deferred until after the MVP.

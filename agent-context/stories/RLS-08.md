# RLS-08 — Harden the station for a 48-hour run

- **Status:** Blocked
- **Priority:** P0
- **Milestone:** M5 — MVP
- **Depends on:** RLS-01 through RLS-07

## User story

As the operator, I want the station to run safely for 48 hours without intervention so that the
bench prototype becomes a dependable home-automation device.

## Outcome

The mechanically secured station completes a monitored 48-hour run, recovers from controlled
failures, and has enough documentation to rebuild and operate it.

## In scope

- Safe temporary mounting, airflow, thermal inspection, watchdog/recovery behavior, and observability.
- Controlled power, sensor, Wi-Fi, and broker interruption tests.
- Build, restore, wiring, and known-limit documentation.

## Out of scope

- Custom PCB, production enclosure certification, battery operation, and mains integration.

## Acceptance criteria

- [ ] Boards, conductors, and power connections are mechanically secured and touch-safe.
- [ ] Sensors receive representative airflow and are not measurably heated by ESP32 or supply placement.
- [ ] No cable, regulator, or module shows abnormal heating during inspection.
- [ ] Watchdog or controlled recovery handles a simulated stuck/failure state.
- [ ] Restarts, availability, and last-measurement age are observable.
- [ ] Power, sensor, Wi-Fi, and broker interruption tests recover as documented.
- [ ] A 48-hour run completes without unexplained resets or data gaps.
- [ ] Rebuild and recovery documentation matches the tested station.

## Validation

- Manual: pre-run electrical/mechanical checklist, thermal checks, failure matrix, and timestamped run log.
- Automated: applicable firmware, configuration, and deployment gates remain green at the tested revision.

## Evidence

Blocked by RLS-01 through RLS-07.

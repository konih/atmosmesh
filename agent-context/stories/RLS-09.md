# RLS-09 — Add a true NDIR CO₂ sensor

- **Status:** Optional
- **Priority:** P3
- **Milestone:** Post-MVP
- **Depends on:** RLS-08 and a confirmed NDIR sensor

## User story

As a resident, I want an actual CO₂ measurement so that ventilation decisions can use ppm from a
sensor designed for that quantity.

## Outcome

A confirmed NDIR sensor such as SCD40/SCD41, SCD30, MH-Z19B, or Senseair is integrated and its CO₂
measurement remains distinct from any MQ135 trend.

## In scope

- Exact sensor identification, electrical integration, ppm semantics, plausibility testing, and dashboard update.

## Out of scope

- Deriving CO₂ from MQ135 or claiming calibration/certification not provided by the sensor.

## Acceptance criteria

- [ ] The module is confirmed from markings and authoritative documentation as an NDIR CO₂ sensor.
- [ ] Power and signal levels are verified before connection.
- [ ] The value is published with ppm units, validity, and calibration state.
- [ ] A fresh-air/plausibility test and controlled elevated-occupancy trend are documented.
- [ ] MQ135 data remains separately named or is removed.

## Validation

- Automated: payload/schema and invalid/calibration-state tests.
- Manual: fresh-air and trend evidence appropriate to the selected sensor.

## Evidence

Optional; no confirmed NDIR module is currently in inventory.

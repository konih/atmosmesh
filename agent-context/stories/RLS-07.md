# RLS-07 — Add dashboard, history, and the first alert

- **Status:** Blocked
- **Priority:** P1
- **Milestone:** M4 — Platform integration
- **Depends on:** RLS-05, RLS-06

## User story

As a resident, I want current readings, trends, and a restrained warning so that I can notice
ventilation or particulate events without reacting to single noisy samples.

## Outcome

Home Assistant shows device state and availability; Grafana shows 24-hour trends; one experimental
particulate alert uses duration or hysteresis and is demonstrated end to end.

## In scope

- Home Assistant entities for all valid measurements and device availability.
- Grafana panels for PM2.5, PM10, temperature, humidity, and pressure.
- One experimental sustained-PM warning and synthetic test input.

## Out of scope

- Medical advice, certified thresholds, automated HVAC, and MQ135-based safety alerts.

## Acceptance criteria

- [ ] Home Assistant displays current values, units, validity, and availability.
- [ ] Grafana displays at least 24 hours of the five primary measurements.
- [ ] Missing/offline data is visually distinct from numeric zero.
- [ ] The warning ignores a single outlier and uses a documented duration or hysteresis.
- [ ] A synthetic measurement is traceable through MQTT, storage, dashboard, and alert state.
- [ ] Dashboards and rules are versioned or reproducibly exported without secrets.

## Validation

- Automated: dashboard/rule/config parsing where supported.
- Manual: synthetic event, offline device, outlier, and sustained-threshold demonstrations.

## Evidence

Blocked by RLS-05 and RLS-06.

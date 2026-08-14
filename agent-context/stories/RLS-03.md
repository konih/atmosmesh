# RLS-03 — Integrate all MVP sensors on the bench

- **Status:** Blocked
- **Priority:** P0
- **Milestone:** M2 — Bench station
- **Depends on:** RLS-01, RLS-02

## User story

As the operator, I want all planned sensors sampled together so that electrical, timing, and
software conflicts are found before networking and enclosure work.

## Outcome

DHT22, SDS011, BMP280, OLED, and optionally MQ135 run together for at least 30 minutes with
plausible values, explicit invalid states, and no bus or watchdog failure.

## In scope

- DHT22 temperature/humidity, SDS011 PM2.5/PM10, BMP280 pressure/temperature.
- MQ135 raw/normalized trend only if safe ADC evidence exists.
- Independent sensor health and timestamp/age tracking.

## Out of scope

- CO₂ claims, calibrated health advice, MQTT, final display pages, enclosure.

## Acceptance criteria

- [ ] Every sensor reports a value, unit, validity state, and update age.
- [ ] PM2.5/PM10 arrive over the verified UART wiring.
- [ ] MQ135, if included, is named only as a relative gas/air-quality trend.
- [ ] Voltage measured at the connected ADC node remains within the RLS-01 approved limit.
- [ ] Disconnecting one sensor does not block or falsify the others.
- [ ] A 30-minute run has no unexplained reset, bus lock, or silent data gap.

## Validation

- Automated: sensor parsing, stale/invalid state, and unit tests where supported.
- Manual: serial log, ADC measurement, controlled disconnects, and timestamped 30-minute run log.

## Evidence

Blocked by RLS-01 and RLS-02.

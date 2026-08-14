# RLS-02 — Bring up I²C and the mini OLED

- **Status:** Blocked
- **Priority:** P0
- **Milestone:** M2 — Bench station
- **Depends on:** RLS-01

## User story

As the builder, I want the OLED and BMP280 working on one I²C bus so that display and pressure
measurement share a verified foundation.

## Outcome

The exact OLED and BMP280 are detected reliably, survive reboot, and produce visible/plausible
output without hiding bus failures.

## In scope

- Firmware framework decision OQ-001 and minimal reproducible toolchain.
- I²C scan, OLED test screen, BMP280 temperature/pressure readout.
- Documented addresses and bus configuration.

## Out of scope

- Final display layout, MQTT, and remaining sensors.

## Acceptance criteria

- [ ] Reproducible install/build/flash/monitor commands are documented.
- [ ] I²C scan detects the expected OLED and BMP280 addresses.
- [ ] OLED renders a stable test screen without flicker or corruption.
- [ ] BMP280 produces plausible temperature and pressure values with units.
- [ ] Both devices reinitialize automatically after three controlled reboots.
- [ ] A missing I²C device is reported explicitly rather than as a zero value.

## Validation

- Automated: configuration/build validation and any host-side logic tests supported by the chosen framework.
- Manual: I²C scan log, display photo, three-reboot checklist, and one unplugged-device test.

## Evidence

Blocked by RLS-01.

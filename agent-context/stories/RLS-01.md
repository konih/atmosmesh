# RLS-01 — Identify hardware and approve wiring

- **Status:** Ready
- **Priority:** P0
- **Milestone:** M1 — Hardware approved
- **Depends on:** —

## User story

As the builder, I want every module variant, pin, and voltage confirmed so that the station can be
assembled without damaging hardware or relying on a generic diagram.

## Outcome

An evidence-backed wiring table is approved for the exact physical modules. All unresolved
electrical assumptions are either answered or explicitly block the next story.

## In scope

- Front/back photographs of ESP32, both OLEDs, BMP280, DHT22, SDS011 and adapter, MQ135, and power supply.
- Exact connector orientation and pin labels.
- Supply and signal-level verification from markings, authoritative documentation, or measurement.
- I²C controller/address expectations, MQ135 divider calculation, and available resistor values.
- Final wiring table and pre-power checklist under `docs/hardware/`.

## Out of scope

- Full station assembly, firmware implementation, enclosure, or cluster work.

## Acceptance criteria

- [ ] Every selected module has readable front/back evidence linked from the hardware document.
- [ ] Supply voltage, logic level, connector order, and role are known for every connection.
- [ ] No path can apply 5 V to an ESP32 GPIO or `3V3` pin.
- [ ] The MQ135 ADC divider has a safe measured maximum with margin below 3.3 V.
- [ ] The selected 5-V supply is measured at ~5 V, enclosed on the primary side, and has adequate
      current for SDS011, MQ135, **and** ESP32 VIN if they share the rail (see `docs/hardware/power.md`).
- [ ] A pre-power continuity/voltage checklist exists and has been reviewed with the user.

## Validation

- Manual: visual cross-check against module markings and multimeter verification of supply rails.
- Calculation: divider output at worst-case measured/source voltage.
- Review: line-by-line wiring-table approval before power is connected.

## Evidence

- 2026-08-14: Manufacturer datasheets stored under `docs/hardware/datasheets/` with SHA-256 and
  source URLs. Chip-level comparison in `docs/hardware/spec-comparison.md`.
- 2026-08-14: USB/`esptool` identity of the connected controller recorded in
  `docs/hardware/inventory.md` (ESP32-D0WDQ6 rev 1.0, 4 MB, CP2102, MAC `ac:67:b2:37:26:78`).
- 2026-08-14: Station 5 V architecture recorded as D-005 / `docs/hardware/power.md`. Candidate PSU
  is the open `5V07 / 12V04` AC/DC; DC voltage and current capability not yet measured.
- 2026-08-14: GY-BMP280 SDA=GPIO21, SCL=GPIO19. AM2302 data=GPIO18. OLED SDA=GPIO5, SCL=GPIO4
  at 0x3C. Firmware pin map in `firmware/include/atmosmesh/pins.hpp`.
- Still awaiting: front/back photographs, 5 V output measurement, MQ135 AO measurement.

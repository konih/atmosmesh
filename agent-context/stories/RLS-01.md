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
- [ ] The selected 5-V supply has adequate current capacity for SDS011 and MQ135.
- [ ] A pre-power continuity/voltage checklist exists and has been reviewed with the user.

## Validation

- Manual: visual cross-check against module markings and multimeter verification of supply rails.
- Calculation: divider output at worst-case measured/source voltage.
- Review: line-by-line wiring-table approval before power is connected.

## Evidence

Not started. Awaiting hardware photographs and power-supply details.

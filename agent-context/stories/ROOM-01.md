# ROOM-01 — Protected room carrier board

- **Status:** Blocked (design and plan complete; fabrication blocked on physical evidence)
- **Priority:** P1
- **Milestone:** Room variant hardware design
- **Depends on:** Exact module photos before fabrication approval

## User story

As the builder, I want a compact through-hole carrier for the Ideaspark ESP32/OLED, VEML7700,
SHT41, D-SUN PIR and buzzer so that wiring mistakes are contained and repeatable damage is less
likely.

## Outcome

A real, reviewable KiCad 10 project for a 60×80 mm, two-layer, THT carrier. The footprint and
connector pin orders remain explicitly provisional until readable front/back photographs and
measurements establish the exact hardware.

## Acceptance criteria

- [x] Project includes `.kicad_pro`, `.kicad_sch`, `.kicad_pcb`, local libraries, BOM and wiring
      documentation.
- [x] Board is 60×80 mm, two copper layers, THT, with four M3 mounting holes.
- [x] GPIO21/22 reach external I²C only through individual 330 Ω series resistors.
- [x] VEML7700 and SHT41 use direct 3.3 V plus local 220 nF decoupling so the onboard OLED's 3.3 V
      I²C pull-ups cannot exceed a lowered sensor VDD; no supply diode or resistor is fitted.
- [x] The confirmed VEML7700 header is VIN/3Vo/GND/SCL/SDA; VIN gets 3.3 V and 3Vo is NC.
- [x] PIR 5 V is default-open and reverse-feed protected; its signal reaches GPIO33 only through
      the active-low 2N3904 interface and 3.3 V pull-up.
- [x] Buzzer uses a low-side NPN driver; the flyback footprint is DNP unless a magnetic buzzer is
      confirmed.
- [x] Board and documentation visibly block fabrication until exact board, sensor, PIR and buzzer
      photos resolve pin order, row spacing, voltages and buzzer type.
- [x] A complete through-hole placement, assembly, unpowered-test, commissioning and stop-condition
      plan is committed with the design.
- [x] Deterministic structural validation, KiCad ERC/DRC/export, and repository gates pass, with
      unconnected items explicitly expected because copper routing is blocked on physical evidence.

## Validation plan

1. Run `python3 hardware/kicad/atmosmesh-room/validate_room_carrier.py`; it must fail before the
   project exists, then pass only with the required geometry, components, nets and warnings.
2. Run KiCad CLI ERC at all severities and DRC with schematic parity and refilled zones.
3. Export a schematic PDF and netlist, then inspect the artifacts and board render.
4. Run `task check` and host tests. Firmware builds are not required because this story changes no
   firmware.
5. Keep this story blocked from fabrication until front/back photographs and measurements are
   linked as evidence.

## Evidence

- 2026-08-27: Validation contract added first. Initial run failed because the KiCad project files
  did not yet exist, proving the check detects the absent design.
- 2026-08-28: Confirmed Adafruit-style VEML7700 pin order recorded as VIN/3Vo/GND/SCL/SDA; `3Vo`
  is NC and no extra pull-ups are fitted.
- 2026-08-28: Sensor supply revised to direct 3.3 V. Series resistance/diode drop was rejected
  because the onboard OLED holds I²C pull-ups at 3.3 V. `C1`, `C3`, `C4` use available 220 nF
  ceramics (47–220 nF acceptable).
- 2026-08-28: `validate_room_carrier.py` passed. KiCad 10.0.5 ERC reported 0 errors/warnings; DRC
  reported 0 rule violations and 0 schematic-parity problems. Its 52 unconnected items are the
  intentional unrouted ratsnest while physical identities block safe copper routing.
- 2026-08-28: KiCad XML netlist and schematic PDF exported successfully; rendered PDF was visually
  inspected after hiding inherited template metadata and separating references/values.
- 2026-08-28: `task check` passed and `task test` passed 121/121 host tests. Firmware build was not
  run because ROOM-01 changes no firmware.

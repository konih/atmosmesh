# ROOM-01 — Protected room carrier board

- **Status:** Blocked (design and plan complete; energising blocked on physical measurement)
- **Priority:** P1
- **Milestone:** Room variant hardware design
- **Depends on:** Row-spacing measurement and 5 V module measurements before energising

## User story

As the builder, I want a compact through-hole carrier for the ideaspark ESP32 1.14 inch TFT board, VEML7700,
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
- [x] VEML7700 and SHT41 use direct 3.3 V plus local 220 nF decoupling; no supply diode or resistor
      is fitted. (Rationale corrected 2026-08-28 — see D-022's superseded-rationale note.)
- [x] `R_PU_SDA`/`R_PU_SCL` 4.7 kΩ supply the only I²C pull-ups on the bus (D-023).
- [x] No carrier net reaches GPIO2/4/15/18/23/32 (TFT), GPIO12 (strap) or GPIO1/3 (USB-UART),
      enforced by a generator guard and a validator assertion, both mutation-proved (D-024).
- [x] The confirmed VEML7700 header is VIN/3Vo/GND/SCL/SDA; VIN gets 3.3 V and 3Vo is NC.
- [x] The confirmed SHT41 header is VIN/GND/SCL/SDA and the confirmed D-SUN PIR header is
      GND/OUT/VCC; both corrected from the provisional orders on 2026-08-28.
- [x] PIR 5 V is default-open and reverse-feed protected; its signal reaches GPIO33 only through
      the active-low 2N3904 interface and 3.3 V pull-up.
- [x] Buzzer uses a low-side NPN driver; the flyback footprint is DNP unless a magnetic buzzer is
      confirmed. **Superseded 2026-08-28 by D-025** — the part is a confirmed Keyes 3-pin S/VCC/−
      active module, so GPIO25 drives `S` directly and the driver and flyback are removed.
- [x] Board and documentation visibly block energising until the controller row spacing and both
      5 V modules' supply voltage and output swing are measured.
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
- 2026-08-28: Independent-review corrections (real diode/NPN symbols with polarity and C/B/E
  semantics) were found to pass structural validation while failing three other gates: 27 ERC
  `lib_symbol_mismatch` warnings from a library and an embedded schematic copy built from separate
  sources, one `silk_over_copper` DRC violation where the new anode marker clipped H2's mask, and
  `git diff --check` "space before tab" in the footprint writer. All three fixed; the local library
  is now derived from the exact symbol list the schematic embeds.
- 2026-08-28: `generate_project.py` verified to regenerate every artifact byte-identically before
  any further edit, so the committed KiCad files provably match the generator.
- 2026-08-28: Operator supplied `docs/hardware/ideaspark-esp32-tft-pinout.png`. The controller is
  the ideaspark ESP32 **1.14 inch TFT LCD** board. Its display is SPI, so the "integrated OLED holds
  the I²C pull-ups" reasoning behind D-022 was false; D-023 adds the pull-ups the bus actually
  needs and D-024 records the six display-owned GPIOs.
- 2026-08-28: U1's socket map was provisional and wrong in almost every position — pad 7 carried
  GPIO33 where the real board has GPIO26, and pad 30 carried 3V3 where the real board has GPIO23
  (TFT MOSI). Rebuilt from a named pin table with a generator guard that refuses reserved pins.
- 2026-08-28: Operator confirmed the SHT41 header as VIN/GND/SCL/SDA and the D-SUN PIR header as
  GND/OUT/VCC. Both were wrong on the carrier: SHT41 had SDA and SCL crossed, and the PIR order
  would have put 5 V on the module's ground pin.
- 2026-08-28: Both new gates mutation-proved. Mapping GPIO23 in `U1_NETS` raises in the generator;
  injecting a rogue `GPIO18_*` net fails the validator; renaming `R_PU_SDA` fails the pull-up
  assertion. A first attempt at the reserved-GPIO mutation tripped the required-nets check instead
  and proved nothing, so it was redone in isolation.
- 2026-08-28: Operator identified the buzzer as a no-name Keyes 3-pin breakout, S / VCC / −, the
  same part AtmosMesh v1 drives active-HIGH from GPIO25. The carrier's 2-pin low-side-NPN design
  could not have worked with it: the module's `S` input had no connection at all. Rebuilt per D-025.
- 2026-08-28: Final review caught U1's socket **symbol** still carrying inherited DevKit V1 pin
  names, which run from the opposite end of the row. The nets were correct but the schematic would
  have *displayed* `3V3` against pad 30, which is really GPIO23 / TFT MOSI, and `GPIO34` against
  pad 4, which is really the GPIO12 flash strap. Symbol renamed to
  `Ideaspark_ESP32_1V14_TFT_30Pin` and every pin relabelled from the confirmed map. The validator
  now asserts pads 30, 24, 4 and 10 are labelled GPIO23/18/12/32 and left unconnected, and refuses
  the DevKit name; both mutation-proved.
- 2026-08-28: Operator completed the Zener inventory — 1N4733, 4738, 4739, 4740, 4741, 4742, 4744,
  4745, 4746, 4748. The lowest is still 5.1 V and no 1N4728 is held, so the "no Zener clamp" rule
  now rests on a complete list rather than a partial one.

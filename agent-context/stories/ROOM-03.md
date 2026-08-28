# ROOM-03 — Build the Room carrier on perfboard

- **Status:** Ready to solder (energising still blocked)
- **Priority:** P1
- **Milestone:** Room variant hardware design
- **Depends on:** ROOM-01, ROOM-02

## User story

As the builder, I want a hole-level plan for the 31 × 27 perfboard I actually own, so that I can
solder the Room carrier without translating a PCB layout in my head.

## Outcome

`perfboard.md` as the build target, gated by `validate_room_perfboard.py`, with the KiCad PCB
explicitly parked so nobody mistakes it for a fabrication candidate.

## Acceptance criteria

- [x] The plan states the operator's authoritative 31 × 27 hole count and derives the 76.2 × 66.04
      mm span, noting the unholed lead-in strips that explain the 7 × 9 cm label.
- [x] Every schematic part has a placement zone; the validator fails if one is unplaced.
- [x] The socket is proven to fit: 15 holes per row and 10 pitches between rows inside 31 × 27.
- [x] The plan opens with a dry-fit step that settles the row spacing, discharging D-024's open item.
- [x] The copper-side mirroring hazard is stated, and the wiring table is authoritative by pin name.
- [x] The plan wires no display, strap or USB-UART pin, enforced by the validator.
- [x] The KiCad PCB is marked parked in the README and must not be ordered.
- [x] All five perfboard assertions are mutation-proved.
- [ ] Dry-fit performed and the 25.4 mm row spacing confirmed on the physical board.
- [ ] Board soldered and the unpowered checks in `wiring.md` recorded.

## Validation plan

1. `python3 hardware/kicad/atmosmesh-room/validate_room_perfboard.py` must pass, and must fail on
   an unplaced part, a reserved GPIO, a removed orientation warning, or a socket that cannot fit.
2. `validate_room_carrier.py`, ERC and DRC must stay green: the schematic is still the contract.
3. Do not apply power. Both 5 V jumpers stay open until ROOM-02's measurements are recorded.

## Evidence

- 2026-08-28: Five assertions mutation-proved — removing `R_SDS_TX` from the plan, adding a GPIO23
  instruction, softening "mirrored", and widening the socket past 31 columns or 27 rows each fail.
  The first attempt at the fit mutation tripped the hole-count string check instead and proved
  nothing, so it was redone against `U1_PINS_PER_ROW` and `U1_ROW_PITCHES` directly.
- 2026-08-28: ERC 0, DRC 0 violations and 0 schematic-parity problems, both validators PASS.

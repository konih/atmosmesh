# ROOM-02 — SDS011 particulate sensor on the Room carrier

- **Status:** Blocked (design complete; energising blocked on 5 V measurements)
- **Priority:** P1
- **Milestone:** Room variant hardware design
- **Depends on:** ROOM-01, plus VIN and 5 V budget measurements before `JP_SDS_5V` is closed

## User story

As the builder, I want the SDS011 that already worked on AtmosMesh v1 on the Room carrier, with
more protection than the bench had, so that a repeat of the 2026-08-17 wiring fault cannot damage
the ESP32.

## Outcome

`J_SDS`, `JP_SDS_5V`, `R_SDS_RX`, `R_SDS_TX`, `C6` and `C7` on the Room schematic, with the UART
crossing and the diodeless 5 V rail enforced by generator guards rather than by documentation.

## Acceptance criteria

- [x] Sensor TXD reaches GPIO16/RX2 and GPIO17/TX2 reaches sensor RXD, each through a 1 kΩ series
      resistor.
- [x] `generate_project.py` raises rather than emit a straight-through SDS011 UART, and the refusal
      is mutation-proved.
- [x] The SDS011 5 V rail carries no series diode, enforced by a mutation-proved generator guard,
      because a Schottky drop lands under the sensor's 4.7 V minimum.
- [x] `JP_SDS_5V` is default-open and the protected side has bulk and HF decoupling.
- [x] GPIO16/GPIO17 are confirmed free on the ideaspark 1.14 inch TFT board and are not among the
      display, strap or USB-UART pins reserved by D-024.
- [x] `wiring.md` states the module's own connector order is unconfirmed and must be read from its
      printed labels.
- [ ] `VIN` measured under Wi-Fi load and shown to stay above 4.7 V.
- [ ] The shared 5 V budget re-checked with the SDS011 fan included.
- [ ] Rail ripple measured against the < 20 mV specification with the fan running.

## Validation plan

1. `python3 hardware/kicad/atmosmesh-room/validate_room_carrier.py` must pass, and must fail if the
   SDS011 nets or references are removed.
2. Mutation-prove both generator guards: swap the header's RXD/TXD nets, and add a Schottky to the
   SDS011 rail. Each must raise.
3. KiCad ERC and DRC with schematic parity.
4. Do not close `JP_SDS_5V` until the three measurement criteria above are recorded here.

## Evidence

- 2026-08-28: Both generator guards mutation-proved. Swapping the header's RXD/TXD nets raises
  "SDS011 TXD must reach GPIO16/RX2 through R_SDS_RX"; adding `D_SDS` to the rail raises "puts a
  series diode on the SDS011 rail; its minimum is 4.7 V".
- 2026-08-28: ERC 0 violations, DRC 0 violations and 0 schematic-parity problems, structural
  validation PASS.
- 2026-08-28: Operator chose hard-wired TX with series resistors over a default-open TX jumper, so
  that laser duty-cycling against the 8000 h life stays available without rework.

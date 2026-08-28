# ROOM-04 — Duty-cycle the SDS011 with its own hibernation command

- **Status:** Proposed (firmware only; no hardware change)
- **Priority:** P2
- **Milestone:** Room variant hardware design
- **Depends on:** ROOM-02, ROOM-03

## Why this exists

The SDS011 is rated for **8000 h of laser-on time** (`docs/hardware/spec-comparison.md`), and the
datasheet names duty-cycling as its own suggestion for a continuous monitor. Run continuously, that
budget is spent in under a year.

## The sensor already does this

`docs/hardware/datasheets/nova-sds011.pdf` lists under **Extended functionality**:

1. Manual hibernation (Sleep and wake up)
2. Timed hibernate (Cycle to work)

and specifies a **sleep current below 4 mA**, annotated "Lase&Fan sleep" — the laser and fan both
stop. Item 2 is a built-in periodic duty cycle requiring no host scheduling at all.

The operator's ROOM-02 decision to hard-wire the UART TX leg is what makes this reachable: commands
travel GPIO17/TX2 → `R_SDS_TX` → sensor RXD. **No hardware change is needed.**

Note the byte-level frames are *not* in the datasheet held here — it says only "If you have any
other requirements, please contact us." Nova publishes the control protocol as a separate document.
Obtain it before writing the firmware; do not guess frame formats at a sensor.

## Rejected: an IRLZ34N low-side power switch

An earlier revision of this story proposed switching the sensor's 5 V with one of the ten IRLZ34N
logic-level MOSFETs in stock. **Rejected on two independent grounds.**

- **The topology does not work.** A low-side switch opens the sensor's *ground return* while its
  VCC stays at 5 V. The load is not de-powered; its ground floats up until current stops and the
  sensor sits partially biased at an intermediate potential. De-powering a load requires high-side
  switching, and no P-channel MOSFET is recorded in the inventory. The relay modules that are in
  stock would reach roughly 100k contact operations in about 70 days at one cycle per minute.
- **It is unnecessary.** The sensor hibernates on command, at under 4 mA, with the fan and laser
  both stopped. Adding a transistor to do what the device already does is pure added failure
  surface.

The parasitic-path objection recorded in the earlier revision — TX2 idling HIGH into an unpowered
sensor's RXD — disappears with the switch, because the sensor is never externally unpowered.

## Acceptance criteria

- [ ] Nova SDS011 control-protocol document obtained and committed to `docs/hardware/datasheets/`.
- [ ] Duty cycle chosen against the 8000 h budget and recorded with its arithmetic.
- [ ] Firmware issues the hibernation command and handles the sensor being asleep without logging a
      fault; the 2026-08-17 capture showed a no-frame message at ~93 lines/s flooding the log, and a
      sleeping sensor must not reproduce that.
- [ ] Measured sleep current confirms the sensor actually hibernated rather than merely stopped
      reporting.

## Not in scope

No hardware change. ROOM-03 solders the board without any power switch, and the sensor runs
continuously until this story lands.

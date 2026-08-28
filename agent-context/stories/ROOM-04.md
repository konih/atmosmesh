# ROOM-04 — Duty-cycle the SDS011 laser with an IRLZ34N low-side switch

- **Status:** Proposed (not built; needs a decision before any wiring)
- **Priority:** P2
- **Milestone:** Room variant hardware design
- **Depends on:** ROOM-02, ROOM-03

## Why this exists

The SDS011 is rated for **8000 h of laser-on time** (`docs/hardware/spec-comparison.md`), and the
datasheet's own suggestion for a continuous monitor is to duty-cycle it. Running it continuously
spends that life in under a year.

`docs/elektronik-inventar.md` records **10× IRLZ34/IRLZ34N**. These are *logic-level* N-channel
MOSFETs: they turn fully on from a 3.3 V gate, so an ESP32 GPIO can switch the sensor's 5 V supply
directly with no driver stage. That makes hardware duty-cycling available for the cost of one
transistor and two resistors.

## Sketch

Low-side switch in the SDS011 ground return, gate from a free GPIO — GPIO27 and GPIO26 are both
free and are neither display, strap nor USB-UART pins under D-024.

```text
GPIO27 ── R_SDS_GATE 220 Ω ── IRLZ34N gate
IRLZ34N gate ── R_SDS_GPD 100 kΩ ── GND     (an open gate is undefined)
SDS011 GND return ── IRLZ34N drain
IRLZ34N source ── GND
```

## The objection that has to be answered first

With the sensor's ground switched off, the ESP32 still drives `GPIO17/TX2` into the sensor's `RXD`
through `R_SDS_TX` 1 kΩ, and UART idle is **HIGH**. That can forward-bias the sensor's input
protection and partially power an unpowered module through its own ESD diodes. The 1 kΩ bounds it
to roughly 3 mA, which is survivable but is exactly the kind of parasitic path that produces
confusing bench faults.

Options, none yet chosen:

1. Hold `GPIO17` LOW in firmware whenever the switch is off. Cheapest; relies on firmware
   discipline, which is what the 2026-08-17 contention fault already cost this project once.
2. Switch the **high side** instead, with a P-channel MOSFET. No IRLZ34N applies; nothing suitable
   is recorded in the inventory.
3. Drop the TX leg. The sensor reports autonomously at 1 Hz and needs no commands, so the leg exists
   only for duty-cycling — and with hardware power switching, sleep commands are unnecessary anyway.
   This is the tidiest, and it reverses the operator's earlier choice to hard-wire TX.

## Acceptance criteria

- [ ] One of the three options above chosen and recorded as a decision.
- [ ] Gate resistor and pulldown fitted; an open gate must never be possible.
- [ ] Measured: the sensor draws no current through any signal pin while switched off.
- [ ] Duty cycle chosen against the 8000 h budget and recorded.

## Not in scope

Nothing here is built. ROOM-03 solders the board without this switch, and the SDS011 runs
continuously until this story is decided.

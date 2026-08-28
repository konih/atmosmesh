# AtmosMesh Room — perfboard build plan

This is the **build target**. The KiCad project is the electrical contract; the PCB file is parked
(see [README.md](README.md)) because the board is hand-soldered, not fabricated.

## The board

Operator-supplied, 2026-08-28: **31 × 27 holes**, 2.54 mm pitch, sold as 7 × 9 cm. The hole count
is authoritative — the extra millimetres are unholed lead-in strips at the edges on both sides.

| Fact | Value |
| --- | --- |
| Grid | 31 columns × 27 rows |
| Pitch | 2.54 mm |
| Span between outer hole centres | 76.2 mm × 66.04 mm |
| Total holes | 837 |

## Coordinate convention — read this before counting anything

Columns are numbered **1–31 left to right** and rows **1–27 top to bottom**, **viewed from the
component side**, with the ideaspark board's USB connector pointing toward column 1.

> **The pinout sheet is a top view. Soldering happens from the copper side, where left and right are
> mirrored.** Two connector orders on this design were already found reversed before a single joint
> was made. Do not translate this plan into copper-side coordinates in your head. **The wiring table
> in [wiring.md](wiring.md) is authoritative by pin *name*** — every joint should be checked against
> the name printed next to the hole, not against a coordinate.

## Step 1 — dry-fit, and settle the row spacing

Before cutting anything, push the ideaspark board's header pins into the grid and count.

The design assumes 25.4 mm between the two pin rows, which is **exactly 10 pitches**. If the board
seats cleanly with 10 clear holes between its rows, that assumption is discharged and the open
row-spacing item in D-024 and ROOM-01 closes. If it does not seat, stop: nothing below is valid.

Also confirm 15 pins per row.

## Placement zones

U1 occupies **rows 8 and 18, columns 2–16** — 15 holes along each row, 10 pitches apart. Pad 1
(`VIN`, left row) and pad 16 (`3V3`, right row) both sit at column 2, the USB end.

| Zone | Columns | Rows | Contents |
| --- | --- | --- | --- |
| Controller | 2–16 | 8 and 18 | U1 socket rows |
| Under-board | 2–16 | 9–17 | Wiring only. Nothing taller than the socket |
| Light edge | 1–16 | 1–7 | `J_VEML`, `C3`, `R_SDA`, `R_SCL`, `R_PU_SDA`, `R_PU_SCL` |
| Ventilated edge | 1–16 | 19–27 | `J_SHT`, `C4`, `C1`, `C2`, `TP_3V3`, `TP_5V`, `TP_GND`, `TP_SDA`, `TP_SCL` |
| PIR block | 25–31 | 15–27 | `J_PIR`, `JP_PIR_5V`, `D_PIR`, `Q_PIR`, `R_PIR_IN`, `R_PIR_PD`, `R_PIR_PU`, `C5` |
| SDS011 block | 17–24 | 15–27 | `J_SDS`, `JP_SDS_5V`, `R_SDS_RX`, `R_SDS_TX`, `C6`, `C7` |
| Buzzer | 17–24 | 8–14 | `J_BEEP`, `R_BEEP_S` |
| Indicators | 25–31 | 1–7 | `D_LED_3V3`, `R_LED_3V3`, `D_LED_SDS`, `R_LED_SDS`, `D_LED_PIR`, `R_LED_PIR` |

Put the three indicator LEDs together at one visible edge and label them on the board. They are
only useful if you can read them at a glance while the board is powered — grouping them beats
placing each one next to its own circuit.

Keep the 5 V domain physically fenced to the PIR and SDS011 blocks so that a slipped 5 V strand cannot land on
a 3.3 V or GPIO joint. `J_VEML` goes at the row-1 edge with an opaque divider between it and the
TFT backlight. `J_SHT` goes at the row-27 edge, away from the regulator, the PIR electronics and
the buzzer.

## Hole budget

U1 reserves a 15 × 11 block of the 31 × 27 grid. That leaves 837 − 165 = 672 holes for 37 further
parts and their wiring, which is not tight.

## Build order

1. Dry-fit U1 per step 1. Do not proceed if the rows do not seat.
2. Fit the two socket strips. Solder one pin at each end first and re-check squareness.
3. Fit the ground and 3.3 V distribution wiring. Verify continuity before anything else goes on.
4. Fit the I²C branch: `R_SDA`, `R_SCL`, `R_PU_SDA`, `R_PU_SCL`, then `J_VEML` and `J_SHT`.
5. Fit the decoupling: `C1`, `C2`, `C3`, `C4`. Observe polarity on `C2`.
6. Fit the buzzer: `R_BEEP_S`, then `J_BEEP`.
7. Sleeve every 5 V-carrying lead and module flying lead in heat shrink before fitting it. A 5 V
   strand that slips onto a GPIO is the exact failure the whole protection scheme is built against,
   and fencing the domains only helps if the wires stay where you put them.
8. Fit the 5 V domain last, jumpers **left open**: `J_PIR`, `J_SDS`, `D_PIR`, `Q_PIR`, the PIR
   resistors, `R_SDS_RX`, `R_SDS_TX`, `C5`, `C6`, `C7`.
9. Fit the test-point pairs.
10. Fit the indicator LEDs and their resistors. Observe LED polarity: the flat/short lead is the
   cathode and goes to GND.

Then follow the unpowered checks, staged commissioning and stop conditions in
[wiring.md](wiring.md) unchanged. Both 5 V jumpers stay open until the measurements named there and
in ROOM-02 are recorded.

## What this plan does not settle

- The module connector orders for the SDS011 are still unread from its own printed labels.
- `VIN` under Wi-Fi load, the shared 5 V budget and the SDS011 rail ripple are unmeasured.
- Nothing here authorises applying power. It authorises soldering.

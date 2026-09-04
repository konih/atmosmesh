# AtmosMesh Spot, first unit — bill of materials, layout and wiring

The first Spot is the **In/Out variant with the DS18B20 probe** (operator decision,
4 September 2026): SHT41 and VEML7700 on the board, an HLK-LD2410S radar for presence, a BME280
on a cable outside the window, and a temperature probe. Story:
[`SP-01`](../../../agent-context/stories/SP-01.md). Companion drawings:
[`atmosmesh-spot-layout.svg`](atmosmesh-spot-layout.svg) (hole-by-hole placement) and
[`atmosmesh-spot-wiring.svg`](atmosmesh-spot-wiring.svg) (schematic-style net drawing, every
pin by its printed name).

**One 3.3 V domain.** The SuperMini's `5V` pin is not wired to anything. Every part below is
datasheet- or operator-confirmed to run from 3.3 V, and no signal on the board can exceed 3.3 V.
This document is a build contract, not power approval: the unpowered checks and the staged
bring-up at the end still apply.

Evidence level per part:

| Part | Pin order | Electrical | Source |
| --- | --- | --- | --- |
| HLK-LD2410S | **confirmed**, manual Table 3-2 | **confirmed**, 3.0–3.6 V, all pins 0–3.3 V | [`docs/hardware/datasheets/`](../../../docs/hardware/datasheets/README.md) |
| SHT41, VEML7700 | **photo-confirmed** on the spares, 4 September | 3.3 V into their own LDO and level shifter | photo pass, inventory |
| ESP32-C3 SuperMini OLED | **photo-confirmed** silkscreen, 4 September | 3.3 V logic, on-board LDO | photo pass; OLED I²C pins still by scan |
| BME280 breakout `GY-BM E/P 280` | **photo-confirmed**, 6-pin | **3.3 V only**, no regulator, four 10 kΩ | photo pass; BME vs BMP by chip ID |
| DS18B20 probe | cable colours are a claim; **ring out** | 3.3 V parasite or powered | Maxim datasheet (to file) |

The photos themselves were deleted after evaluation at the operator's request; the facts they
established are in `docs/hardware/inventory.md` under the 4 September photo pass.

## 1. Bill of materials

Everything except the cable is in stock. Reference designators match SP-01.

| Ref | Qty | Part | Value / type | From stock | Note |
| --- | ---: | --- | --- | --- | --- |
| `U1` | 1 | ESP32-C3 SuperMini with 0.42" OLED | — | 1 of 5 | The board on the bench |
| `U2` | 1 | SHT41 breakout | I²C `0x44` | 1 of 4 (last free one after Room and Room v2) | `J_SHT` |
| `U3` | 1 | VEML7700 breakout | I²C `0x10` | 1 of 3 | `J_VEML` |
| `U4` | 1 | HLK-LD2410S | 24 GHz presence radar | 1 of 1 | `J_RAD` |
| `U5` | 1 | BME280 breakout `GY-BM E/P 280`, 6-pin, no regulator | I²C `0x76` | 1 of 6 | outdoor, on the cable; CSB and SDO stay on their on-board pull-ups |
| `U6` | 1 | DS18B20 encapsulated probe, 2.5 m | 1-Wire | 1 of 2 | `J_1W` |
| `PCB` | 1 | perfboard 14 × 20 holes, 2.54 mm, double-sided | — | stock | |
| `S1`, `S2` | 2 | female header strip, 8-pin | 2.54 mm | header stock | U1 socket rows; the board stays removable |
| `J_SHT` | 1 | female header, 4-pin | 2.54 mm | header stock | |
| `J_VEML` | 1 | female header, 5-pin | 2.54 mm | header stock | pin 2 (`3Vo`) left unwired |
| `J_RAD` | 1 | female header, 5-pin | 2.54 mm (measure the radar's pins first) | header stock | radar stands upright |
| `J_OUT` | 1 | PCB screw terminal, 4-pin | 5.08 mm assumed — measure | 1 of 10 | outdoor cable |
| `J_1W` | 1 | PCB screw terminal, 3-pin | 5.08 mm assumed — measure | 1 of 10 | probe cable |
| `R_PU_SDA`, `R_PU_SCL` | 2 | resistor | 2.2 kΩ | E24 kit | In/Out value (cable on the bus) |
| `R_OUT_SDA`, `R_OUT_SCL` | 2 | resistor | 100 Ω | E24 kit | series to the cable |
| `R_1W` | 1 | resistor | 4.7 kΩ | E24 kit | 1-Wire pull-up |
| `D_CL1`–`D_CL4` | 4 | Schottky diode | 1N5819 | 4 of 10 | rail clamps on the cable lines; fitted on this unit |
| `C1`, `C_SHT`, `C_VEML`, `C_RAD`, `C_OUT` | 5 | ceramic capacitor | 100 nF | ceramic assortment | one per supply pin |
| `C_RAD_BULK` | 1 | electrolytic | 100 µF, ≥ 6.3 V | Elko assortment | radar transmit peaks (~118 mA) |
| `TP_3V3`, `TP_GND`, `TP_SDA`, `TP_SCL` | 4 | test-point pin | — | stock | |
| cable | 1 | 4-core, ≤ 3 m | flat 4-core or Cat5 offcut | **to source** | see section 6 |
| shield | 1 | vented housing for the outdoor BME280 | inverted cup or plate stack | workshop | not electronics |

Not fitted on this unit, footprint kept: nothing. The In variant's `J_SOIL` was dropped in SP-01.

Stock after this build: 4 SuperMinis, 3 SHT41 minus the two reserved (so 1 free), 2 VEML7700
minus one reserved (1 free), 0 LD2410S, 5 BME280, 1 DS18B20, 6 of 10 1N5819.

## 2. Controller: ESP32-C3 SuperMini OLED (`U1`)

Two rows of eight pins. **Photo-verified silkscreen, 4 September.** With the USB socket toward
row 1 and the OLED facing up, row A is the side carrying the `PWR` LED and the `IO8` LED, row B
the other side. In the layout below row A is column 4 and row B column 11; if your board sits
the other way round when the USB points at row 1, swap the columns, never the net list.

| Row A (col 4), from the USB end | Row B (col 11), from the USB end |
| --- | --- |
| `5V` — **not wired** | `10` GPIO10 |
| `GD` GND | `9` GPIO9 — BOOT button, strapping: unused |
| `3V` 3V3 — the rail | `8` GPIO8 — `IO8` LED, strapping: unused |
| `RX` GPIO20 — boot log: unused | `7` GPIO7 |
| `TX` GPIO21 — boot log: unused | `6` GPIO6 — SCL (by scan) |
| `2` GPIO2 — strapping: unused | `5` GPIO5 — SDA (by scan) |
| `1` GPIO1 — UART1 RX ← radar `OT1` | `4` GPIO4 — 1-Wire |
| `0` GPIO0 — UART1 TX → radar `RX` | `3` GPIO3 — radar `OT2` |

Beyond pin `0`/`3` the module continues for about two more pitches: `RST` and `BOOT` buttons and
a small red part with a white dot, presumed to be the antenna. That end is the keep-out.

> **Row spacing is settled:** the operator pushed the module into this perfboard on 4 September
> and it fits with three free columns on each side, so the rows are **7 pitches (17.78 mm)**
> apart at columns 4 and 11. **Still to verify before soldering:** the OLED's I²C pins by a bus
> scan on GPIO5/6 — they are not printed on the board. If the scan on GPIO5/6 does not show
> `0x3C`, the OLED is on other pins and the I²C net moves with it.

Firmware limits Wi-Fi TX power to about 8.5 dBm at start-up (ceramic antenna).

## 3. Board layout — 14 × 20 holes (35.6 × 50.8 mm)

Columns 1–14 left to right, rows 1–20 top to bottom, component side, USB toward row 1. Header and
terminal positions are exact; passives go in the row named for them, exact holes are the
builder's choice within that row.

```text
        col 1   2   3   4   5   6   7   8   9  10  11  12  13  14
       ┌────────────────────────────────────────────────────────────┐
row 1  │              [ USB-C overhangs this edge ]                 │  edge pads under the module: unused
row 2  │  ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   │
row 3  │  ·   ·   ·  (5V)  ·   ·   ·   ·   ·   ·  (G10) ·   ·   ·   │  ← U1 socket pins: col 4 (row A) and col 11 (row B)
row 4  │  ·   ·   ·  (GND) ·   ·   ·   ·   ·   ·  (G9)  ·   ·   ·   │     7 pitches apart (dry-fit confirmed 4 Sept)
row 5  │  ·   ·   ·  (3V3) ·   ·   ·   ·   ·   ·  (G8)  ·   ·   ·   │
row 6  │  ·   ·   ·  (G20) ·   ·   ·   ·   ·   ·  (G7)  ·   ·   ·   │
row 7  │  ·   ·   ·  (G21) ·   ·   ·   ·   ·   ·  (G6)  ·   ·   ·   │
row 8  │  ·   ·   ·  (G2)  ·   ·   ·   ·   ·   ·  (G5)  ·   ·   ·   │
row 9  │  ×   ×   ·  (G1)  ·   ·   ·   ·   ·   ·  (G4)  ·   ×   ×   │  × = keep-out: no copper, no wire,
row 10 │  ×   ×   ·  (G0)  ·   ·   ·   ·   ·   ·  (G3)  ·   ×   ×   │      no coating — buttons and antenna
row 11 │  ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   │      end of the module, rows 11–13
row 12 │  ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   │
row 13 │  ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   ×   │
row 14 │ [R_PU_SDA 2k2] [R_PU_SCL 2k2]    [C1 100n]   [J_OUT VIN]   │  J_OUT: 4-pole terminal, col 13,
row 15 │ [J_SHT: VIN GND SCL SDA] · [J_VEML: VIN 3Vo GND SCL SDA]   │  rows 14/16/18/20 (5.08 mm), cable
row 16 │ [C_SHT]   ·   ·   ·  [C_VEML] · [R_OUT_SDA][R_OUT_SCL] [J_OUT GND] │  exits the right edge
row 17 │ TP_3V3 TP_GND TP_SDA TP_SCL   [C_OUT 100n]  [R_1W 4k7]    │
row 18 │ [D_CL1][D_CL2][D_CL3][D_CL4]   ·   ·   ·   ·  [J_OUT SCL] │
row 19 │ [C_RAD_BULK 100µ] [C_RAD 100n]  ·   ·   ·   ·   ·   ·   ·  │
row 20 │ [J_RAD: 3V3 GND OT1 RX OT2]  · [J_1W: 3V3 · DQ · GND] [J_OUT SDA] │  J_1W: 3-pole terminal, cols 8/10/12
       └────────────────────────────────────────────────────────────┘     radar upright at this edge, face outward
```

Placement rules:

- **Keep-out at the module's far end.** Beyond pins `0`/`3` the module carries the `RST` and
  `BOOT` buttons and the presumed antenna, ending around row 12. Rows 11–13 across the whole
  width, plus columns 1–2 and 13–14 in rows 9–10, carry nothing: no wire on either side, no
  copper, and no conformal coating later. This is the single most important rule on this board.
- **The USB socket overhangs the row-1 edge.** The perfboard's strip of elongated edge pads at
  that edge sits under the module and stays unused.
- **Radar upright at the bottom edge**, antenna face (the side with the two large copper
  patches and the `HLK-LD2410S` print) pointing away from the board; its header is on the back,
  so the label side faces outward when it is plugged in correctly. On a windowsill unit lying
  flat that is the horizontal look into the room. The radar's 20 mm body spans about columns 1–8
  in the air above row 20; `J_1W` sits at columns 8–12 beside it, which clears it because the
  terminal is low.
- **SHT41 and VEML7700 stand upright on their headers** in row 15, in free air above the board.
  The SHT41 is on the far side from the SuperMini's LDO (the warm part) with the whole width of
  the board between them. If the light sensor must look up rather than sideways, put a
  right-angle female header at `J_VEML` and lay the module flat; nothing else changes.
- **Cable terminals at the edges** so the cables leave the board without crossing it: the
  outdoor cable off the right edge, the probe off the bottom edge.
- **If the screw terminals turn out to be 2.54 mm**, `J_OUT` takes col 13 rows 14–17 and `J_1W`
  cols 8–10 in row 20; nothing else moves.

## 4. Net list by pin name

```text
+3V3 rail (from U1 3V, col 4 row 5):
  ├── C1 100n ── GND                      beside the pin
  ├── R_PU_SDA 2k2 ── SDA
  ├── R_PU_SCL 2k2 ── SCL
  ├── J_SHT  VIN ──┤C_SHT 100n── GND
  ├── J_VEML VIN ──┤C_VEML 100n── GND     (J_VEML 3Vo: NOT CONNECTED)
  ├── J_RAD  3V3 ──┬┤C_RAD_BULK 100µ (+)── GND
  │                └┤C_RAD 100n── GND
  ├── J_OUT  VIN ──┤C_OUT 100n── GND
  ├── J_1W   3V3 ── R_1W 4k7 ── J_1W DQ ── U1 GPIO4
  ├── D_CL1 cathode (band)   [anode to J_OUT SDA side of R_OUT_SDA]
  ├── D_CL3 cathode (band)   [anode to J_OUT SCL side of R_OUT_SCL]
  └── TP_3V3

GND rail (U1 GD, col 4 row 4):
  every GND above, D_CL2 anode, D_CL4 anode, TP_GND

SDA (U1 GPIO5, col 11 row 8):
  ├── J_SHT SDA
  ├── J_VEML SDA
  ├── TP_SDA
  └── R_OUT_SDA 100R ── node OUT_SDA ──┬── J_OUT SDA
                                       ├── D_CL1 anode  (cathode → +3V3)
                                       └── D_CL2 cathode (anode → GND)

SCL (U1 GPIO6, col 11 row 7):
  ├── J_SHT SCL
  ├── J_VEML SCL
  ├── TP_SCL
  └── R_OUT_SCL 100R ── node OUT_SCL ──┬── J_OUT SCL
                                       ├── D_CL3 anode  (cathode → +3V3)
                                       └── D_CL4 cathode (anode → GND)

U1 GPIO3 (col 11 row 10) ── J_RAD OT2      presence in, high = occupied
U1 GPIO1 (col 4 row 9)   ── J_RAD OT1      radar UART TX → C3 UART1 RX
U1 GPIO0 (col 4 row 10)  ── J_RAD RX       C3 UART1 TX → radar UART RX
U1 GPIO4 (col 11 row 9)  ── J_1W DQ        1-Wire, with R_1W to +3V3
U1 5V    ── nothing
U1 GPIO2, 7, 8, 9, 10, RX (GPIO20), TX (GPIO21) ── nothing
```

The clamp diodes sit on the **cable side** of the 100 Ω resistors, so a discharge into the cable
is clamped before the resistor and the resistor then limits what reaches the C3. Band toward
`+3V3` on `D_CL1`/`D_CL3`; band toward the line on `D_CL2`/`D_CL4`. Check each with the diode
test before power: line to 3V3 must conduct one way only, and GND to line likewise.

## 5. Connection diagrams per module

### 5.1 SHT41 (`J_SHT`, 4 pins) — on board

```text
              J_SHT
 +3V3 ──────── 1  VIN
 GND  ──────── 2  GND
 SCL  ──────── 3  SCL
 SDA  ──────── 4  SDA
```

Photo-confirmed on the spare (`SHT4X` board): `VIN GND SCL SDA`. Address `0x44`, no address pin.
The board carries a `662K` 3.3 V LDO and a level shifter, so `VIN` at 3.3 V reaches the sensor
at about 3.2 V, which is fine. Its `103` array serves the shifter's sensor side; the two empty
pads at the top edge are the host-side pull-ups, so **this board contributes no pull-up** to the
bus — the carrier's `R_PU_*` do that job.

### 5.2 VEML7700 (`J_VEML`, 5 pins) — on board

```text
              J_VEML
 +3V3 ──────── 1  VIN
 NC   ──────── 2  3Vo     regulator output — never driven, never tied to 3V3
 GND  ──────── 3  GND
 SCL  ──────── 4  SCL
 SDA  ──────── 5  SDA
```

Photo-confirmed on the spare (`HW-900` board, "Vin/logic 3-5V"): `VIN 3VO GND SCL SDA`. Address
`0x10`. Same regulator-plus-level-shifter architecture as the SHT41 board, but with all four
elements of its `103` array in use, so it presents about 10 kΩ per line to the bus. Face the
sensor away from the OLED.

### 5.3 HLK-LD2410S (`J_RAD`, 5 pins) — on board, manual Table 3-2

```text
              J_RAD (J2 on the module, order as printed: 3V3 GND OT1 RX OT2)
 +3V3 ──────── 1  3V3     3.0–3.6 V; C_RAD_BULK 100 µF + C_RAD 100 nF right here
 GND  ──────── 2  GND
 GPIO1 ─────── 3  OT1     the module's UART TX (0–3.3 V) → C3 UART1 RX
 GPIO0 ─────── 4  RX      the module's UART RX (0–3.3 V) ← C3 UART1 TX
 GPIO3 ─────── 5  OT2     presence: HIGH = somebody, LOW = nobody (0–3.3 V)
```

Nothing between the module and the GPIOs: the manual gives every pin as 0–3.3 V, so there is no
level to shift and no fault current to limit beyond what a 3.3 V rail can supply. **`OT1` is not
the presence pin.** UART 115200 8N1; the default report is the 5-byte minimal frame
`6E · state · distance-cm (2, little-endian) · 62`, state 0/1 = nobody, 2/3 = somebody. The J1
pads (SWD) are not used.

Photo-confirmed: the front silkscreen reads `HLK-LD2410S` with the header labelled `3V3 GND OT1
RX OT2`, exactly the manual's order; the factory header is on the back, five pins, standard
2.54 mm by appearance against the 20 mm body. Measure it once before soldering `J_RAD`. The
antenna face (labels, two copper patches) looks away from the board.

### 5.4 BME280, outdoor (`J_OUT`, 4-pole screw terminal) — on the cable

```text
   board                                   cable, ≤ 3 m                    GY-BM E/P 280 (6 pins)
 +3V3 ──────── J_OUT 1  VIN ─────────── (pair A, wire 1) ─────────────── VCC
 GND  ──────── J_OUT 2  GND ─────────── (pair B, wire 1) ─────────────── GND
 OUT_SCL ───── J_OUT 3  SCL ─────────── (pair A, wire 2: twisted with VCC) ── SCL
 OUT_SDA ───── J_OUT 4  SDA ─────────── (pair B, wire 2: twisted with GND) ── SDA
                                                                          CSB  — leave open (10 kΩ to VCC on the board: I²C mode)
                                                                          SDO  — leave open (10 kΩ to GND on the board: 0x76)
```

Photo-confirmed: the breakout is the purple 6-pin `GY-BM E/P 280`, front order `VCC GND SCL SDA
CSB SDO`, **no regulator** (one capacitor, four `103` resistors) — it is a 3.3 V-only board,
which is exactly right here and would be destroyed on a 5 V feed. Address `0x76` from the SDO
pull-down; strap SDO to VCC for `0x77` if ever needed. The board carries its own 100 nF, so no
extra capacitor at the outdoor end. The sensor chip is marked `31E / UP`; the `E/P` board is
shared between BME280 and BMP280, so the first scan reads the chip-ID register: `0x60` is a
BME280, `0x58` a BMP280 without humidity.

### 5.5 DS18B20 probe (`J_1W`, 3-pole screw terminal)

```text
 +3V3 ──┬───── J_1W 1  3V3 ────── probe VDD
        R_1W 4k7
 GPIO4 ─┴───── J_1W 2  DQ  ────── probe DATA
 GND  ──────── J_1W 3  GND ────── probe GND
```

Powered mode (VDD wired), not parasite. **Ring the three wires out** against the probe's own
datasheet pin order at the sensor end with a continuity meter before trusting any colour; on the
encapsulated probes in stock the colours are a product claim. The 2.5 m cable is inside the
1-Wire limits with a 4.7 kΩ pull-up.

### 5.6 Test points

`TP_3V3`, `TP_GND`, `TP_SDA`, `TP_SCL` in row 14. The unpowered checks and the rise-time capture
use them; label them on the board.

## 6. The outdoor cable

- **Up to 3 m**, four cores. A Cat5 offcut works: use two pairs, **VIN with SCL** on one and
  **GND with SDA** on the other, so neither clock nor data is twisted with the other. Leave the
  other two pairs unconnected at both ends. A flat 4-core telephone cable also works for a window
  frame; there the pairing rule does not apply and the length limit is the same.
- **Bus speed 50 kHz** on this variant (firmware flag). Pull-up sum, from what has been read off
  the boards (operator, 4 September): the VEML7700 carries a `103` part, so 10 kΩ per line if it
  is a resistor or a two-element array; the SHT41 has an **empty** `103` footprint, so nothing.
  The SuperMini's OLED and the outdoor BME280 are unread and assumed 10 kΩ each until measured.

  | Fitted pull-ups per line | Total | Sink at 3.3 V |
  | --- | ---: | ---: |
  | carrier 2.2 kΩ + VEML7700 10 kΩ | 1.8 kΩ | 1.8 mA |
  | + BME280 10 kΩ | 1.5 kΩ | 2.2 mA |
  | + OLED 10 kΩ | 1.3 kΩ | 2.5 mA |

  All inside the 3 mA limit, so nothing has to be lifted. Confirm with the meter, unpowered:
  VEML7700 `SDA` to `VIN` and `SCL` to `VIN` about 10 kΩ each (a `103` capacitor would read open);
  SHT41 the same pairs open. Then capture SDA at the board end on the DSO2D15 after the cable is
  at its final length and record the rise time (must be under 1 µs).
- **Outdoor end:** breakout in a vented shield under the sill, out of rain and sun, cable in a
  drip loop, no coating on the sensor.
- **Not in stock.** The JST-PH set has pre-crimped 24 AWG leads that make a tidy pigtail at the
  breakout end, but a 3 m run needs a real cable.

## 7. Build order

1. Dry-fit `U1` into the grid once more in the final orientation: two rows of eight, 7 pitches
   apart (confirmed 4 September), USB overhanging the row-1 edge, row A (the LED side) on the
   column-4 side. Mark the two rows on the copper side before the socket strips go in.
2. Solder `S1`/`S2` socket strips, one pin at each end first, then re-check squareness.
3. Ground and 3.3 V distribution as bare wire on the copper side, staying out of the antenna
   keep-out. Continuity check: 3V3 to every supply pad, GND to every GND pad, **no** continuity
   between 3V3 and GND.
4. `R_PU_SDA`, `R_PU_SCL`, `C1`, then `J_SHT`, `J_VEML`, `C_SHT`, `C_VEML`, `TP_*`.
5. `R_OUT_SDA`, `R_OUT_SCL`, `D_CL1`–`D_CL4` (diode-test each), `C_OUT`, `J_OUT`.
6. `C_RAD_BULK` (polarity!), `C_RAD`, `J_RAD`.
7. `R_1W`, `J_1W`.
8. Unpowered: resistance SDA to 3V3 and SCL to 3V3 (≈ 2.2 kΩ with no modules plugged in, about
   1.8 kΩ with the VEML7700 plugged in, unchanged by the SHT41); every
   GPIO pad to 3V3 and to GND open circuit; `J_OUT` and `J_1W` poles to their nets.

## 8. Staged bring-up

1. **Controller alone**, nothing plugged in. USB power. `TP_3V3` ≈ 3.3 V; SDA and SCL idle-high;
   bus scan shows exactly `0x3C`. Serial hello with product, station id, variant. Wi-Fi joins at
   the limited TX power; note the RSSI.
2. **SHT41**, unpowered plug-in. Scan: `0x3C 0x44`. Plausible T/RH against Room.
3. **VEML7700.** Scan adds `0x10`. Dark and bright response.
4. **Radar.** Before plugging in: with the module powered from the bench at 3.3 V on its own,
   measure `OT2` with nobody in front (low) and a hand in front (high) — the manual says so, the
   meter confirms it. Then plug in; `presence` follows `OT2`. Sit still for five minutes: stays
   on. Leave the room for five minutes: goes off. Only then connect the UART pins in firmware.
5. **Probe.** After ringing out: `temperature_probe` plausible, responds to a hand.
6. **Outdoor cable**, on the bench first at full length, breakout in the room: scan adds `0x76`;
   rise-time capture; readings agree with the SHT41 within the story's tolerance. Then outside.
7. **Soak**: 24 hours, no reconnect gap over 5 minutes, outdoor trace follows the weather service.

Stop conditions: any voltage above 3.3 V on any pad (there is no source for one on this board,
so it means a wrong connection to USB 5 V), a warm part other than the SuperMini's LDO, a bus
scan that shows an address not in the list, or a probe or cable whose pin order is uncertain.

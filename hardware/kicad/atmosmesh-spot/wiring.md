# AtmosMesh Spot, first unit — bill of materials, layout and wiring

The first Spot is the **In/Out variant with the DS18B20 probe** (operator decision,
4 September 2026): SHT41 and VEML7700 on the board, an HLK-LD2410S radar for presence, a BME280
on a cable outside the window, and a temperature probe. Story:
[`SP-01`](../../../agent-context/stories/SP-01.md). Companion drawing:
[`atmosmesh-spot-layout.svg`](atmosmesh-spot-layout.svg).

**One 3.3 V domain.** The SuperMini's `5V` pin is not wired to anything. Every part below is
datasheet- or operator-confirmed to run from 3.3 V, and no signal on the board can exceed 3.3 V.
This document is a build contract, not power approval: the unpowered checks and the staged
bring-up at the end still apply.

Evidence level per part:

| Part | Pin order | Electrical | Source |
| --- | --- | --- | --- |
| HLK-LD2410S | **confirmed**, manual Table 3-2 | **confirmed**, 3.0–3.6 V, all pins 0–3.3 V | [`docs/hardware/datasheets/`](../../../docs/hardware/datasheets/README.md) |
| SHT41, VEML7700 | confirmed on the Room units; re-check the spares | 3.3 V | Room wiring |
| ESP32-C3 SuperMini OLED | type knowledge; **verify on the silkscreen** | 3.3 V logic, on-board LDO | seller pinout, board on the bench |
| BME280 breakout | type knowledge; **photograph** | 3.3 V; regulator and pull-ups to check | — |
| DS18B20 probe | cable colours are a claim; **ring out** | 3.3 V parasite or powered | Maxim datasheet (to file) |

## 1. Bill of materials

Everything except the cable is in stock. Reference designators match SP-01.

| Ref | Qty | Part | Value / type | From stock | Note |
| --- | ---: | --- | --- | --- | --- |
| `U1` | 1 | ESP32-C3 SuperMini with 0.42" OLED | — | 1 of 5 | The board on the bench |
| `U2` | 1 | SHT41 breakout | I²C `0x44` | 1 of 4 (last free one after Room and Room v2) | `J_SHT` |
| `U3` | 1 | VEML7700 breakout | I²C `0x10` | 1 of 3 | `J_VEML` |
| `U4` | 1 | HLK-LD2410S | 24 GHz presence radar | 1 of 1 | `J_RAD` |
| `U5` | 1 | BME280 breakout, 4-pin | I²C `0x76` | 1 of 6 | outdoor, on the cable |
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

Two rows of eight pins. Type knowledge for the common SuperMini footprint, USB at the top,
board's top side (OLED) facing up, read as the silkscreen shows:

| Left row (col 4), from USB | Right row (col 10), from USB |
| --- | --- |
| `5V` — **not wired** | `GPIO5` — SDA (verify) |
| `GND` | `GPIO6` — SCL (verify) |
| `3V3` — the rail | `GPIO7` |
| `GPIO4` — 1-Wire | `GPIO8` — on-board LED, strapping: unused |
| `GPIO3` — radar `OT2` | `GPIO9` — BOOT button, strapping: unused |
| `GPIO2` — strapping: unused | `GPIO10` |
| `GPIO1` — UART1 RX ← radar `OT1` | `GPIO20` — UART0 RX, boot log: unused |
| `GPIO0` — UART1 TX → radar `RX` | `GPIO21` — UART0 TX, boot log: unused |

> **Verify before soldering the socket strips:** (a) the pin names against the silkscreen on your
> board — if the rows are mirrored, mirror this table, never the net list; (b) the row spacing by
> dry-fitting the board into the grid: the layout assumes **6 pitches (15.24 mm)** between the
> rows; (c) the OLED's I²C pins by running the seller's example sketch or a bus scan on GPIO5/6.
> If the scan on GPIO5/6 does not show `0x3C`, the OLED is on other pins and the I²C net moves
> with it.

Firmware limits Wi-Fi TX power to about 8.5 dBm at start-up (ceramic antenna).

## 3. Board layout — 14 × 20 holes (35.6 × 50.8 mm)

Columns 1–14 left to right, rows 1–20 top to bottom, component side, USB toward row 1. Header and
terminal positions are exact; passives go in the row named for them, exact holes are the
builder's choice within that row.

```text
        col 1   2   3   4   5   6   7   8   9  10  11  12  13  14
       ┌────────────────────────────────────────────────────────────┐
row 1  │                   [ USB-C, U1 top edge ]                   │
row 2  │  ·   ·   ·  (5V)  ·   ·   ·   ·   ·  (G5)  ·   ·   ·   ·   │  ← U1 socket pins: col 4 and col 10
row 3  │  ·   ·   ·  (GND) ·   ·   ·   ·   ·  (G6)  ·   ·   ·   ·   │
row 4  │  ·   ·   ·  (3V3) ·   ·   ·   ·   ·  (G7)  ·   ·   ·   ·   │
row 5  │  ×   ×   ·  (G4)  ·   ·   ·   ·   ·  (G8)  ·   ×   ×   ×   │  × = antenna keep-out, rows 5–10
row 6  │  ×   ×   ·  (G3)  ·   ·   ·   ·   ·  (G9)  ·   ×   ×   ×   │      no copper, no wire, no coating
row 7  │  ×   ×   ·  (G2)  ·   ·   ·   ·   ·  (G10) ·   ×   ×   ×   │
row 8  │  ×   ×   ·  (G1)  ·   ·   ·   ·   ·  (G20) ·   ×   ×   ×   │
row 9  │  ×   ×   ·  (G0)  ·   ·   ·   ·   ·  (G21) ·   ×   ×   ×   │
row 10 │  ×   ×   ·   ·   ·   ·   ·   ·   ·   ·   ·   ×   ×   ×   │
row 11 │ [R_PU_SDA 2k2] [R_PU_SCL 2k2]    [C1 100n]    ·   ·   ·   │
row 12 │ [J_SHT: VIN GND SCL SDA] · [J_VEML: VIN 3Vo GND SCL SDA] [J_OUT VIN] │  J_OUT: 4-pole terminal, col 13,
row 13 │ [C_SHT]   ·   ·   ·   ·  [C_VEML]  ·   ·   ·   ·   ·   ·   │  rows 12/14/16/18 (5.08 mm), cable
row 14 │ TP_3V3 TP_GND · TP_SDA TP_SCL · [R_OUT_SDA 100R][R_OUT_SCL] [J_OUT GND] │  exits the right edge
row 15 │ [D_CL1][D_CL2][D_CL3][D_CL4]      [C_OUT 100n]  ·   ·   ·   │
row 16 │ [C_RAD_BULK 100µ] [C_RAD 100n]  ·   ·   ·   ·  [J_OUT SCL] │
row 17 │  ·   ·   ·   ·   ·   ·   ·   ·   ·   ·  [R_1W 4k7]  ·   ·  │
row 18 │  ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·  [J_OUT SDA] │
row 19 │ [J_RAD: 3V3 GND OT1 RX OT2]  · [J_1W: 3V3  ·  DQ  ·  GND]   │  J_1W: 3-pole terminal, cols 8/10/12
row 20 │        radar stands upright here, antenna face outward       │  (5.08 mm), probe cable exits bottom
       └────────────────────────────────────────────────────────────┘
```

Placement rules:

- **Antenna keep-out.** The SuperMini's ceramic antenna is at the end away from the USB socket.
  Columns 1–2 and 12–14 in rows 5–10 carry nothing: no wire on either side, no copper, and no
  conformal coating later. This is the single most important rule on this board.
- **Radar upright at the bottom edge**, antenna face (the side with the two large copper
  patches) pointing away from the board. On a windowsill unit lying flat that is the horizontal
  look into the room. The radar's 20 mm body spans about columns 1–8 in the air above row 19;
  `J_1W` sits at columns 8–12 below its edge, which clears it because the terminal is low.
- **SHT41 and VEML7700 stand upright on their headers** in row 12, in free air above the board.
  The SHT41 is on the far side from the SuperMini's LDO (the warm part) with the whole width of
  the board between them. If the light sensor must look up rather than sideways, put a
  right-angle female header at `J_VEML` and lay the module flat; nothing else changes.
- **Cable terminals at the edges** so the cables leave the board without crossing it: the
  outdoor cable off the right edge, the probe off the bottom edge.
- **If the screw terminals turn out to be 2.54 mm**, `J_OUT` takes col 13 rows 12–15 and `J_1W`
  cols 8–10 in row 19; nothing else moves.

## 4. Net list by pin name

```text
+3V3 rail (from U1 3V3, col 4 row 4):
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

GND rail (U1 GND, col 4 row 3):
  every GND above, D_CL2 anode, D_CL4 anode, TP_GND

SDA (U1 GPIO5, col 10 row 2):
  ├── J_SHT SDA
  ├── J_VEML SDA
  ├── TP_SDA
  └── R_OUT_SDA 100R ── node OUT_SDA ──┬── J_OUT SDA
                                       ├── D_CL1 anode  (cathode → +3V3)
                                       └── D_CL2 cathode (anode → GND)

SCL (U1 GPIO6, col 10 row 3):
  ├── J_SHT SCL
  ├── J_VEML SCL
  ├── TP_SCL
  └── R_OUT_SCL 100R ── node OUT_SCL ──┬── J_OUT SCL
                                       ├── D_CL3 anode  (cathode → +3V3)
                                       └── D_CL4 cathode (anode → GND)

U1 GPIO3 ── J_RAD OT2      presence in, high = occupied
U1 GPIO1 ── J_RAD OT1      radar UART TX → C3 UART1 RX
U1 GPIO0 ── J_RAD RX       C3 UART1 TX → radar UART RX
U1 GPIO4 ── J_1W DQ        1-Wire, with R_1W to +3V3
U1 5V    ── nothing
U1 GPIO2, 7, 8, 9, 10, 20, 21 ── nothing
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

Room's operator-confirmed order; hold the spare next to the Room unit and compare the silkscreen
before fitting. Address `0x44`, no address pin.

### 5.2 VEML7700 (`J_VEML`, 5 pins) — on board

```text
              J_VEML
 +3V3 ──────── 1  VIN
 NC   ──────── 2  3Vo     regulator output — never driven, never tied to 3V3
 GND  ──────── 3  GND
 SCL  ──────── 4  SCL
 SDA  ──────── 5  SDA
```

Address `0x10`. Face the sensor away from the OLED.

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

Mounting: the header pins are factory-fitted; measure their pitch before buying into the 2.54 mm
assumption. The antenna face looks away from the board.

### 5.4 BME280, outdoor (`J_OUT`, 4-pole screw terminal) — on the cable

```text
   board                                   cable, ≤ 3 m                    outdoor breakout
 +3V3 ──────── J_OUT 1  VIN ─────────── (pair A, wire 1) ─────────────── VIN / VCC
 GND  ──────── J_OUT 2  GND ─────────── (pair B, wire 1) ─────────────── GND
 OUT_SCL ───── J_OUT 3  SCL ─────────── (pair A, wire 2: twisted with VIN) ── SCL
 OUT_SDA ───── J_OUT 4  SDA ─────────── (pair B, wire 2: twisted with GND) ── SDA
```

Address `0x76` (SDO low, the breakout default; `0x77` if the photo shows SDO tied high — either
is fine, there is no collision). Photograph the breakout first: regulator fitted or not (feed
3.3 V regardless), pull-ups fitted (they add to the bus total), pin order. Put a 100 nF across
VIN/GND at the breakout end too if the breakout does not carry one.

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
- **Bus speed 50 kHz** on this variant (firmware flag). With 2.2 kΩ carrier pull-ups plus the
  breakouts' own, expect roughly 1.6 kΩ total; capture SDA at the board end on the DSO2D15 after
  the cable is at its final length and record the rise time (must be under 1 µs).
- **Outdoor end:** breakout in a vented shield under the sill, out of rain and sun, cable in a
  drip loop, no coating on the sensor.
- **Not in stock.** The JST-PH set has pre-crimped 24 AWG leads that make a tidy pigtail at the
  breakout end, but a 3 m run needs a real cable.

## 7. Build order

1. Dry-fit `U1` into the grid: two rows of eight, 6 pitches apart, pin names matching section 2.
   Stop if either is wrong.
2. Solder `S1`/`S2` socket strips, one pin at each end first, then re-check squareness.
3. Ground and 3.3 V distribution as bare wire on the copper side, staying out of the antenna
   keep-out. Continuity check: 3V3 to every supply pad, GND to every GND pad, **no** continuity
   between 3V3 and GND.
4. `R_PU_SDA`, `R_PU_SCL`, `C1`, then `J_SHT`, `J_VEML`, `C_SHT`, `C_VEML`, `TP_*`.
5. `R_OUT_SDA`, `R_OUT_SCL`, `D_CL1`–`D_CL4` (diode-test each), `C_OUT`, `J_OUT`.
6. `C_RAD_BULK` (polarity!), `C_RAD`, `J_RAD`.
7. `R_1W`, `J_1W`.
8. Unpowered: resistance SDA to 3V3 and SCL to 3V3 (≈ 2.2 kΩ with no modules plugged in); every
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

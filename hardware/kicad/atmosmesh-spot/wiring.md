# AtmosMesh Spot, first unit — bill of materials, layout and wiring

The first Spot is **SHT41 + VEML7700 + HLK-LD2410S on the board and a DS18B20 probe on a
terminal**. No BME280: the outdoor-cable variant drawn on 4 September was withdrawn by the operator
on 5 September, and with it the 4-pole terminal, the 100 Ω series resistors, the rail clamps and
the 2.2 kΩ pull-ups. Story: [`SP-01`](../../../agent-context/stories/SP-01.md). Companion drawings:
[`atmosmesh-spot-layout.svg`](atmosmesh-spot-layout.svg) (hole-by-hole placement) and
[`atmosmesh-spot-wiring.svg`](atmosmesh-spot-wiring.svg) (schematic-style net drawing, every
pin by its printed name).

**One 3.3 V domain.** The SuperMini's `5V` pin is not wired to anything. Every part below is
datasheet-, photo- or scan-confirmed to run from 3.3 V, and no signal on the board can exceed
3.3 V. This document is a build contract, not power approval: the unpowered checks and the staged
bring-up at the end still apply.

Evidence level per part:

| Part | Pin order | Electrical | Source |
| --- | --- | --- | --- |
| ESP32-C3 SuperMini OLED | **photo-confirmed** silkscreen; **OLED at 0x3C on GPIO5/GPIO6 by scan** | 3.3 V logic, on-board LDO | photo pass and `firmware/tools/c3scan`, 4 September |
| HLK-LD2410S | **confirmed**, manual Table 3-2 and silkscreen | **confirmed**, 3.0–3.6 V, all pins 0–3.3 V | [`docs/hardware/datasheets/`](../../../docs/hardware/datasheets/README.md) |
| SHT41, VEML7700 | **photo-confirmed** on the spares, 4 September | 3.3 V into their own LDO and level shifter | photo pass, inventory |
| DS18B20 probe | cable colours are a claim; **ring out** | 3.3 V, powered mode | Maxim datasheet (to file) |

The photos themselves were deleted after evaluation at the operator's request; the facts they
established are in `docs/hardware/inventory.md` under the 4 September photo pass.

## 1. Bill of materials

Everything is in stock. Reference designators match SP-01.

| Ref | Qty | Part | Value / type | From stock | Note |
| --- | ---: | --- | --- | --- | --- |
| `U1` | 1 | ESP32-C3 SuperMini with 0.42" OLED | rev 4, 4 MB, MAC `E4:EC:E6:92:05:D4` | 1 of 5 | The scanned board |
| `U2` | 1 | SHT41 breakout, `SHT4X` board | I²C `0x44` | 1 of 4 (last free one after Room and Room v2) | `J_SHT` |
| `U3` | 1 | VEML7700 breakout, `HW-900` | I²C `0x10` | 1 of 3 | `J_VEML` |
| `U4` | 1 | HLK-LD2410S | 24 GHz presence radar | 1 of 1 | `J_RAD` |
| `U5` | 1 | DS18B20 encapsulated probe, 2.5 m | 1-Wire | 1 of 2 | `J_1W` |
| `PCB` | 1 | perfboard 14 × 20 holes, 2.54 mm, double-sided (4 × 6 cm) | — | stock | edge pads on both short edges |
| `S1`, `S2` | 2 | female header strip, 8-pin | 2.54 mm | header stock | U1 socket rows; the board stays removable |
| `J_SHT` | 1 | female header, 4-pin | 2.54 mm | header stock | |
| `J_VEML` | 1 | female header, 5-pin | 2.54 mm | header stock | pin 2 (`3VO`) left unwired |
| `J_RAD` | 1 | female header, 5-pin | 2.54 mm (measure the radar's pins first) | header stock | radar stands upright |
| `J_1W` | 1 | PCB screw terminal, 3-pole | 5.08 mm assumed — measure | 1 of 10 | probe lead |
| `R_PU_SDA`, `R_PU_SCL` | 2 | resistor | 4.7 kΩ | E24 kit | bus stays on the board |
| `R_1W` | 1 | resistor | 4.7 kΩ | E24 kit | 1-Wire pull-up |
| `C1`, `C_SHT`, `C_VEML`, `C_RAD` | 4 | ceramic capacitor | 100 nF | ceramic assortment | one per supply pin |
| `C_RAD_BULK` | 1 | electrolytic, low ESR | 100 µF, ≥ 6.3 V | Elko assortment | radar transmit peaks (~118 mA) |
| `TP_3V3`, `TP_GND`, `TP_SDA`, `TP_SCL` | 4 | test-point pin | — | stock | |

Removed on 5 September with the BME280: `J_OUT`, `R_OUT_SDA`/`R_OUT_SCL`, `D_CL1`–`D_CL4`, `C_OUT`,
the cable and the shield.

Stock after this build: 4 SuperMinis, 1 SHT41 free (of 4: Room, Room v2, Spot), 1 VEML7700 free,
0 LD2410S, 1 DS18B20, all 6 BME280 unassigned.

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
| `RX` GPIO20 — UART0 RX ← radar `OT1` | `7` GPIO7 |
| `TX` GPIO21 — UART0 TX → radar `RX` | `6` GPIO6 — SCL (scan-confirmed: OLED at 0x3C) |
| `2` GPIO2 — strapping: unused | `5` GPIO5 — SDA (scan-confirmed) |
| `1` GPIO1 — ADC-capable, kept free | `4` GPIO4 — 1-Wire |
| `0` GPIO0 — ADC-capable, kept free | `3` GPIO3 — radar `OT2` |

> **Why the marked `RX`/`TX` pins carry the radar (changed 5 September).** On this board the
> USB console is the chip's native USB-serial, so UART0 on GPIO20/21 is a free hardware UART
> (`Serial0` in the Arduino core, `Serial` being USB). The Room v1 rule "never put a sensor on
> UART0" does not apply here: that board's UART0 *was* the console. The one side effect is the
> ROM boot log, which the ESP32-C3 prints on `TX` at 115200 baud at every reset and the radar
> therefore receives; it cannot form a radar command (those need the `FD FC FB FA` header and a
> valid length), so it is harmless. Using the labelled pins keeps GPIO0/GPIO1, two of the five
> ADC pins, free for a later analog sensor.

Beyond pin `0`/`3` the module continues for about two more pitches: `RST` and `BOOT` buttons and
a small red part with a white dot, presumed to be the antenna. That end is the keep-out.

> **Row spacing is settled:** the operator pushed the module into this perfboard on 4 September
> and it fits with three free columns on each side, so the rows are **7 pitches (17.78 mm)**
> apart at columns 4 and 11. **The OLED's I²C pins are settled too:** the scanner in
> `firmware/tools/c3scan`, run on this board on 4 September, found the OLED at `0x3C` on
> SDA = GPIO5, SCL = GPIO6 and nothing on any other pair. Nothing about `U1` is an assumption
> any more; only the row-A/row-B side when the USB faces row 1 is checked once at the dry-fit.

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
row 14 │ [R_PU_SDA 4k7] [R_PU_SCL 4k7]    [C1 100n]    ·   ·   ·   │
row 15 │ [J_SHT: VIN GND SCL SDA] · [J_VEML: VIN 3VO GND SCL SDA]   │  both modules standing upright
row 16 │ [C_SHT]   ·   ·   ·  [C_VEML]  ·   ·   ·   ·   ·   ·   ·   │
row 17 │ TP_3V3 TP_GND TP_SDA TP_SCL   ·   ·  [R_1W 4k7]   ·   ·    │
row 18 │ [C_RAD_BULK 100µ] [C_RAD 100n]  ·   ·   ·   ·   ·   ·   ·  │
row 19 │  ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   ·   │  wiring corridor
row 20 │ [J_RAD: 3V3 GND OT1 RX OT2]  · [J_1W: 3V3 · DQ · GND]  ·   │  J_1W: 3-pole terminal, cols 8/10/12
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
- **The probe terminal at the bottom edge** so the lead leaves the board without crossing it.
- **If the screw terminal turns out to be 2.54 mm**, `J_1W` takes cols 8–10 in row 20; nothing
  else moves.
- Columns 13–14 below row 13 are free; a future spare I²C header goes there if ever wanted.

## 4. Net list by pin name

```text
+3V3 rail (from U1 3V, col 4 row 5):
  ├── C1 100n ── GND                      beside the pin
  ├── R_PU_SDA 4k7 ── SDA
  ├── R_PU_SCL 4k7 ── SCL
  ├── J_SHT  VIN ──┤C_SHT 100n── GND
  ├── J_VEML VIN ──┤C_VEML 100n── GND     (J_VEML 3VO: NOT CONNECTED)
  ├── J_RAD  3V3 ──┬┤C_RAD_BULK 100µ (+)── GND
  │                └┤C_RAD 100n── GND
  ├── J_1W   3V3 ── R_1W 4k7 ── J_1W DQ ── U1 GPIO4
  └── TP_3V3

GND rail (U1 GD, col 4 row 4):
  every GND above, TP_GND

SDA (U1 GPIO5, col 11 row 8):  J_SHT SDA, J_VEML SDA, TP_SDA
SCL (U1 GPIO6, col 11 row 7):  J_SHT SCL, J_VEML SCL, TP_SCL

U1 GPIO3 (col 11 row 10) ── J_RAD OT2      presence in, high = occupied
U1 RX = GPIO20 (col 4 row 6) ── J_RAD OT1  radar UART TX → C3 UART0 RX
U1 TX = GPIO21 (col 4 row 7) ── J_RAD RX   C3 UART0 TX → radar UART RX
U1 GPIO4 (col 11 row 9)  ── J_1W DQ        1-Wire, with R_1W to +3V3
U1 5V    ── nothing
U1 GPIO0, 1, 2, 7, 8, 9, 10 ── nothing
```

Pull-up sum per line: carrier 4.7 kΩ in parallel with the VEML7700's 10 kΩ gives about 3.2 kΩ;
with an assumed 10 kΩ on the OLED about 2.4 kΩ, 1.4 mA sink at 3.3 V. The SHT41 board adds
nothing (its host-side pads are empty). Meter check, unpowered: VEML7700 `SDA` to `VIN` about
10 kΩ, SHT41 the same pair open, SuperMini `5` to `3V` for the OLED's own pull-up.

## 5. Connection diagrams per module

### 5.1 SHT41 (`J_SHT`, 4 pins)

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

### 5.2 VEML7700 (`J_VEML`, 5 pins)

```text
              J_VEML
 +3V3 ──────── 1  VIN
 NC   ──────── 2  3VO     regulator output — never driven, never tied to 3V3
 GND  ──────── 3  GND
 SCL  ──────── 4  SCL
 SDA  ──────── 5  SDA
```

Photo-confirmed on the spare (`HW-900` board, "Vin/logic 3-5V"): `VIN 3VO GND SCL SDA`. Address
`0x10`. Same regulator-plus-level-shifter architecture as the SHT41 board, but with all four
elements of its `103` array in use, so it presents about 10 kΩ per line to the bus. Face the
sensor away from the OLED.

### 5.3 HLK-LD2410S (`J_RAD`, 5 pins) — manual Table 3-2

```text
              J_RAD (J2 on the module, order as printed: 3V3 GND OT1 RX OT2)
 +3V3 ──────── 1  3V3     3.0–3.6 V; C_RAD_BULK 100 µF + C_RAD 100 nF right here
 GND  ──────── 2  GND
 RX (GPIO20) ── 3  OT1     the module's UART TX (0–3.3 V) → C3 UART0 RX
 TX (GPIO21) ── 4  RX      the module's UART RX (0–3.3 V) ← C3 UART0 TX
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

### 5.4 DS18B20 probe (`J_1W`, 3-pole screw terminal)

```text
 +3V3 ──┬───── J_1W 1  3V3 ────── probe VDD
        R_1W 4k7
 GPIO4 ─┴───── J_1W 2  DQ  ────── probe DATA
 GND  ──────── J_1W 3  GND ────── probe GND
```

Powered mode (VDD wired), not parasite. **Ring the three wires out** against the probe's own
datasheet pin order at the sensor end with a continuity meter before trusting any colour; on the
encapsulated probes in stock the colours are a product claim. The 2.5 m lead is inside the
1-Wire limits with a 4.7 kΩ pull-up, and it is the only thing that may leave the room: held out
of a window it gives an outside temperature with no electronics outside.

### 5.5 Test points

`TP_3V3`, `TP_GND`, `TP_SDA`, `TP_SCL` in row 17. The unpowered checks use them; label them on
the board.

## 6. Build order

1. Dry-fit `U1` into the grid once more in the final orientation: two rows of eight, 7 pitches
   apart (confirmed 4 September), USB overhanging the row-1 edge, row A (the LED side) on the
   column-4 side. Mark the two rows on the copper side before the socket strips go in.
2. Solder `S1`/`S2` socket strips: push the module in as an alignment jig, tack one pin at each
   end of each strip, pull the module out, finish the joints, re-check squareness.
3. Ground and 3.3 V distribution as bare wire on the copper side, staying out of the antenna
   keep-out. Continuity check: 3V3 to every supply pad, GND to every GND pad, **no** continuity
   between 3V3 and GND.
4. `R_PU_SDA`, `R_PU_SCL`, `C1`, then `J_SHT`, `J_VEML`, `C_SHT`, `C_VEML`, `TP_*`.
5. `C_RAD_BULK` (polarity!), `C_RAD`, `J_RAD`.
6. `R_1W`, `J_1W`.
7. Unpowered: resistance SDA to 3V3 and SCL to 3V3 (≈ 4.7 kΩ with no modules plugged in, about
   3.2 kΩ with the VEML7700 plugged in, unchanged by the SHT41); every GPIO pad to 3V3 and to
   GND open circuit; `J_1W` poles to their nets.

## 7. Staged bring-up

1. **Controller alone**, nothing plugged in. USB power. `TP_3V3` ≈ 3.3 V; SDA and SCL idle-high;
   bus scan shows exactly `0x3C` (`firmware/tools/c3scan` does this). Serial hello with product
   and station id once the product firmware exists. Wi-Fi joins at the limited TX power; note
   the RSSI.
2. **SHT41**, unpowered plug-in. Scan: `0x3C 0x44`. Plausible T/RH against Room.
3. **VEML7700.** Scan adds `0x10`. Dark and bright response.
4. **Radar.** Before plugging in: with the module powered from the bench at 3.3 V on its own,
   measure `OT2` with nobody in front (low) and a hand in front (high) — the manual says so, the
   meter confirms it. Then plug in; `presence` follows `OT2`. Sit still for five minutes: stays
   on. Leave the room for five minutes: goes off. Only then connect the UART pins in firmware.
5. **Probe.** After ringing out: `temperature_probe` plausible, responds to a hand.
6. **Soak**: 24 hours, no reconnect gap over 5 minutes.

Stop conditions: any voltage above 3.3 V on any pad (there is no source for one on this board,
so it means a wrong connection to USB 5 V), a warm part other than the SuperMini's LDO, a bus
scan that shows an address not in the list, or a probe lead whose pin order is uncertain.

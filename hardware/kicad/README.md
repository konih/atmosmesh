# AtmosMesh bench station — KiCad PCB

A 2-layer, all-through-hole carrier board for the **bench** revision: an ESP32-WROOM-32 DevKit sits
in two 1×15 sockets, the three 3.3 V sensors get proper connectors, and the future 5 V sensors get
connectors in a visually and electrically separated block. **No mains on this board** — the AC/DC
module stays off-board and enclosed ([`docs/hardware/power.md`](../../docs/hardware/power.md)).

Board outline is **139 × 58 mm**, 1.6 mm, 2 copper layers, GND poured on both.

## Files

| Path | What it is |
| --- | --- |
| `atmosmesh-bench/atmosmesh-bench.kicad_pro` | Project file — open this one |
| `atmosmesh-bench/atmosmesh-bench.kicad_sch` | Single-sheet schematic (A3) with named nets |
| `atmosmesh-bench/atmosmesh-bench.kicad_pcb` | Board: placement, netlist, outline, keepout, GND pours |
| `atmosmesh-bench/atmosmesh.kicad_sym` | Project symbol library (all symbols, vendored) |
| `atmosmesh-bench/atmosmesh.pretty/` | Project footprint library (all footprints, vendored) |
| `atmosmesh-bench/sym-lib-table`, `fp-lib-table` | Point the project at those two libraries |
| `atmosmesh-bench/atmosmesh-bench-schematic.pdf` | Rendered schematic, for review without KiCad |

## Opening it

Written in **KiCad 9 file format** (`.kicad_sch` 20250114, `.kicad_pcb` 20241229,
`.kicad_sym` 20241209) and validated with **KiCad 10.0.5**. Opens in KiCad 9 and 10.

```sh
open -a KiCad hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_pro
```

Every symbol and footprint is **vendored into the project** (`atmosmesh:*`), so nothing depends on a
global library table — the project opens, ERCs and DRCs identically on any machine. The geometry of
the standard parts is copied verbatim from the KiCad 10.0.5 stock libraries; only the ESP32 DevKit
socket is original.

## Authoritative pin map (operator, 2026-08-14)

This is the **rewired bench** map. It supersedes every earlier D2/GPIO18 arrangement.

| Function | ESP32 pin | Net | Socket pin | Connector |
| --- | --- | --- | --- | --- |
| OLED 0x3C I²C **SDA** (D-001) | **GPIO5** (D5) | `SDA_LCD` | U1.23 | J1.3 |
| OLED 0x3C I²C **SCL** (D-001) | **GPIO4** (D4) | `SCL_LCD` | U1.26 | J1.4 |
| OLED VCC / GND | — | `+3V3` / `GND` | U1.30 / U1.29 | J1.2 / J1.1 |
| BMP280 **SDA** | **GPIO21** | `SDA_SENS` | U1.20 | J2.4 |
| BMP280 **SCL** | **GPIO19** (D19) | `SCL_SENS` | U1.21 | J2.3 |
| BMP280 **CSB** → 3V3 (I²C mode) | — | `+3V3` | — | J2.5 |
| BMP280 **SDO** → GND (addr **0x76**) | — | `GND` | — | J2.6 |
| BMP280 VCC | — | `+3V3` | U1.30 | J2.1 |
| AM2302 / DHT22 **DATA** | **GPIO18** (D18) | `DHT_DATA` | U1.22 | J3.2 |
| AM2302 VDD / GND | — | `+3V3` / `GND` | — | J3.1 / J3.3 |
| SDS011 TXD → ESP32 RX2 | GPIO16 | `PM_TX` | U1.25 | J4.3 |
| SDS011 RXD ← ESP32 TX2 | GPIO17 | `PM_RX` | U1.24 | J4.4 |
| MQ135 analog out | — | `MQ_AOUT` | — | J5.3 → R2 |
| MQ135 divider output | GPIO34 (ADC1, input-only) | `MQ_ADC` | U1.4 | R2/R3 |
| DevKit `VIN`, through JP1 | — | `ESP_VIN` | U1.15 | JP1.2 |
| Beeper **SIG** | **GPIO25** | `BEEP_SIG` | U1.8 | J_BEEP.3 |
| Beeper VCC (3V3 default, JP_BEEP → +5V) | — | `BEEP_VCC` | — | J_BEEP.1 / JP_BEEP.2 |
| PIR D-SUN **SIG** (digital in, **not 27**) | **GPIO33** | `PIR_OUT` | U1.7 | J_PIR.3 |
| PIR VCC (3V3 default, JP_PIR → +5V for HC-SR501) | — | `PIR_VCC` | — | J_PIR.1 / JP_PIR.2 |
| VEML7700 lux **SCL** (shared sensor bus) | **GPIO19** | `SCL_SENS` | U1.21 | J_VEML.3 |
| VEML7700 lux **SDA** (shared sensor bus, addr 0x10) | **GPIO21** | `SDA_SENS` | U1.20 | J_VEML.4 |
| VEML7700 VCC / GND (3V3 only) | — | `+3V3` / `GND` | — | J_VEML.1 / J_VEML.2 |
| **GPIO2** | **GPIO2** | *no-connect* | U1.27 | — |

`R1` is the **10 kΩ pull-up from `DHT_DATA` to `+3V3`**. `R4`/`R5` are optional 4k7 pull-ups on the
sensor I²C bus, fitted as **DNP** because the GY-BMP280 breakout already carries pull-ups.

### Named nets

`+3V3` `+5V` `GND` `ESP_VIN` · `SDA_LCD` `SCL_LCD` · `SDA_SENS` `SCL_SENS` · `DHT_DATA` ·
`PM_TX` `PM_RX` · `MQ_AOUT` `MQ_ADC` · `LED_A` ·
`BEEP_VCC` `BEEP_SIG` · `PIR_VCC` `PIR_OUT`

Signals use **global labels**, so the board net names match the sheet exactly (no `/` prefix).
Deliberately unused pins carry no-connect flags and therefore appear as KiCad's
`unconnected-(…)` nets on the board — that is intended, not an omission.

### J7 — 3.3 V flying-lead harness

An alternative to the socket: bring the DevKit in on jumper wires if its pin order turns out to
differ from the assumption below. **Populate U1 or J7, never both.**

| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| +3V3 | GND | GPIO4 | GPIO5 | GPIO18 | GPIO19 | GPIO21 | GPIO16 | GPIO17 | GPIO34 | GND | +3V3 |

### Extras strip — J_BEEP / J_PIR / J_VEML (2026-08-14)

Live extras from the breadboard (`agent-context/extra-peripherals.md`), placed in their own silk
box at the right board edge. `J_BEEP` and `J_PIR` are **1×3, pin order VCC / GND / SIG**; SIG is
always 3.3 V. `J_BEEP` (silk `BEEPER`, SIG=GPIO25) and `J_PIR` (silk `D-SUN PIR`, SIG=GPIO33 —
**not 27**) get a 3-pin VCC-select jumper (`JP_BEEP` / `JP_PIR`: 1=+3V3 default, 2=VCC out,
3=+5V — 5 V position only for 5 V-powered modules). `J_VEML` (silk `VEML7700 LUX`) is a 1×4
VCC/GND/SCL/SDA header on the **shared sensor I²C bus** (GPIO21/GPIO19, same bus as the BMP280;
VEML7700 addr 0x10, BMP280 0x76 — no clash), 3V3-only. The HC-20/DC-20 mic was removed from the
bench (2026-08-14); GPIO22 and GPIO35 are free. **No J_TFT** — the colour TFT was dropped.

## Electrical rules built into the board

1. **No 5 V on any ESP32 GPIO or on `+3V3`.** The 5 V nets touch only J6 (input), J4/J5 (future
   sensors), C1/C4, R6/D1, JP1, and pin 3 of the VCC-select jumpers JP_BEEP / JP_PIR. **Common GND
   only** — the two domains share nothing else. The jumpers feed module **power** only; SIG pins
   are always 3.3 V.
2. `+3V3` is an **output** of the DevKit's onboard LDO (U1.30). Nothing back-feeds it; the 3.3 V AC/DC
   spare must not be paralleled onto it.
3. **`JP1` stays OPEN on the bench.** It is the only path from the 5 V rail to the DevKit `VIN`.
   Close it only after the AC/DC module has been measured and enclosed, and **never together with
   USB** — USB/VIN coexistence on this board is unverified.
4. The 5 V block is fenced off on the silkscreen (`5V DOMAIN - NEVER TO A GPIO`) and sits in the
   bottom-right corner, physically away from the 3.3 V connectors.
5. `MQ135` on the schematic still shows `R2`/`R3` as 10 k / 15 k (3.0 V at 5.0 V in). The **live
   bench** (2026-08-14) is **10 kΩ series + 20 kΩ to GND** → 3.33 V at 5.0 V AOUT, **no GPIO
   headroom**. Firmware logs that warning. Never put 5 V on GPIO34, and never label MQ135 as CO₂.

### GPIO2

**GPIO2 is left free and unconnected** — it is a download strap and must stay unloaded so USB
flashing works without unplugging anything. There is no display jumper any more: the display (OLED,
D-001) moved to GPIO5/GPIO4, so the old "unplug to flash" workaround is gone. GPIO5 is also a
strap, but the module's I²C pull-up keeps it idle-high, which is the correct boot level.

## Before you fabricate: verify the DevKit pin order

The DevKit's silkscreen has **not been photographed yet**
([`docs/hardware/inventory.md`](../../docs/hardware/inventory.md) still lists the board name and pin
labels as unconfirmed). `U1` assumes the common DOIT-style 30-pin order, USB at the `+Y` end:

- **Left row, pins 1–15:** EN, GPIO36/VP, GPIO39/VN, GPIO34, GPIO35, GPIO32, GPIO33, GPIO25, GPIO26,
  GPIO27, GPIO14, GPIO12, GPIO13, GND, VIN
- **Right row, pins 16–30:** GPIO23, GPIO22, GPIO1/TX0, GPIO3/RX0, GPIO21, GPIO19, GPIO18, GPIO5,
  GPIO17/TX2, GPIO16/RX2, GPIO4, GPIO2, GPIO15, GND, 3V3

The board also carries `VERIFY DEVKIT PINOUT / GPIO2 FREE` on the front silkscreen. If the physical
board differs, fix `U1`'s pin names in `atmosmesh.kicad_sym` (the footprint pads are positional, so
only the symbol mapping changes) — or ignore the socket and wire through **J7**. Two row spacings are
provided: `ESP32_DevKit_1x15x2_P25.40mm` (placed) and `ESP32_DevKit_1x15x2_P22.86mm` (0.9 in
variants).

## State of the board — placed AND routed

The board is **fully routed**: all signal and power nets are on copper, GND is carried by the
poured zones on `F.Cu` and `B.Cu` (fills are saved in the file). Routing was generated
programmatically (grid router, 0.25/0.32 mm clearance, 0.4 mm signals / 0.6 mm power per the
netclasses) and verified with KiCad DRC: **0 violations at all severities, 0 unconnected items,
0 schematic parity issues**. 27 vias total; B.Cu carries short crossings only, so the ground
plane stays largely intact.

- Edge cuts, four M3 mounting holes, and a `User.Drawings` keepout marking the DevKit body plus
  the USB cable exit (the module deliberately overhangs the bottom edge so the cable clears the
  PCB).
- Every footprint carries a `path` link to its schematic symbol, so **Update PCB from
  Schematic** matches existing footprints instead of re-adding them.
- Review the routes in pcbnew before fabricating — they are machine-generated; eyeball the
  5 V / 3V3 domain boundaries and re-run DRC after any manual edit.

## Validation with kicad-cli

The macOS app bundle does **not** put `kicad-cli` on `PATH`. Use the absolute path:

```sh
export KCLI=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
cd hardware/kicad/atmosmesh-bench

# version
"$KCLI" version

# Electrical rules check, all severities, non-zero exit if anything is found
"$KCLI" sch erc --severity-all --exit-code-violations \
  -o atmosmesh-bench-erc.rpt atmosmesh-bench.kicad_sch

# Design rules check, including schematic/PCB parity, with zones refilled in memory
"$KCLI" pcb drc --severity-all --schematic-parity --refill-zones \
  -o atmosmesh-bench-drc.rpt atmosmesh-bench.kicad_pcb

# Netlist and a reviewable schematic PDF
"$KCLI" sch export netlist -o atmosmesh-bench.net atmosmesh-bench.kicad_sch
"$KCLI" sch export pdf -o atmosmesh-bench-schematic.pdf atmosmesh-bench.kicad_sch
```

The PCB file is KiCad 10 format (the schematic still opens in KiCad 9 and 10). Zone fills are
saved in the file; refill with `B` in pcbnew after moving anything.

## Fabrication output — gerbers **and drill**

A gerber set without drill files is a board with **no holes**: JLCPCB's viewer shows a bare outline
and the order is rejected. `pcb export gerbers` never emits drill data — `pcb export drill` is a
**separate command** and is the step that is easy to forget. Always run both, and always upload the
zip that this recipe builds rather than hand-picking files.

```sh
export KCLI=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
cd hardware/kicad/atmosmesh-bench
rm -rf gerbers && mkdir gerbers

# 1. Copper / mask / silk / paste / outline. Protel extensions are what JLCPCB expects.
"$KCLI" pcb export gerbers --check-zones \
  -l F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
  -o gerbers atmosmesh-bench.kicad_pcb

# 2. Excellon drill — separate PTH/NPTH, plus a drill map and a hit report to review.
"$KCLI" pcb export drill --format excellon --drill-origin absolute \
  --excellon-units mm --excellon-zeros-format decimal --excellon-separate-th \
  --generate-map --map-format pdf \
  --generate-report --report-path gerbers/atmosmesh-bench-drill-report.rpt \
  -o gerbers atmosmesh-bench.kicad_pcb

# 3. One zip — this is the file to upload.
(cd gerbers && zip -j atmosmesh-bench-jlcpcb.zip \
  *.gtl *.gbl *.gts *.gbs *.gto *.gbo *.gtp *.gbp *.gm1 *.drl *.gbrjob)
```

Pass **no origin flags to either command**. Both default to the absolute page origin, so leaving
them alone is what keeps holes aligned to pads; `--use-drill-file-origin` on the gerber side would
plot copper against a different datum and produce the classic "holes offset from pads" rejection.

`gerbers/` is gitignored — it is regenerated output, not source.

### Verify before uploading

Existence of a `.drl` is not enough. Check all three:

1. **Hole counts** — read `atmosmesh-bench-drill-report.rpt`. Expect **132 plated** holes
   (including 27 × 0.4 mm, the vias) and **4 unplated** at **3.2 mm** — the M3 mounting holes. A
   missing NPTH file passes a naive existence check and still ships a board you cannot bolt down.
2. **Alignment** — the drill coordinates must sit inside the `Edge_Cuts` outline. Edge cuts span
   X `100.0 … 239.0`, Y `-118.0 … -60.0` mm; the four M3 holes sit at X `104.0`/`235.0`,
   Y `-64.0`/`-114.0` — 4 mm in from each edge. Drill near zero while the outline sits at an
   offset (or vice versa) means the origins diverged — see the flag warning above.
3. **File count** — the zip must hold **12** files: 9 gerbers, `PTH.drl`, `NPTH.drl`, `.gbrjob`.

Note that `.gbrjob` lists only the gerber layers; it never references the drill files, so it cannot
be used to confirm step 1.

### Results — KiCad 10.0.5, 2026-08-14

| Check | Result |
| --- | --- |
| `sch erc --severity-all` | **0 violations** (0 errors, 0 warnings) |
| `pcb drc --severity-all` | **0 violations** |
| Netlist ↔ PCB pad diff | **104 pins, 0 mismatches** |
| `pcb drc --schematic-parity` | **0 parity issues** |
| Unconnected items | **0** — fully routed |

## Follow-up: firmware matches — except the MQ135 divider constant

[`firmware/include/atmosmesh/pins.hpp`](../../firmware/include/atmosmesh/pins.hpp) now matches this
board's GPIO map (`kOledSdaGpio = 5`, `kOledSclGpio = 4`, `kSensorSclGpio = 19`,
`kAm2302DataGpio = 18`, SDS011 on GPIO16/17, MQ135 on GPIO34). J1 is the **OLED** header
(D-001); the historical `SDA_LCD`/`SCL_LCD` net names were kept to avoid churning the netlist.

One deliberate divergence remains: **R3 is 15 kΩ on this board** (10 k/15 k divider → 3.0 V max on
GPIO34), while firmware `kMq135GndOhms = 20000` matches the **live breadboard's** 10 k/20 k
divider (3.33 V at 5 V AOUT — no headroom). When this carrier board replaces the breadboard, set
`kMq135GndOhms = 15000` in the same change — see `agent-context/INBOX.md`.

## Provenance

Standard part geometry is copied from the KiCad 10.0.5 stock libraries (`Connector_Generic`,
`Connector`, `Device`, `Mechanical`, `power`, `Connector_PinHeader_2.54mm`, `Resistor_THT`,
`Capacitor_THT`, `LED_THT`, `TerminalBlock_Phoenix`, `MountingHole`) into the project libraries, with
3D-model references dropped. The ESP32 DevKit socket symbol and its two footprints are original —
KiCad ships no 30-pin ESP32 DevKit part. The initial geometry was laid out programmatically; from
here on **the KiCad files are the source of truth** and should be edited in KiCad.

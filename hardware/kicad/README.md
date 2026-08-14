# AtmosMesh bench station — KiCad PCB

A 2-layer, all-through-hole carrier board for the **bench** revision: an ESP32-WROOM-32 DevKit sits
in two 1×15 sockets, the three 3.3 V sensors get proper connectors, and the future 5 V sensors get
connectors in a visually and electrically separated block. **No mains on this board** — the AC/DC
module stays off-board and enclosed ([`docs/hardware/power.md`](../../docs/hardware/power.md)).

Board outline is **115 × 58 mm**, 1.6 mm, 2 copper layers, GND poured on both.

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
| LCD 1602 I²C **SDA** | **GPIO5** (D5) | `SDA_LCD` | U1.23 | J1.3 |
| LCD 1602 I²C **SCL** | **GPIO4** (D4) | `SCL_LCD` | U1.26 | J1.4 |
| LCD VCC / GND | — | `+3V3` / `GND` | U1.30 / U1.29 | J1.2 / J1.1 |
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
| **GPIO2** | **GPIO2** | *no-connect* | U1.27 | — |

`R1` is the **10 kΩ pull-up from `DHT_DATA` to `+3V3`**. `R4`/`R5` are optional 4k7 pull-ups on the
sensor I²C bus, fitted as **DNP** because the GY-BMP280 breakout already carries pull-ups.

### Named nets

`+3V3` `+5V` `GND` `ESP_VIN` · `SDA_LCD` `SCL_LCD` · `SDA_SENS` `SCL_SENS` · `DHT_DATA` ·
`PM_TX` `PM_RX` · `MQ_AOUT` `MQ_ADC` · `LED_A`

Signals use **global labels**, so the board net names match the sheet exactly (no `/` prefix).
Deliberately unused pins carry no-connect flags and therefore appear as KiCad's
`unconnected-(…)` nets on the board — that is intended, not an omission.

### J7 — 3.3 V flying-lead harness

An alternative to the socket: bring the DevKit in on jumper wires if its pin order turns out to
differ from the assumption below. **Populate U1 or J7, never both.**

| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| +3V3 | GND | GPIO4 | GPIO5 | GPIO18 | GPIO19 | GPIO21 | GPIO16 | GPIO17 | GPIO34 | GND | +3V3 |

## Electrical rules built into the board

1. **No 5 V on any ESP32 GPIO or on `+3V3`.** The 5 V nets touch only J6 (input), J4/J5 (future
   sensors), C1/C4, R6/D1 and JP1. **Common GND only** — the two domains share nothing else.
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
flashing works without unplugging anything. There is no LCD jumper any more: the LCD moved to
GPIO5/GPIO4, so the old "unplug the LCD to flash" workaround is gone. GPIO5 is also a strap, but the
LCD backpack's I²C pull-up keeps it idle-high, which is the correct boot level.

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

## State of the board — placement, not routing

The board is **placed, netlisted, outlined and poured; the signal traces are not routed.**

- Edge cuts, four M3 mounting holes, and a `User.Drawings` keepout marking the DevKit body plus the
  USB cable exit (the module deliberately overhangs the bottom edge so the cable clears the PCB).
- GND is poured on `F.Cu` and `B.Cu`, so every GND pad is already connected.
- The remaining 13 nets show as **40 unconnected items** in DRC — that is the ratsnest waiting to be
  routed in Pcbnew, not a rule violation.
- Netclasses are set up: `Default` 0.30 mm tracks / 0.25 mm clearance, `Power` (`GND`, `+3V3`, `+5V`,
  `ESP_VIN`) 0.60 mm.

Routing was left to the operator on purpose: with only one free signal layer above the ground pour,
trace assignment is a hand-editing job, and it is cheaper to do it once the DevKit pin order is
confirmed than to redo it afterwards.

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

Do **not** add `--save-board` to the DRC command: it rewrites `.kicad_pcb` in KiCad 10 format and
drops this project's KiCad 9 compatibility. Zones are refilled on load anyway.

### Results — KiCad 10.0.5, 2026-08-14

| Check | Result |
| --- | --- |
| `sch erc --severity-all` | **0 violations** (0 errors, 0 warnings) |
| `pcb drc --severity-all` | **0 violations** |
| `pcb drc --schematic-parity` | **0 parity issues** |
| Unconnected items | **40** — the unrouted signal nets, see above |

## Follow-up: firmware still carries the old pin map

[`firmware/include/atmosmesh/pins.hpp`](../../firmware/include/atmosmesh/pins.hpp) has **not** been
updated to the rewired bench and currently contradicts this board. It needs:

| Constant | In firmware now | Must become |
| --- | --- | --- |
| `kLcdSdaGpio` | `2` | **`5`** |
| `kLcdSclGpio` | `4` | `4` (unchanged) |
| `kSensorSdaGpio` | `21` | `21` (unchanged) |
| `kSensorSclGpio` | `18` | **`19`** |
| `kAm2302DataGpio` | `5` | **`18`** |

The comment on the LCD constants ("unplug LCD to flash") is also obsolete now that GPIO2 is free.

## Provenance

Standard part geometry is copied from the KiCad 10.0.5 stock libraries (`Connector_Generic`,
`Connector`, `Device`, `Mechanical`, `power`, `Connector_PinHeader_2.54mm`, `Resistor_THT`,
`Capacitor_THT`, `LED_THT`, `TerminalBlock_Phoenix`, `MountingHole`) into the project libraries, with
3D-model references dropped. The ESP32 DevKit socket symbol and its two footprints are original —
KiCad ships no 30-pin ESP32 DevKit part. The initial geometry was laid out programmatically; from
here on **the KiCad files are the source of truth** and should be edited in KiCad.

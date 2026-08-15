# AtmosMesh bench — schematic/PCB fix list (other-session)

> ## ⚠️ VERIFIED INVALID in the PCB session (2026-08-14, later) — do not apply §1–§4
>
> The geometric premise of the blocking fixes is wrong. KiCad **symbol libraries store pin
> offsets y-up, while the schematic sheet is y-down**: a placed pin sits at `anchor_y − rel_y`,
> not `anchor_y + rel_y`. Re-deriving every attachment with the correct convention shows the
> labels this review calls "off-pin" or "reversed" sit **exactly on the intended pins** (e.g.
> `MQ_ADC` at y=119.38 = 129.54 − 10.16 = **U1.4 GPIO34**, not U1.12).
>
> Evidence, current files:
> - `kicad-cli sch erc --severity-error --severity-warning`: **0 errors, 0 warnings** on the
>   untouched sheet. (Applying §1's moves produces 6 errors/6 warnings — tested, then reverted.)
> - Netlist export diffed pin-by-pin against PCB pad nets: **89 pins, 0 mismatches** — schematic
>   and board already agree completely, including U1, J1–J7, JP1 and all passives.
> - Consequence: `Update PCB from Schematic` would be a no-op net-wise; the §"Hard rule" hazard
>   does not exist. Routing can proceed whenever the DevKit pinout photo lands.
>
> What was real and is now **done**: §2/§6 J1 identity — Value `LCD1602_I2C` → `OLED_I2C` in
> schematic and PCB, silkscreen `LCD1602` → `OLED 0x3C` (module SDA/SCL order still to be
> photo-verified before copper). §5 R3 divider: real divergence, decision logged in `INBOX.md`
> (board stays 10 k/15 k; firmware switches to 15000 when the carrier board replaces the
> breadboard — not before, the live breadboard is physically 10 k/20 k). The stale
> `hardware/kicad/README.md` firmware paragraph was rewritten.

**Date:** 2026-08-14 (re-read of current files; PCB has grown / been re-laid-out, still unrouted).
**Do not fabricate. Do not flash. Do not merge or commit KiCad.** This file is the work order.

**Files (read-only until the PCB session edits them in KiCad):**

| Path | Role |
| --- | --- |
| `hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_sch` | Single sheet; KiCad 9 sch format (`20250114`) |
| `hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_pcb` | Placement + net names; **KiCad 10** (`20260206`, pcbnew 10.0). Title block: signals intentionally unrouted. No `segment`/`via` traces. |
| `hardware/kicad/atmosmesh-bench/atmosmesh.kicad_sym` | Vendored symbols, including `ESP32_DevKit_V1_Socket` |
| `firmware/include/atmosmesh/pins.hpp` | Authoritative GPIO map (main). `.worktrees/atmosmesh-oled-u8g2` GPIO numbers **match**; only the OLED driver comment differs. |

**Live bench truth (operator 2026-08-14) — firmware already matches this:**

| Function | Wiring | Nets (intended) |
| --- | --- | --- |
| Mini OLED 0x3C | SDA=GPIO5, SCL=GPIO4, VCC=3V3 | `SDA_LCD`, `SCL_LCD`, `+3V3` |
| GY-BMP280 | SDA=21, SCL=19, CSB=3V3, SDO=GND | `SDA_SENS`, `SCL_SENS` |
| AM2302 | GPIO18 | `DHT_DATA` |
| SDS011 | sensor TX→GPIO16, GPIO17→sensor RX, **5 V on sensor VCC only** | `PM_TX`, `PM_RX`, `+5V` |
| MQ135 | AOUT 10 kΩ series to GPIO34, 20 kΩ to GND (firmware `10000`/`20000`) | `MQ_AOUT`, `MQ_ADC` |
| GPIO2 | free (download strap) | no-connect on U1.27 |
| UART0 | USB/CP2102 only | U1.18 TX0 / U1.19 RX0 no-connect |
| 5 V | never on a GPIO | 5 V block only: J6, J4, J5, C1/C4, R6/D1, JP1 |

`hardware/kicad/README.md` still says firmware carries the *old* LCD/GPIO18 map. That paragraph is **stale**. `pins.hpp` on main is the live map.

---

## Hard rule — Do **not** Update PCB from Schematic

Until every U1 (and J1–J7) **symbol pin number** carries the net that the **pad of the same number** should have, **Tools → Update PCB from Schematic** will smash the board.

Today the **PCB pad nets follow the intended pinout** (README / silkscreen). The **schematic wires follow geometry as if pin 1 were at the bottom of each `Conn_01xN` / as if U1 EN/GPIO23 were at the VIN/3V3 end**. KiCad `Conn_01xN` and `ESP32_DevKit_V1_Socket` put **pin 1 at +Y (top of the symbol)**.

Evidence that the board is still the “good pad-number” side:

- U1 pad 4 = `MQ_ADC`, pad 20 = `SDA_SENS`, pad 21 = `SCL_SENS`, pad 22 = `DHT_DATA`, pad 23 = `SDA_LCD`, pad 24 = `PM_RX`, pad 25 = `PM_TX`, pad 26 = `SCL_LCD`, pad 27 unconnected (GPIO2), pad 29 `GND`, pad 30 `+3V3`.
- J2 pads: 1 `+3V3`, 2 `GND`, 3 `SCL_SENS`, 4 `SDA_SENS`, 5 `+3V3` (CSB), 6 `GND` (SDO).
- J1 pads: 1 `GND`, 2 `+3V3`, 3 `SDA_LCD`, 4 `SCL_LCD`.
- J4 pads: 1 `+5V`, 2 `GND`, 3 `PM_TX`, 4 `PM_RX`.

Fix the **schematic attachments** (move wires/labels onto the correct pin electrical points, or mirror/re-number). Then ERC. Only then update the PCB — and only if a pad net actually disagrees. Prefer **never** letting a bulk update rewrite U1.

If you only need copper: **route the existing PCB nets**; do not re-import.

---

## Blocking fixes (do these first)

Root cause for 1–4: labels were placed as if **pin 1 = bottom** (`−Y`). KiCad symbols use **pin 1 = top** (`+Y`). Several “last” pins landed **2.54 mm off the symbol**, so they are not on any pin (ERC can still be quiet if the orphan is a power symbol).

### 1. U1 — wires sit on the wrong pin numbers

Symbol `atmosmesh:ESP32_DevKit_V1_Socket` at `(95.25, 129.54)`. Electrical X: left `77.47`, right `113.03`. Pin 1 EN is at **+Y** (`y=147.32`), pin 15 VIN at **−Y** (`y=111.76`).

| Net (label) | Attached today (symbol pin / name) | Must attach to |
| --- | --- | --- |
| `ESP_VIN` | **U1.1 EN** (`y=147.32`) | **U1.15 VIN** (`y=111.76`) |
| `GND` (left) | **U1.2 GPIO36/VP** (`y=144.78`) | **U1.14 GND** (`y=114.30`) |
| `MQ_ADC` | **U1.12 GPIO12** (`y=119.38`) | **U1.4 GPIO34** (`y=139.70`) |
| `+3V3` (right) | **U1.16 GPIO23** (`y=147.32`) | **U1.30 3V3** (`y=111.76`) |
| `GND` (right) | **U1.17 GPIO22** (`y=144.78`) | **U1.29 GND** (`y=114.30`) |
| `SDA_SENS` | **U1.26 GPIO4** (`y=121.92`) | **U1.20 GPIO21** (`y=137.16`) |
| `SCL_SENS` | **U1.25 GPIO16/RX2** (`y=124.46`) | **U1.21 GPIO19** (`y=134.62`) |
| `DHT_DATA` | **U1.24 GPIO17/TX2** (`y=127.00`) | **U1.22 GPIO18** (`y=132.08`) |
| `SDA_LCD` | U1.23 GPIO5 (`y=129.54`) | **keep** (only signal that already matches) |
| `PM_RX` | **U1.22 GPIO18** (`y=132.08`) | **U1.24 GPIO17/TX2** (`y=127.00`) |
| `PM_TX` | **U1.21 GPIO19** (`y=134.62`) | **U1.25 GPIO16/RX2** (`y=124.46`) |
| `SCL_LCD` | **U1.20 GPIO21** (`y=137.16`) | **U1.26 GPIO4** (`y=121.92`) |

Leave **no-connect** on: EN, VP/VN, GPIO35–13, GPIO23/22, **GPIO1/TX0**, **GPIO3/RX0**, **GPIO2**, GPIO15. Today TX0/RX0/GPIO2 happen to be NC — keep them NC after the move.

**How:** select the wire + global label on each U1 pin and move them 1:1 onto the pin of the **name** in the third column. Do not rename pads in the footprint. Do not “fix” this by swapping pin numbers inside the symbol unless you also change the footprint pad order (you must not).

After this, a netlist pin-number export must match the PCB pad table above.

### 2. J1 — `SCL_LCD` is off-pin; header still named LCD1602

`J1` `Conn_01x04` at `(185.42, 39.37)`. Pin 1 at `+2.54` (`y=41.91`), pin 4 at `−5.08` (`y=34.29`).

| Pin | Intended | Schematic today |
| --- | --- | --- |
| J1.1 | `GND` | **`SDA_LCD`** (`y=41.91`) |
| J1.2 | `+3V3` | `+3V3` (`y=39.37`) — only this matches by accident |
| J1.3 | `SDA_LCD` | **`GND`** (`y=36.83`) |
| J1.4 | `SCL_LCD` | **no connection** — label `SCL_LCD` sits at `y=44.45` (**2.54 mm above pin 1**) |

PCB pads already have GND / 3V3 / SDA_LCD / SCL_LCD on 1–4. **Do not update PCB.** Move the four attachments onto pins 1–4.

**Rename** Value `LCD1602_I2C` → **`OLED_I2C`** (or `SSD1306_SH1106`). Silkscreen `LCD1602` on the PCB (`gr_text` near J1) must become **OLED**. D-001: 1602 is spare, not the station display.

**OLED pin order (GND-VCC-SDA-SCL vs GND-VCC-SCL-SDA):** J1 and the PCB silkscreen are **GND, 3V3, SDA=5, SCL=4**. Many 4-pin 0.96″ modules are **GND, VCC, SCL, SDA** (clock on pin 3). Photograph the module before copper. If the glass is SCL-then-SDA, either swap J1.3/J1.4 nets **or** document a crossed jumper — do not guess.

### 3. J2 — `SDO` GND is off-pin; SCL/SDA sit on the wrong pin numbers

`J2` `GY-BMP280` `Conn_01x06` at `(185.42, 69.85)`. Pin 1 at `+5.08` (`y=74.93`), pin 6 at `−7.62` (`y=62.23`).

| Pin | Intended (module + silkscreen) | Schematic today |
| --- | --- | --- |
| J2.1 VCC | `+3V3` | `+3V3` (this is the **CSB** row’s geometry — pin 1 is the *top*) |
| J2.2 GND | `GND` | **`SDA_SENS`** (`y=72.39`) |
| J2.3 SCL | `SCL_SENS` (GPIO19) | `SCL_SENS` (`y=69.85`) — matches because it is the middle |
| J2.4 SDA | `SDA_SENS` (GPIO21) | **`GND`** (`y=67.31`) |
| J2.5 CSB | `+3V3` | `+3V3` (`y=64.77`) |
| J2.6 SDO | `GND` (addr 0x76) | **no connection** — extra `GND` at `y=77.47` (**2.54 mm above pin 1**) |

PCB pads 1–6 are already the intended nets. Silkscreen `3V3 / GND / SCL=19 / SDA=21 / CSB / SDO` is the target. Move labels onto pin 1…6; put SDO’s `GND` on **J2.6**, not in space.

### 4. Same pin-1-at-top bug on J3, J4, J5, J7

Same `Conn_01xN` convention. PCB pads are the intended order; schematic pin numbers are reversed or missing the last pin.

**J3 AM2302** at `(185.42, 105.41)` — intended 1=`+3V3`, 2=`DHT_DATA`, 3=`GND`. Today pin 1=`GND`, pin 2=`DHT_DATA`, pin 3=`+3V3`.

**J4 SDS011** at `(185.42, 154.94)`:

| Pin | Intended | Schematic today |
| --- | --- | --- |
| J4.1 | `+5V` | **`PM_TX`** (`y=157.48`) |
| J4.2 | `GND` | `GND` |
| J4.3 | `PM_TX` (sensor TX → GPIO16) | **`+5V`** (`y=152.40`) |
| J4.4 | `PM_RX` (GPIO17 → sensor RX) | **off-pin** `y=160.02` |

**J5 MQ135** — intended 1=`+5V`, 2=`GND`, 3=`MQ_AOUT`, 4=NC. Today pin 1=`MQ_AOUT`, pin 2=`GND`, pin 3=`+5V`, NC flag at `y=195.58` (above pin 1). PCB pad 4 is already `unconnected-(J5-Pin_4-Pad4)`.

**J7 ESP32_Harness** `Conn_01x12` at `(299.72, 69.85)`, pin 1 at `+12.7` (`y=82.55`). Intended (README): 1 `+3V3` … 12 `+3V3`. Labels run `+3V3` at `y=57.15` (near **pin 11**) through `+3V3` at `y=85.09` (**2.54 mm above pin 1** — off-pin). PCB pads 1–12 already match the README table. Populate **U1 or J7, never both**.

### 5. R3 is still 15 kΩ; firmware is 20 kΩ

`R2` = `10k` series `MQ_AOUT`→`MQ_ADC`. `R3` = **`15k`** `MQ_ADC`→`GND`. Firmware `kMq135GndOhms = 20000`. Live breadboard is 10 k / 20 k.

| Ratio | GPIO at 5.0 V AOUT | Headroom vs 3.3 V abs max |
| --- | --- | --- |
| 10 k / 15 k (schematic now) | 3.00 V | 300 mV |
| 10 k / 20 k (bench + firmware) | 3.33 V | **none** |

**Recommendation:** keep **10 k / 15 k on copper** (R2=`10k`, R3=`15k`), then set firmware `kMq135GndOhms = 15000` and the AOUT reconstruction to 15/25 in the **same** change. Alternative: set R3=`20k` and leave firmware at 20000 — worse for GPIO stress, matches the breadboard only.

Do **one** pair everywhere (schematic value, silkscreen, `pins.hpp`, inventory). Do not ship 15 k on the board with 20 k in firmware.

### 6. J1 product identity + OLED SCL/SDA order

Covered under §2: rename LCD→OLED; confirm module pin 3/4 against a photo before locking copper. GPIO map (SDA=5, SCL=4) does not by itself define header order.

---

## Passive rationale (fit vs DNP)

| Ref | Value | Domain | Fit? | Why |
| --- | --- | --- | --- | --- |
| R1 | 10 k | 3V3 | **Fit** | AM2302 data pull-up to `+3V3` if the module has none. Harmless if the module already has one. |
| R2 | 10 k | 5 V analog | **Fit** | Series from `MQ_AOUT` to GPIO34. Never omit — 5 V AOUT must not hit the pin. |
| R3 | 15 k (see §5) | 5 V analog | **Fit** | Divider to GND. Must match firmware. |
| R4 | 4k7 DNP | 3V3 I²C | **DNP** | Optional `SDA_SENS` pull-up. GY-BMP280 already has pull-ups. Fitting both can over-stiffen the bus. Flag is already `dnp yes`. |
| R5 | 4k7 DNP | 3V3 I²C | **DNP** | Same for `SCL_SENS`. `dnp yes`. |
| R6 | 1 k | 5 V | **Fit** | Series for D1. 5 V domain only. |
| C1 | 100 µ / 16 V | 5 V | **Fit on PCB; optional on breadboard** | Bulk on `+5V`. SDS011 is a noisy 5 V load; keep on copper. Breadboard can skip if the PSU is stiff and local. |
| C2 | 10 µ | 3V3 | **Fit on PCB; optional on breadboard** | Bulk on DevKit `+3V3`. DevKit already has ceramics; extra bulk helps OLED/BMP280. |
| C3 | 100 n | 3V3 | **Fit on PCB; optional on breadboard** | HF on `+3V3`. |
| C4 | 100 n | 5 V | **Fit on PCB; optional on breadboard** | HF on `+5V`. |
| D1 | LED_5V | 5 V | **Fit** | Rail present indicator. Not a GPIO. |
| JP1 | VIN_LINK | 5 V → `ESP_VIN` | **Open on bench** | Close only after enclosed, measured PSU, and **never with USB**. |

OLED I²C pull-ups: not on this sheet (module + GPIO5 idle-high). Do not add a GPIO2 load.

---

## What is already OK (do not “fix”)

- **Firmware GPIO map** on `main` (`pins.hpp`) matches live bench. Worktree `.worktrees/atmosmesh-oled-u8g2` same GPIOs.
- **GPIO2** free: schematic NC on U1.27; PCB `unconnected-(U1-GPIO2-Pad27)`.
- **UART0** unused on copper: U1 pads 18/19 unconnected. Keep USB as the only UART0 user.
- **No 5 V net on a GPIO pad.** `+5V` touches J6, J4, J5, C1, C4, R6, D1, JP1 only. MQ135 hits GPIO34 only through R2/R3 as `MQ_ADC`.
- **R4/R5** already `dnp yes`.
- **PCB connector pad nets** already match the intended J1–J5 / J7 / U1 pinout. That is why a schematic→PCB update is dangerous, not because the copper pinout is unknown.
- **JP1** description: open on bench, never with USB.
- **BMP280 straps** on the *intended* J2 table: CSB=`+3V3`, SDO=`GND` → 0x76. Only the schematic *pin numbers* are wrong.
- Board still **unrouted** (ratsnest only). File growth is placement/silkscreen/KiCad 10 rewrite, not finished traces.

---

## Suggested edit order in KiCad

1. **U1 only** — move wires to the named pins in the table. ERC. Export netlist; diff U1 pin nets against the PCB pad list. **Do not update PCB.**
2. **J1–J5, J7** — same: move onto pin 1 = top. Park off-pin labels onto the last pin. Rename J1 OLED. ERC.
3. **R3 vs firmware** — pick 10k/15k (preferred) or 10k/20k; change the other side in the same session.
4. Silkscreen: `LCD1602` → OLED; confirm SCL/SDA legend vs module photo.
5. Only then: route, or Update PCB **with a pin-by-pin net diff**. Do not `--save-board` DRC if you still need KiCad 9 PCB files — this PCB is already KiCad 10.
6. Re-export `atmosmesh-bench-schematic.pdf` after the sheet is honest (PDF today will still show LCD1602 and the wrong U1 attachments).

## Out of scope for the PCB session

- Flashing, merging, committing product code (unless `pins.hpp` must change with R3=15k).
- Photographing the DevKit (U1 order is still provisional in the title block; silkscreen `VERIFY DEVKIT PINOUT`).
- Closing JP1 or paralleling a 3.3 V AC/DC onto `+3V3`.

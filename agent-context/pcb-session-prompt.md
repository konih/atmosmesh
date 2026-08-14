# COPY-PASTE PROMPT — AtmosMesh bench KiCad (extra connectors)

Paste everything below the line into the **KiCad / PCB session**. Do not start from firmware
memory. Live GPIOs below are occupied on the breadboard.

---

You are the **AtmosMesh bench KiCad session**. Goal: add three 3-pin headers that match the **live
breadboard**, without stealing occupied GPIOs and without a bulk schematic→PCB rewrite.

Repo: `atmosmesh`. Open `hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_pro`.

Read first (gitignored, this repo):

- `agent-context/extra-peripherals.md` — live extras + header table
- `agent-context/pcb-schematic-review.md` — pad nets, R3, OLED identity, hard rule
- `agent-context/INBOX.md` — TFT dropped; MQ135 15 k vs 20 k
- `firmware/include/atmosmesh/pins.hpp` — GPIO map on `main`
- If present, `.worktrees/atmosmesh-oled-u8g2/firmware/include/atmosmesh/pins.hpp` is the live
  extras image (`kBeeperGpio=25`, `kPirGpio=33`, `kMicGpio=35` analog). If that worktree is missing,
  use the pin table in this prompt and `extra-peripherals.md`.

Do **not** flash, fabricate, merge, or commit from a coding session unless the operator asked.
Do **not** edit firmware to “match” the schematic while the breadboard is still the running station.

## Goal

1. Add **J_BEEP**, **J_PIR**, **J_MIC** as **1×3** headers: **VCC, GND, SIG** (pin 1 → last).
2. Place silkscreen so a human can plug the live 3-pin cables without a datasheet.
3. Leave existing U1 / J1–J7 pad nets alone unless a pin-by-pin netlist diff proves a mismatch.
4. Station display stays **J1 OLED**. **No TFT. No 480×320. No `J_TFT`.** Operator dropped the LCD.

## Do

- Add the three connectors on the schematic **and** PCB (footprints + silk). Prefer vendored
  `Conn_01x03` / matching 2.54 mm 1×3 THT in `atmosmesh.pretty`.
- Default **VCC = 3.3 V**. Optional jumper **VCC → +5V** only on **beeper power** and **PIR power**
  if the module is a 5 V board (HC-SR501). **SIG never 5 V.** ESP32 GPIOs are 3.3 V.
- Keep GPIO2 and UART0 (GPIO1 / GPIO3) free — USB flash/monitor only.
- Photograph the OLED 4-pin order (GND-VCC-SDA-SCL vs GND-VCC-SCL-SDA) before locking J1 copper.
- Photograph PIR pin order if OUT sits in the middle; silk should still read VCC / GND / SIG unless
  the module forces a different order — then document the crossed jumper, do not guess.
- Run schematic ERC. After adding nets, **diff netlist pin numbers against existing PCB pad nets**
  for U1 and J1–J7 **before** any Update PCB.
- Re-export `atmosmesh-bench-schematic.pdf` when the sheet is honest.

## Do not

- **Do not Tools → Update PCB from Schematic** until every U1 and J* **symbol pin number** carries
  the net that the **pad of the same number** already has. See `pcb-schematic-review.md`. If you
  only need copper for the new headers, add footprints and assign nets by hand; **route existing
  nets**; do not re-import the whole board.
- Do **not** apply the old review’s §1–§4 “move every U1/J* wire” recipe. A later session verified
  that those moves assumed the wrong y-axis sign (symbol lib y-up vs sheet y-down). Untouched sheet:
  ERC **0 errors / 0 warnings**; netlist vs PCB **89 pins / 0 mismatches**. Applying those moves
  produced ERC failures and was reverted. **Re-attaching “fixes” will smash a matching netlist.**
- Do **not** steal GPIOs **4, 5, 16, 17, 18, 19, 21, 25, 33, 34, 35**. GPIO22 is **free**.
- Do **not** use GPIO12 (flash-voltage strap). Keep GPIO0 and GPIO2 unloaded.
- Do **not** add I2S (SCK/WS/SD) for the mic. J_MIC SIG is **GPIO35 analog AO** (ADC1, input-only).
  GPIO22 is **not** the mic. AO must stay **≤3.3 V** (module VCC 3V3).
- Do **not** put 5 V on a GPIO pad. 5 V stays on SDS011 **sensor VCC** and MQ135 **heater** (J4 / J5
  / J6 / C1 / C4 / R6 / D1 / JP1 only, plus optional beeper/PIR **VCC jumper**).
- Do **not** rename U1 footprint pads to “fix” nets. Do not populate U1 and J7 together.
- Do **not** close JP1 with USB attached. Do not add a GPIO2 load or extra OLED pull-ups on GPIO5.
- Do **not** change firmware `kMq135GndOhms` while the breadboard (10 k / 20 k) is live.

## Live pin map (authoritative — match this)

| Function | Wiring | Notes |
| --- | --- | --- |
| OLED I²C 0x3C | SDA=**GPIO5**, SCL=**GPIO4**, VCC=**3V3** | 128×64 SSD1306 **ALT0**. **Not LCD1602.** Visible glass may look 128×32; keep 3-line lower-band UI. Nets `SDA_LCD` / `SCL_LCD`. |
| BMP280 | SDA=**21**, SCL=**19**, CSB=**3V3**, SDO=**GND**, VCC=**3V3** | Addr 0x76. Nets `SDA_SENS` / `SCL_SENS`. |
| AM2302 | DATA=**GPIO18**, VDD=**3V3** | Net `DHT_DATA`. |
| SDS011 | sensor TX → **GPIO16** (RX2), **GPIO17** → sensor RX | **5 V on sensor VCC only.** **Not UART0.** Nets `PM_TX` / `PM_RX`. |
| MQ135 | AOUT **10 kΩ** to **GPIO34**, **20 kΩ** to GND on **breadboard** | 5 V **heater** only. **Never CO2.** ADC1 input-only. |
| **Beeper** | SIG=**GPIO25** | 3-pin. Output. |
| **PIR D-SUN** | SIG=**GPIO33** | 3-pin. **Not 27.** Digital in. |
| **Mic analog AO** | SIG=**GPIO35** **analog** | ADC1, 11 dB. Input-only. VCC 3V3; AO ≤3.3 V. **Not GPIO22.** |
| GPIO2 | free | Download strap. |
| UART0 GPIO1 / GPIO3 | USB / CP2102 only | Keep NC on copper. |

Occupied — do not reuse: **4, 5, 16, 17, 18, 19, 21, 25, 33, 34, 35**, plus 1 / 3 / 0 / 2.

Leftover (do not assign without a new live pin from the operator): 13, 14, 15, 22, 23, 26, 27, 32,
36, 39. Avoid 12.

## New connectors

All **1×3**, pin order **VCC, GND, SIG**. Never 5 V into SIG.

| Ref | Type | Pin 1 → last | Nets | Silkscreen |
| --- | --- | --- | --- | --- |
| **J_BEEP** | 1×3 | VCC, GND, SIG | `BEEP_VCC` (default **3V3**, optional jumper to **+5V** for 5 V **power** only), `GND`, `BEEP_SIG` = **U1 GPIO25** | `BEEPER` |
| **J_PIR** | 1×3 | VCC, GND, SIG | `PIR_VCC` (default **3V3**; jumper to **+5V** only if the board is HC-SR501), `GND`, `PIR_OUT` = **U1 GPIO33** | `D-SUN PIR` |
| **J_MIC** | 1×3 | VCC, GND, SIG | `MIC_VCC` = **3V3** (no 5 V jumper), `GND`, `MIC_SIG` = **U1 GPIO35 analog AO** | `MIC AO` |

Do **not** add `J_TFT`. GPIO22 was once sketched as TFT RST, then digital mic — it is **free**.

Identity (do not block the headers):

- Beeper: active vs passive unknown; GPIO25 HIGH is enough electrically.
- PIR: D-SUN 3-pin (AM312-class vs HC-SR501 with pots). Photo before locking the 5 V VCC jumper default.
- Mic: analog **AO** on GPIO35. Firmware `analogRead` (ADC1 11 dB). Not digital DO.

## Power

- **3.3 V** on every ESP32 GPIO and on OLED / BMP280 / AM2302 / default VCC for the three new headers.
- **5 V only:** SDS011 sensor **VCC**, MQ135 **heater**, optional **BEEP_VCC** / **PIR_VCC** jumpers.
- JP1 (`VIN_LINK`) stays **open on bench**, never with USB.

## Keep these review items (do not churn past them)

Full tables: `agent-context/pcb-schematic-review.md`. Status as of 2026-08-14:

| Item | What is still true | What not to do |
| --- | --- | --- |
| **U1 wiring** | PCB pad nets already follow the intended DevKit pinout (e.g. pad 4 `MQ_ADC`/GPIO34, 20 `SDA_SENS`, 21 `SCL_SENS`, 22 `DHT_DATA`, 23 `SDA_LCD`, 24 `PM_RX`, 25 `PM_TX`, 26 `SCL_LCD`, 27 GPIO2 NC, 29 GND, 30 +3V3). Netlist already matched 89/0. | Do **not** Update PCB. Do **not** re-run the invalidated “move labels because pin 1 is bottom” pass. Before any bulk update, re-diff pin numbers vs pads. |
| **J1 OLED name / order** | Value/silk should be **OLED** / **OLED 0x3C**, not LCD1602 (D-001; 1602 is spare). Intended pads: 1 GND, 2 +3V3, 3 SDA=GPIO5, 4 SCL=GPIO4. | Confirm module pin 3/4 **SDA then SCL** vs **SCL then SDA** from a **photo** before locking copper. GPIO map does not define header order. |
| **J2 SDO** | Intended: 1 +3V3, 2 GND, 3 SCL=19, 4 SDA=21, 5 CSB=+3V3, 6 **SDO=GND** (0x76). PCB pads already that. | Keep SDO on GND. Do not float SDO or tie it to 3V3 unless the operator changes address to 0x77. |
| **R3 ratio** | Copper/schematic **R2=10 k / R3=15 k** (3.0 V at 5 V AOUT — keep on the carrier). Live breadboard + firmware **10 k / 20 k**. | Do **not** change firmware until the carrier replaces the breadboard. Then set `kMq135GndOhms = 15000` in the **same** change that commissions the board. Do not ship 15 k copper with 20 k firmware on the carrier. |

Board is still **unrouted** (ratsnest). PCB is **KiCad 10** (`20260206`). Schematic format KiCad 9
(`20250114`). Title block: verify DevKit pinout against a photo (`VERIFY DEVKIT PINOUT`).

## Files to edit (KiCad only)

| Path | Role |
| --- | --- |
| `hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_sch` | Add J_BEEP / J_PIR / J_MIC, nets, jumpers |
| `hardware/kicad/atmosmesh-bench/atmosmesh-bench.kicad_pcb` | Place 1×3 footprints, silk; **do not bulk-update from schematic** |
| `hardware/kicad/atmosmesh-bench/atmosmesh.kicad_sym` | Only if a new symbol is required (prefer stock Conn_01x03) |
| `hardware/kicad/atmosmesh-bench/atmosmesh.pretty/` | Only if a new footprint is required |
| `hardware/kicad/atmosmesh-bench/atmosmesh-bench-schematic.pdf` | Re-export after the sheet is honest |
| `hardware/kicad/README.md` | Document the three new headers + GPIO 25/33/35 |

Do not edit `.kicad_sch` / `.kicad_pcb` from a firmware-only session.

## Success criteria

- [ ] J_BEEP 1×3: VCC / GND / SIG; SIG = GPIO25; silk `BEEPER`; VCC default 3V3.
- [ ] J_PIR 1×3: VCC / GND / SIG; SIG = GPIO33 (**not 27**); silk `D-SUN PIR`; VCC default 3V3.
- [ ] J_MIC 1×3: VCC / GND / SIG; SIG = GPIO35 **analog AO**; silk `MIC AO`; VCC 3V3 only. **Not GPIO22.**
- [ ] No `J_TFT`. No 480×320. J1 remains OLED 0x3C (SDA=5, SCL=4).
- [ ] Occupied GPIOs 4, 5, 16, 17, 18, 19, 21, 25, 33, 34, 35 unused by anything new.
- [ ] GPIO2 and UART0 still NC. No 5 V net on a GPIO pad.
- [ ] ERC clean. If Update PCB was used at all: pin-by-pin netlist vs **pre-update** U1/J1–J7 pad nets is empty except the **new** connector pads.
- [ ] README lists the three headers. Schematic PDF regenerated.
- [ ] R3 stays 15 k on copper; firmware 20 k left for the breadboard.
- [ ] J2.6 SDO remains GND. J1 silk is OLED, not LCD1602.

When done, note the outcome in this repo’s `agent-context/INBOX.md` (tick or short status). Do not
merge KiCad with firmware extras in one commit unless the operator asked.

# INBOX — atmosmesh

Repo-local coordination. Not the workspace harness inbox.

---

## Operator note — mic SIG is GPIO35 analog (2026-08-14)

**Live breadboard + firmware** (`fix/oled-u8g2-sds011-listen`): microphone **AO on GPIO35**
(ADC1, 11 dB). **Not GPIO22.** GPIO22 is free. Module VCC **3V3**; AO must stay **≤3.3 V**.
GPIO35 is input-only — firmware never drives it as output.

Serial: `mic: raw=…` ~750 ms; `mic: sound` / `mic: quiet` when raw crosses **800** (~645 mV).
PIR stays GPIO33; beeper GPIO25.

**KiCad copper still has `MIC_SIG` on GPIO22** from the extras-header session. **Do not edit
KiCad from firmware.** Next PCB session: move J_MIC SIG to **GPIO35 analog**; leave GPIO22 NC.
Authoritative map: [`extra-peripherals.md`](extra-peripherals.md) and
[`pcb-session-prompt.md`](pcb-session-prompt.md).

## ~~Operator task / PCB session — extra connectors (2026-08-14, **live pins**)~~ DONE

**Outcome (PCB session, 2026-08-14):** J_BEEP (SIG=GPIO25), J_PIR (SIG=GPIO33), J_MIC
(SIG=**GPIO35** digital — operator rewired the mic from GPIO22 to D35 mid-session; U1.17/GPIO22
is NC and free again; **firmware `kMicGpio` on `fix/oled-u8g2-sds011-listen` still says 22 and
needs a firmware-session update to 35**) added to schematic **and** PCB as 1×3 VCC/GND/SIG headers, plus 3-pin
VCC-select jumpers JP_BEEP / JP_PIR (1=+3V3 default, 3=+5V; J_MIC 3V3-only). No J_TFT. Added
by hand — **no Update-PCB-from-Schematic was run**. Board widened 115→139 mm for the labeled
EXTRAS strip; U1 pads 7/8/17 carry `PIR_OUT`/`BEEP_SIG`/`MIC_SIG` (were NC). Verification:
ERC 0/0, DRC 0 violations, **0 schematic-parity issues**, netlist↔PCB **104 pins / 0
mismatches**. GPIO2/UART0 still NC; no 5 V on any GPIO pad; R3 still 15 k. README + schematic
PDF regenerated. Photos still owed: OLED pin order, PIR module class (before locking copper).

### Superseded original entry

#### Operator task / PCB session — extra connectors (2026-08-14, **live pins**)

**Paste-ready prompt for the KiCad session:** [`pcb-session-prompt.md`](pcb-session-prompt.md)
(goal, do/don’t, live pin table, J_BEEP / J_PIR / J_MIC, power, files, success criteria).

**Owner:** KiCad session. **Do not edit `hardware/kicad/` from firmware.** Authoritative map:
[`extra-peripherals.md`](extra-peripherals.md). **TFT dropped — do not add J_TFT.** GPIO22 is
the mic, not TFT RST.

Add 3-pin headers **VCC / GND / SIG** (never 5 V on SIG):

| Header | Pins | Default VCC | SIG | Silk |
| --- | --- | --- | --- | --- |
| J_BEEP 1x3 | VCC, GND, SIG | 3V3 (jumper to 5 V if buzzer is 5 V **power** only) | **GPIO25** | beeper |
| J_PIR 1x3 | VCC, GND, SIG | 3V3 (jumper to 5 V if HC-SR501) | **GPIO33** (not 27) | D-SUN PIR |
| J_MIC 1x3 | VCC, GND, SIG | 3V3 | **GPIO22** digital DO | **HC-20/DC-20** |

**GPIO22 is not ADC.** Mic is sound-detect DO, not analog AO. Occupied: OLED 5/4, BMP 21/19,
AM2302 18, SDS011 16/17, MQ135 34, extras 25/33/22.

OLED: 3-line lower-band UI stays J1. Firmware on `fix/oled-u8g2-sds011-listen` already polls
these GPIOs.

## ~~Open decision — colour TFT vs D-001 OLED-only MVP (2026-08-14)~~ RESOLVED — TFT dropped

**Outcome:** operator dropped the colour TFT. No `J_TFT`. GPIO22/33 are live mic and PIR.

## ~~Operator task / other-session fix — KiCad schematic vs PCB (2026-08-14)~~ RESOLVED — review was invalid

**Outcome (PCB session, 2026-08-14 later):** the review's §1–§4 rested on a y-axis sign error
(symbol libs are y-up, the sheet is y-down). Verified: ERC clean (0/0) and netlist↔PCB diff
**89 pins / 0 mismatches** on the untouched schematic. Nothing was re-attached. Done instead:
J1 renamed `OLED_I2C` (sch+PCB+silk, D-001), README firmware paragraph rewritten. Verdict
prepended to [`pcb-schematic-review.md`](pcb-schematic-review.md).

## Open decision — MQ135 divider: board 15 k vs firmware/breadboard 20 k (2026-08-14)

The carrier board and schematic have R2/R3 = **10 k/15 k** (3.0 V max at GPIO34 — correct choice,
keep). Firmware `kMq135GndOhms = 20000` matches the **live breadboard's physical 10 k/20 k**
(3.33 V at 5 V AOUT, zero headroom; firmware logs the warning). Do **not** change firmware while
the breadboard is the running hardware. **When the carrier board replaces the breadboard:** set
`kMq135GndOhms = 15000` (and the AOUT reconstruction to 15/25) in the same change that
commissions the board.

## Superseded original entry (kept for history)

## Operator task / other-session fix — KiCad schematic vs PCB (2026-08-14)

**Owner:** next KiCad session (not this one). **Do not fabricate. Do not flash. Do not merge or commit KiCad from a coding session unless the operator asked.**

**Pointer:** [`pcb-schematic-review.md`](pcb-schematic-review.md) — full pin/net tables, passive DNP table, MQ135 ratio, and the hard rule **Do not Update PCB from Schematic** until U1 pin numbers match pad numbers.

### Why this is a separate session

The bench board was re-laid-out (PCB file grew; now KiCad 10 `20260206`, still **unrouted**). A geometry re-read of `atmosmesh-bench.kicad_sch` shows the **previous review is still true**. Firmware `firmware/include/atmosmesh/pins.hpp` on `main` already matches live wiring; `.worktrees/atmosmesh-oled-u8g2` uses the **same GPIO numbers**. The defect is schematic **wire-to-pin-number** attachment, not the firmware map.

### Live bench (do not “correct” firmware to the schematic)

- OLED **0x3C**, SDA=**GPIO5**, SCL=**GPIO4**, VCC=**3V3**
- BMP280 SDA=**21**, SCL=**19**, CSB=**3V3**, SDO=**GND**
- AM2302 **GPIO18**
- SDS011 sensor TX→**GPIO16**, **GPIO17**→sensor RX, **5 V on sensor only**
- MQ135 AOUT **10 kΩ** series to **GPIO34**, **20 kΩ** to GND (firmware); schematic **R3 is still 15 kΩ**
- GPIO2 free; UART0 USB only; no 5 V on GPIOs

### Six blocking fixes (details in the review file)

1. **U1** — global labels sit on the wrong DevKit pin numbers (e.g. `MQ_ADC` on GPIO12 not GPIO34; `ESP_VIN` on EN; `+3V3` on GPIO23). PCB pads already have the intended nets. Move wires to the **named** pins; do not remap the footprint.
2. **J1** — `SCL_LCD` is **2.54 mm off pin 4**; SDA/GND swapped onto pins 1/3. Value still **`LCD1602_I2C`**. Station display is the OLED (D-001). Confirm module pin 3/4 is SDA-then-SCL vs SCL-then-SDA before locking copper.
3. **J2** — `SDO` GND is **off pin 6**; SDA/GND on the wrong pin numbers. SCL on pin 3 is the only one that already matches.
4. **J3, J4, J5, J7** — same KiCad `Conn_01xN` pin-1-at-**top** vs labels-as-if-pin-1-at-**bottom**. J4 `PM_RX` and J7 pin-12 `+3V3` are off-pin. PCB pads already match the intended header tables.
5. **R3 15 kΩ vs firmware 20 kΩ** — pick one pair. Prefer **10 k / 15 k on copper** (3.0 V at 5 V AOUT, some GPIO headroom) and set `kMq135GndOhms = 15000` in the same change; or 10 k / 20 k on **both** (no headroom at 5 V AOUT).
6. **Do not Update PCB from Schematic** until (1)–(4) are fixed. A bulk update would rewrite U1/J* pad nets to the **wrong** schematic pin numbers. Route existing PCB nets, or update only after a pin-by-pin netlist diff.

### Already OK (do not churn)

GPIO2 NC; UART0 NC on U1.18/19; no `+5V` on GPIO pads; R4/R5 already DNP; JP1 documented open on bench; `pins.hpp` matches the live bench.

### Answer / instructions

_Operator: assign the KiCad session here, or tick when the sheet’s pin numbers match the PCB pad nets and R3/firmware agree._

# INBOX — atmosmesh

Repo-local coordination. Not the workspace harness inbox.

---

## DECISION — AQ-01 software merged to main after 3 independent-review rounds (2026-08-26)

Landed AtmosMesh Aqua's software (SHT41 + 128×64 OLED + water probe, MQTT-only) at `0e84e35` via
local rebase + ff-merge + `git push origin main` (no branch protection on this repo, confirmed via
`gh api .../branches/main/protection` → 404 and `gh api .../rulesets` → `[]`, so no PR needed).

Three fresh, coordinator-dispatched reviewer rounds (each with zero memory of the prior round, each
independently re-running the full gate matrix) on `lane/aq-01-aqua-software`:
- **Round 1 — REQUEST CHANGES.** F1 (P1: ADR-0001 step 4 requires Task build/flash/monitor/clean
  targets for the new canonical env; `build-all` was undercounting products), F2 (P2: ADR-0001's own
  product matrix table was stale), F3 (P2: dead, never-called `aqua_core_sensors_ok()`). Fixed.
- **Round 2 — REQUEST CHANGES.** F5 (P1): `aqua_mqtt_connect_budget_remaining_ms()` and
  `aqua_network_work_allowed()` were live, adapted production logic (Grove's two-condition network
  gate narrowed to Aqua's one-condition gate) with zero test coverage — the header wasn't even
  compiled by `pio test -e native`. Fixed with a test mirroring Grove's existing coverage.
- **Round 3 — APPROVE**, no P0/P1. Confirmed all prior fixes genuinely landed (not cosmetic),
  line-diffed the MQTT runtime port against Grove's original and found zero dropped logic,
  re-verified GPIO fail-safe ordering and the full v1/Grove struct-literal field audit from source.

**Chose** to merge on this verdict per `/agent-loop-local`'s local merge gate (full gate matrix green
+ independent APPROVE + operator-authored work) — because: the operator explicitly authorized
push-to-main and PR-merge for this repo in-session before this pass started, and scoped the pass to
"AtmosMesh Aqua — software" specifically (hardware wiring is separately and explicitly out of scope,
still gated on AQ-01's own open questions 1–2, unresolved by this merge).

**Not done by this merge:** no wiring, no flashing, no photo evidence captured. AtmosMesh v1 and
Grove v1.5 are behaviorally unchanged (Grove's binary size grew slightly — RAM +576 B, flash
+2536 B — because it links the shared `mqtt_contract.cpp`, which now also carries Aqua's contract
data; documented and measured against a pre-change baseline in AQ-01.md).

CI (`firmware`, `CodeQL`) is running on merge commit `0e84e35` — watched in background, non-blocking.

**Revert:** `git revert 0e84e35 74aa9a7 97f3c0d b70639e --no-commit` then commit, or
`git reset --hard 797aabd` if no one has pulled `main` since (destructive — check first).

## Open — untracked KiCad directory, not created or touched by this pass

`hardware/kicad/atmosmesh-bench/AtmosmeshAqua/` is untracked with an active KiCad lock file,
observed first on 2026-08-26. Not part of this software lane's scope; flagging for whoever owns
that KiCad session.

---

## DECISION — AQ-01 software implemented; wiring still blocked (2026-08-26)

Operator approved story AQ-01 and asked implementation to start, in parallel with soldering the
ESP8266MOD board. Implemented and verified at the software level only — no hardware was touched or
flashed:

- New `AtmosMesh Aqua` product: `product_profile.hpp` gained an Aqua profile (designated
  initializers now, all three profiles — check any future diff carefully for field-order drift);
  `sht4x_frame.hpp/cpp` (new, CRC-verified SHT41 decoder, host-tested against Sensirion's published
  CRC test vector); `aqua_status.hpp/cpp` (new, 4-row OLED text formatting, host-tested); reused
  `soil_sampler.hpp/cpp` as-is for the water probe (not forked); `mqtt_contract.hpp/cpp` gained
  `MqttProductKind::AtmosMeshAquaV1` (3 entities, no pressure); new `aqua_mqtt_runtime.cpp` (ESP8266
  transport, cloned from Grove's); new `products/atmosmesh_aqua_v1.cpp` composition root; new
  `[env:atmosmesh-aqua-v1]` in `platformio.ini`. New decision [D-020](decisions.md) records the
  OLED page layout.
- Verified 2026-08-26: `pio test -e native` 109/109 passed, `task check` clean; `atmosmesh-aqua-v1`,
  `atmosmesh-v1`/`esp32dev`, `atmosmesh-grove-v1_5`/`esp8266-grove` all build clean. Grove's own
  pins/behavior/tests are unchanged, but its **binary is not byte-identical**: measured against a
  pre-change worktree, RAM +576 B (41.9%→42.6%) and flash +2536 B (29.1%→29.3%), because Grove
  links the shared `mqtt_contract.cpp`, which now also carries Aqua's contract literals — same
  pre-existing pattern already puts Grove's contract data inside the v1 ESP32 build. Full detail in
  [AQ-01](stories/AQ-01.md).
- **Not done, and deliberately not done:** nothing has been flashed. AQ-01's own acceptance
  criterion 1 (photograph the board and water-probe hardware to resolve two open identity
  questions) is still open, and independent review is still required before any flash. The operator
  was told, separately, to check for the A0-divider resistors before soldering the water probe (a
  bare ESP-12 module's A0 is 0–1.0 V; a NodeMCU-style board's A0 header is 0–3.3 V) — not yet
  confirmed either way.
- Needs a human read of the diff (profile struct layout change touches all three products) before
  merging. `agent-context/roadmap.md` now has an Aqua-variant table; not previously present.
- Unrelated: an untracked `hardware/kicad/atmosmesh-bench/AtmosmeshAqua/` directory with an active
  KiCad lock file exists in the working tree. This session did not create it and did not touch it —
  flagging since it's untracked, in case it's a KiCad session's in-progress work that should be
  committed separately.

## DECISION — ADR-0002 / D-019: MQTT-only, ESPHome native API declined (2026-08-26)

Operator asked for ESPHome's native API as a second, switchable transport alongside MQTT for a
new hardware variant (ESP8266MOD + SHT41 + 128×64 OLED + 3-pin water probe). `/design-architecture`
produced [ADR-0002](../docs/adr/0002-mqtt-vs-esphome-native-api-transport.md) recommending
MQTT-only for now — the only real device-side library found (`BentuinoESPHomeAPI`) is AGPL-3.0
(conflicts with this repo's MIT license), ~2.5 months old with unaudited vendored crypto, and its
own RAM footprint alone doesn't leave headroom for Aqua's sensors/OLED/MQTT on one ESP8266.
Operator approved; recorded as **Accepted** in decisions.md D-019. Also drafted story
[AQ-01](stories/AQ-01.md) for the new "AtmosMesh Aqua" product variant — **Proposed**, not Ready:
board identity (bare ESP-12 module vs. NodeMCU-style) and water-probe identity (no DO pin — likely
not a YL-38) are unconfirmed and gate all wiring. Needs a human read of the ADR and story before
either is treated as final; roadmap.md has not yet been updated with an Aqua milestone/row.

---

## Operator note — no mic/clap; VEML7700 on Wire1 (2026-08-14)

**Live firmware is `main`:** **no microphone, no clap.** GPIO22 and GPIO35 are free. PIR GPIO33,
beeper GPIO25 stay. VEML7700 lux (part **not fitted yet**) is I²C **0x10** on **Wire1 with BMP280**
(SDA=GPIO21, SCL=GPIO19, VCC 3V3). Serial `veml7700: not found (ok until fitted)` until fitted.
OLED line 1: `1013 hPa   -- lx` (or `123 lx` when present).

A leftover concept-wiring SVG from `codex/add-wiring-image` was **not** merged: it still showed
OLED on GPIO21/22 and DHT on GPIO27, which contradicts the live map. Do not treat that drawing as
a construction aid.

## ~~Operator task / PCB session — extra connectors (2026-08-14, **live pins**)~~ DONE

**Outcome (PCB session, 2026-08-14, final; landed on `main` 2026-08-15):** J_BEEP (SIG=GPIO25),
J_PIR (SIG=GPIO33), and J_VEML (VEML7700 lux on the **shared sensor I²C bus** SDA=GPIO21/SCL=GPIO19,
addr 0x10, 3V3-only 1×4 VCC/GND/SCL/SDA) added. **The HC-20/DC-20 mic was removed by the operator**
(GPIO22→D35, then out entirely): no J_MIC, GPIO22 **and** GPIO35 are free, U1 pads 5/17 NC.
Firmware on `main` matches (no mic, `kVeml7700Address=0x10` on Wire1). Headers on schematic **and**
PCB as 1×3 VCC/GND/SIG (`J_BEEP`, `J_PIR`) plus 3-pin VCC-select jumpers JP_BEEP / JP_PIR
(1=+3V3 default, 3=+5V). No J_TFT. Added by hand — **no Update-PCB-from-Schematic was run**.
Board widened 115→139 mm for the labeled EXTRAS strip; U1 pads 7/8 carry `PIR_OUT`/`BEEP_SIG`
(were NC). Verification: ERC 0/0, DRC 0 violations, **0 schematic-parity issues**, netlist↔PCB
**104 pins / 0 mismatches**. GPIO2/UART0 still NC; no 5 V on any GPIO pad; R3 still 15 k. README +
schematic PDF regenerated. Photos still owed: OLED pin order, PIR module class (before locking
copper).

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

# AtmosMesh Room v2 — perfboard layout and connection diagrams

Companion to [`R2-01`](../../../agent-context/stories/R2-01.md). This is the **drawing set for the
carrier described there**, on the same 7 × 9 cm double-sided perfboard as Room. It is not
approval to apply power. Pin orders are given by **name**; where a module has not yet been
photographed the order is type knowledge and is marked so. Every joint is checked against the
label printed next to the hole, never against a coordinate (the Room lesson: two connector orders
were found reversed before a single joint was made).

Read [`../atmosmesh-room/perfboard.md`](../atmosmesh-room/perfboard.md) first. The grid, the
coordinate convention, the U1 dry-fit and the copper-side mirror warning are unchanged and are not
repeated here. What changes is the sensor set, the power split and three small circuits that use
the new diode and MOSFET stock.

Evidence level: the ideaspark pad map, VEML7700, SHT41 and buzzer orders are operator-confirmed on
Room. The SCD41 breakout, SPS30, ENS160+AHT20, CJMCU-226 and LD2450 orders are **from datasheets
and common module layouts only** and must be confirmed from photographs of the actual parts
(R2-01 acceptance criterion 1). A companion zone drawing is in
[`atmosmesh-room-v2-zones.svg`](atmosmesh-room-v2-zones.svg).

## 1. Board layout

31 columns × 27 rows, 2.54 mm pitch, viewed from the **component side**, USB connector of U1
toward column 1. U1 keeps Room's position: rows 8 and 18, columns 2–16.

```text
         col 1              col 16 │ col 17      col 24 │ col 25      col 31
        ┌─────────────────────────┬────────────────────┬────────────────────┐
row  1  │ LIGHT EDGE              │ SENSOR RAIL        │ 5 V ENTRY          │
        │ J_VEML   C_VEML         │ U2 AMS1117-3.3     │ J_5V_EXT  D_EXT    │
        │ R_SDA    R_SCL          │ C_REG_IN  10 µF    │ JP_5V_SRC          │
        │ R_PU_SDA R_PU_SCL       │ C_REG_OUT 22 µF    │ F1 RXEF050         │
        │ TP_SDA   TP_SCL         │ D_LED_SENS         │ D_LED_5V           │
row  7  ├─────────────────────────┼────────────────────┼────────────────────┤
row  8  │ U1 left  ●●●●●●●●●●●●●●● pads 1–15 (VIN…EN)   │ BUZZER + AUX       │ 5 V CONSUMERS      │
        │ ┌─────────────────────┐ │ J_BEEP  Q_BEEP     │ J_INA (8)          │
        │ │ under the board:    │ │ R_BEEP_G R_BEEP_PD │ J_SPS (5)          │
        │ │ wiring only,        │ │ J_AUX   Q_AUX      │ C_SPS_BULK C_SPS   │
        │ │ nothing taller      │ │ R_AUX_G R_AUX_PD   │ Q_PM_N Q_PM_P      │
        │ │ than the socket     │ │ D_AUX              │ R_PM_*  (optional) │
        │ └─────────────────────┘ │                    │                    │
row 18  │ U1 right ●●●●●●●●●●●●●●● pads 16–30 (3V3…GPIO23) │                    │                    │
row 19  ├─────────────────────────┼────────────────────┼────────────────────┤
        │ VENTILATED EDGE         │ CO₂                │ PRESENCE           │
        │ J_SHT    C_SHT          │ J_SCD (4)          │ J_RAD (4) LD2450   │
        │ TP_3V3C  TP_3V3S        │ C_SCD_BULK 100 µF  │ C_RAD_BULK 100 µF  │
        │ TP_5V    TP_GND         │ C_SCD 100 nF       │ C_RAD 100 nF       │
        │                 J_ENS   │                    │ R_RAD_TX 1 kΩ      │
row 27  └─────────────────────────┴────────────────────┴────────────────────┘
         ◄────── 3.3 V only ──────► ◄── 3.3 V + 5 V in ──► ◄──── 5 V domain ────►
```

| Zone | Columns | Rows | Contents |
| --- | --- | --- | --- |
| Controller | 2–16 | 8 and 18 | U1 socket rows, 15 pins each, 10 pitches apart |
| Under-board | 2–16 | 9–17 | Wiring only |
| Light edge | 1–16 | 1–7 | `J_VEML`, `C_VEML`, `R_SDA`, `R_SCL`, `R_PU_SDA`, `R_PU_SCL`, `TP_SDA`, `TP_SCL` |
| Ventilated edge | 1–16 | 19–27 | `J_SHT` (cols 1–5), `C_SHT`, `J_ENS` (cols 10–16), `C_ENS`, `TP_3V3C`, `TP_3V3S`, `TP_5V`, `TP_GND` |
| Sensor rail | 17–24 | 1–7 | `U2`, `C_REG_IN`, `C_REG_OUT`, `D_LED_SENS`, `R_LED_SENS` |
| Buzzer + AUX | 17–24 | 8–18 | `J_BEEP`, `Q_BEEP`, `R_BEEP_G`, `R_BEEP_PD`; optional `J_AUX`, `Q_AUX`, `R_AUX_G`, `R_AUX_PD`, `D_AUX` |
| CO₂ | 17–24 | 19–27 | `J_SCD`, `C_SCD_BULK`, `C_SCD` |
| 5 V entry | 25–31 | 1–7 | `J_5V_EXT`, `D_EXT`, `JP_5V_SRC`, `F1`, `D_LED_5V`, `R_LED_5V` |
| 5 V consumers | 25–31 | 8–18 | `J_INA`, `J_SPS`, `C_SPS_BULK`, `C_SPS`; optional gate `Q_PM_N`, `Q_PM_P`, `R_PM_B`, `R_PM_C`, `R_PM_EB` |
| Presence | 25–31 | 19–27 | `J_RAD`, `C_RAD_BULK`, `C_RAD`, `R_RAD_TX` |

Placement rules behind the map:

- **5 V stays in columns 25–31**, plus the single feed into `U2` at the top of columns 17–24 and,
  if the AUX output is fitted, one sleeved lead to `J_AUX`. Nothing in columns 1–16 carries 5 V.
- **Heat at the top, thermal sensors at the bottom.** `U2` dissipates about (5 − 3.3) V × 0.3 A ≈
  0.5 W during an SCD41 burst; it sits in rows 1–7 with the TFT backlight. `J_SHT`, `J_SCD` and
  `J_ENS` are on the row-27 edge, at least eight rows away, and the SCD41 and SHT41 hang on short
  leads outside the board. The ENS160 heater warms its own AHT20; keep `J_ENS` at the far end of
  the edge from `J_SHT`.
- **The SPS30 is on a cable** (JST ZHR-5, 1.5 mm pitch); `J_SPS` is the board end of a pigtail,
  so it sits next to `J_INA` and the fuse rather than at an edge.
- **`J_VEML` stays at the row-1 edge** behind an opaque divider from the TFT backlight, as on Room.
- Test points: `TP_3V3C` (controller rail), `TP_3V3S` (sensor rail), `TP_5V`, `TP_GND`, `TP_SDA`,
  `TP_SCL`. Two 3.3 V test points because there are now two 3.3 V rails.

Hole budget: U1 reserves 15 × 11 = 165 holes; the remaining 672 hold about 55 parts with every
optional block fitted, which is not tight. TO-220 parts (`Q_BEEP`, `Q_AUX`) stand upright with
their tabs facing away from U1.

## 2. Controller pad usage (`U1`, ideaspark ESP32 1.14" TFT)

Pad numbering as on Room: pads 1–15 left column, 16–30 right column, both counted from the USB end.
Full map in [`../atmosmesh-room/wiring.md`](../atmosmesh-room/wiring.md). Only the pads the v2
carrier touches:

| Pad | Name | v2 net | Use |
| ---: | --- | --- | --- |
| 1 | `VIN` | `+5V_SRC` | USB 5 V leaves the board here (measure first: Room's `+5V_USB_CONFIRMED` rule) |
| 2, 17 | `GND` | `GND` | Both to the ground rail |
| 16 | `3V3` | `+3V3_CTRL` | Controller rail: buzzer, `C1`/`C2` only |
| 8 | `GPIO25` | `BEEP` | `Q_BEEP` gate through `R_BEEP_G` |
| 9 | `GPIO33` | — | Unwired. Spare input, reserved for an `OUT`-pin radar if a second LD2410S is ever bought |
| 7 | `GPIO26` | `AUX` | Optional: `Q_AUX` gate (fan PWM) |
| 6 | `GPIO27` | `PM_EN` | Optional: 5 V domain gate; else `INA_ALERT` or unwired |
| 21 | `GPIO16 / RX2` | `RAD_TX` | Radar frames in, through `R_RAD_TX` 1 kΩ; the only presence source |
| 22 | `GPIO17 / TX2` | `RAD_RX` | Radar configuration out |
| 26 | `GPIO21 / SDA` | controller side of `R_SDA` | I²C |
| 29 | `GPIO22 / SCL` | controller side of `R_SCL` | I²C |

Never wire GPIO1, 2, 3, 4, 12, 15, 18, 23, 32 or EN: TFT, strap and USB-UART pins, exactly as on
Room. If the second ideaspark board turns out to be the OLED variant, the pad map changes and
this section is rewritten before anything is soldered (R2-01 open question 1).

## 3. Power tree

```text
 USB (through U1 pad 1 VIN) ─────────────┐
                                         ├─ JP_5V_SRC ── +5V_SRC ─┬──────────────────────► U1 VIN  (controller LDO)
 J_5V_EXT (PSU / bench) ── D_EXT 1N5822 ─┘  default OPEN          │                              └─► +3V3_CTRL
                           reverse-polarity                       │                                  ESP32, TFT, Q_BEEP load
                                                                  │
                                                                  ├─► U2 AMS1117-3.3 ────────────► +3V3_SENS
                                                                  │   VIN ─┤C_REG_IN 10 µF            SCD41, SHT41, VEML7700,
                                                                  │   VOUT ─┤C_REG_OUT 22 µF (+)      ENS160+AHT20, INA226,
                                                                  │   GND; tab = VOUT                 R_PU_SDA / R_PU_SCL
                                                                  │
                                                                  └─► F1 RXEF050 ──► [Q_PM_P gate, optional] ──► J_INA VIN+ ══shunt══ VIN−
                                                                       0.5 A hold                                                   │
                                                                                                  +5V_PM ◄──────────────────────────┘
                                                                                                    ├─► J_SPS VDD   (C_SPS_BULK 100 µF + C_SPS 100 nF)
                                                                                                    ├─► J_RAD 5V    (C_RAD_BULK 100 µF + C_RAD 100 nF)
                                                                                                    └─► J_AUX +     (optional fan)

 GND: one ground rail; U1 pads 2 and 17, U2 GND, every module GND, every bulk-capacitor negative.
```

Why the pull-ups sit on `+3V3_SENS` and not on the controller rail: if the sensor rail were ever
down while the controller rail was up, pull-ups on the controller rail would push current into
the unpowered sensors through their protection diodes and drag the bus. Putting every I²C device
**and** its pull-ups on the same rail removes that state. GPIO21/22 tolerate a 3.3 V bus from
either rail. The INA226 therefore also lives on `+3V3_SENS`; this supersedes the rail named for it
in R2-01.

`D_EXT` is the 3 A Schottky from the assortment, in the external-supply leg only. It drops about
0.3 V at these currents, so an external 5.0 V supply reaches the SPS30 at roughly 4.7 V; the SPS30
minimum is 4.5 V, so set a bench supply to 5.2 V or measure `TP_5V` under SPS30 fan start-up. USB
does not pass through `D_EXT`: the ideaspark board already has its own diode between USB and
`VIN`, and a second one would put the SPS30 below its minimum.

Budget through USB with both jumpers as drawn: ESP32 Wi-Fi peak ≈ 300 mA + TFT ≈ 40 mA + SCD41
burst ≈ 200 mA + SPS30 fan start ≈ 80 mA + LD2450 radar on the order of 100 mA ≈ 720 mA peak. A
USB 2.0 port is rated 500 mA; use a supply or the bench PSU through `J_5V_EXT` for the first run
(R2-01 open question 5). On the 5 V domain alone, SPS30 + radar + fan sit near 280 mA continuous
against the RXEF050's 0.5 A hold; the radar's datasheet figure decides whether `F1` moves up to
the RXEF075.

Indicators, same rule as Room (red or green LED, never blue or white):

| LED | Rail | Resistor | Lit means |
| --- | --- | --- | --- |
| `D_LED_5V` | `+5V_PM` (after F1, the gate and the shunt) | 1 kΩ | the SPS30 domain is live and the fuse holds |
| `D_LED_SENS` | `+3V3_SENS` | 470 Ω | U2 is regulating |

## 4. I²C bus

One bus, 3.3 V, 100 kHz for commissioning. Series resistors at the ESP32 end, pull-ups on the
sensor side of them, sensor rail as the reference.

```text
                                   R_PU_SDA 3k3 ── +3V3_SENS
                                                │
GPIO21 (pad 26) ── R_SDA 330 Ω ── SDA_EXT ──────┼── J_VEML SDA   VEML7700       0x10
                                                ├── J_SHT  SDA   SHT41          0x44
                                                ├── J_ENS  SDA   ENS160 + AHT20 0x52/0x53 + 0x38
                                                ├── J_SCD  SDA   SCD41          0x62
                                                ├── J_INA  SDA   INA226         0x40
                                                └── J_SPS  SDA   SPS30          0x69   (5 V module, 3.3 V logic — confirm)

                                   R_PU_SCL 3k3 ── +3V3_SENS
                                                │
GPIO22 (pad 29) ── R_SCL 330 Ω ── SCL_EXT ──────┼── J_VEML SCL
                                                ├── J_SHT  SCL
                                                ├── J_ENS  SCL
                                                ├── J_SCD  SCL
                                                ├── J_INA  SCL
                                                └── J_SPS  SCL
```

Route `SDA_EXT`/`SCL_EXT` as one daisy chain rather than a star: light edge (`J_VEML`) →
ventilated edge (`J_SHT`, `J_ENS`) → CO₂ block (`J_SCD`) → 5 V consumers (`J_INA`, `J_SPS`). No
capacitors on SDA or SCL anywhere. Expected scan: `0x10 0x38 0x40 0x44 0x52|0x53 0x62 0x69`,
nothing else; `0x3C` appearing means the controller is the OLED variant and section 2 is wrong for
this board.

Pull-up sum: 3.3 kΩ on the carrier in parallel with whatever each of the six breakouts carries.
Photograph and sum them; the parallel result must stay ≥ 1.1 kΩ. With six 10 kΩ breakouts the
total would already be ≈ 1.1 kΩ, so **lift breakout pull-ups** rather than the carrier ones if
the sum comes out lower. Then measure rise time on the DSO2D15 and adjust `R_PU_*` from the
measurement.

If the optional 5 V gate is fitted, an unpowered SPS30 still hangs on the bus. Its interface
pins are specified as tolerant of that state in the datasheet — confirm it there in the same
reading that settles the logic level; if it is not, the gate is dropped, not the sensor.

## 5. Connection diagrams per module

### 5.1 SCD41 CO₂ (`J_SCD`, 4 pins) — sensor rail

Order **unconfirmed** (breakout not photographed). Sensirion, Adafruit and Seeed breakouts all
label the pins; wire by label.

```text
              J_SCD
 +3V3_SENS ── VIN / VDD ──┬── C_SCD_BULK 100 µF (+)   ← bursts up to ~200 mA
                          └── C_SCD 100 nF               both right at the header
 GND       ── GND
 SCL_EXT   ── SCL
 SDA_EXT   ── SDA
```

Address `0x62`, no address pin. If the breakout has a `3Vo`/`3V3` regulator-output pin, leave it
**not connected**, as with the VEML7700. Put the sensor itself on a short lead outside the board
in free air, away from `U2` and the backlight; its T/RH output is compensated for its own package,
not for a warm carrier.

### 5.2 SPS30 particulate (`J_SPS`, 5 pins) — 5 V domain

Order from the SPS30 datasheet (JST ZHR-5 on the sensor, pin 1 marked on the housing).
**Confirm the 3.3 V logic level from the datasheet before wiring** and add the PDF to
`docs/hardware/datasheets/`.

```text
                 J_SPS (board end of the ZH pigtail)
 +5V_PM       ── 1  VDD       5 V, after F1, the optional gate and the INA226 shunt
 SDA_EXT      ── 2  SDA / RX  I²C data (UART RX in UART mode — not used)
 SCL_EXT      ── 3  SCL / TX  I²C clock
 GND          ── 4  SEL       tie to GND  →  selects I²C
 GND          ── 5  GND
```

`C_SPS_BULK` 100 µF and `C_SPS` 100 nF from `+5V_PM` to GND at the header. The SPS30 has no
pull-ups of its own. `SEL` floating selects UART; on a UART sensor the bus scan simply does not
find `0x69`, so a missing `0x69` with everything else present means "check `SEL`" before anything
else. The JST-PH set in stock is 2.0 mm pitch and does not fit this connector (R2-01 open
question 2).

### 5.3 ENS160 + AHT20 (`J_ENS`, 8 pins) — sensor rail

Order as counted by the operator (8 pins) and as printed on the common module; **confirm the
regulator and `ADD` behaviour from the photograph**.

```text
                 J_ENS
 +3V3_SENS    ── 1  VIN     module regulator input (accepts 3.3 V; feed 3.3 V until inspected)
 NC           ── 2  3V3     regulator output on most modules — leave open
 GND          ── 3  GND
 SCL_EXT      ── 4  SCL
 SDA_EXT      ── 5  SDA
 NC           ── 6  CS      I²C mode when high; modules pull it up — verify, else tie to VIN
 GND          ── 7  ADD     LOW → ENS160 at 0x52   (HIGH → 0x53)
 NC           ── 8  INT     optional data-ready, not used
```

`C_ENS` 100 nF at `VIN`/`GND`. The AHT20 answers at `0x38` regardless of `ADD`. Only one of the
two modules in stock goes on this bus; two AHT20s would collide.

### 5.4 SHT41 (`J_SHT`, 4 pins) — sensor rail

Room's operator-confirmed order, **re-check on the spare** before fitting:

```text
              J_SHT
 +3V3_SENS ── 1  VIN
 GND       ── 2  GND
 SCL_EXT   ── 3  SCL
 SDA_EXT   ── 4  SDA
```

`C_SHT` 100 nF at the header. Address `0x44`. Far end of the ventilated edge from `J_ENS`.

### 5.5 VEML7700 (`J_VEML`, 5 pins) — sensor rail

Unchanged from Room:

```text
              J_VEML
 +3V3_SENS ── 1  VIN
 NC        ── 2  3Vo    never driven, never tied to 3.3 V
 GND       ── 3  GND
 SCL_EXT   ── 4  SCL
 SDA_EXT   ── 5  SDA
```

`C_VEML` 100 nF at the header. Address `0x10`. Opaque divider toward the TFT backlight.

### 5.6 INA226 current monitor (`J_INA`, CJMCU-226, 8 pins) — measures the 5 V domain

Pin names as printed on the CJMCU-226 (**order to be photographed**). The shunt is in the
**5 V power path**; the logic side is on the sensor rail. Read the shunt marking (`R100` = 0.1 Ω
is common) and record it for firmware.

```text
                 J_INA
 +3V3_SENS    ── VCC
 GND          ── GND
 SDA_EXT      ── SDA
 SCL_EXT      ── SCL
 NC / GPIO27  ── ALE     open-drain alert, optional (GPIO27 is taken if the 5 V gate is fitted)
 +5V_PM       ── VBS     bus-voltage sense — tie to VIN− so it reads the load side
 F1 / gate out ── VIN+   from the PPTC (or from Q_PM_P collector)
 +5V_PM       ── VIN−    to J_SPS VDD, J_RAD 5V, J_AUX

 current flow:  F1 ──► VIN+ ══ R_shunt ══ VIN− ──► load      (VIN+ more positive = positive current)
```

Address `0x40` with A0/A1 at GND, the module default; if the board has solder jumpers, leave them.
At 0.1 Ω the SPS30's 80 mA peak drops 8 mV and the fuse's 0.5 A hold current 50 mV: harmless.

### 5.7 Presence radar (`J_RAD`, HLK-LD2450, 4 pins) — 5 V domain, UART only

The LD2410S went to the Spot ([SP-01](../../../agent-context/stories/SP-01.md)) because that
board has no 5 V domain; Room v2 has one, so it takes the LD2450. Pin names by type knowledge:
`5V`, `GND`, `RX`, `TX` on a 1.25 mm connector (some boards carry a fifth, unused pin; wire by
label). 5 V supply, current on the order of 100 mA — **read the datasheet**, it sets `F1`. UART
256000 baud 8N1, continuous target frames, 3.3 V logic per the module description — **confirm**.
There is no `OUT` pin: presence exists only once the frame decoder runs (R2-03).

```text
 +5V_PM ── J_RAD 5V ──┬── C_RAD_BULK 100 µF (+)
                      └── C_RAD 100 nF
 GND    ── J_RAD GND

 J_RAD TX ── R_RAD_TX 1 kΩ ── GPIO16 / RX2 (pad 21)     frames into the ESP32
 J_RAD RX ─────────────────── GPIO17 / TX2 (pad 22)     configuration commands out
```

`R_RAD_TX` is Room's `R_SDS_RX` pattern: a 5 V module talking into a 3.3 V GPIO gets a series
resistor that limits the current if the logic level turns out higher than claimed, and it costs
nothing when the level is 3.3 V as expected. Measure `J_RAD TX` idle-high with the meter before
GPIO16 is connected; stop if it reads above 3.3 V and fit a divider instead.

Placement: the radar's antenna face looks into the room, with no metal and no copper in front of
it, **and not across the SPS30's fan or the AUX fan** — a 24 GHz radar sees a spinning fan as a
moving target, and a presence sensor that reports the station's own fan is worse than a PIR.
Put `J_RAD` at the row-27 edge facing away from the 5 V consumers block, or on a short lead.

GPIO33 stays free. If a second LD2410S is bought later, its `OUT` pin goes there through the PIR
transistor stage from the Room wiring, and the LD2450 returns to the drawer.

### 5.8 Buzzer (`J_BEEP`, 3 pins) — MOSFET low-side driver

Room drives the buzzer module's `S` pin straight from GPIO25 through 100 Ω and then has a page of
rules about lowering that resistor and staying under 20 mA. v2 uses one of the ten IRLB8721 to
switch the module's ground instead, so **GPIO25 sources only gate charge** and the transducer type
no longer decides the drive current.

```text
 +3V3_CTRL ─────────────────────── J_BEEP S        (signal/positive; tie to the rail)
 +3V3_CTRL ─────────────────────── J_BEEP middle   (VCC; NC on many modules, harmless)
 J_BEEP −  ── Q_BEEP drain
              Q_BEEP source ── GND
 GPIO25 (pad 8) ── R_BEEP_G 100 Ω ── Q_BEEP gate ── R_BEEP_PD 100 kΩ ── GND
```

IRLB8721, TO-220, facing the label: **gate, drain, source**, tab = drain. It is absurdly
oversized for a 30 mA buzzer and that is fine: it is stock, logic-level, and needs no base-current
calculation. Logic stays active-HIGH on GPIO25. For an active module the drive is DC for at most
250 ms as on Room; for a passive piezo, bounded 2–4 kHz PWM on the gate. If a louder alert is ever
wanted, `S` and the middle pin can move to `+5V_PM` — the MOSFET does not care — but the module
must then be one rated for 5 V, and the lead is sleeved like every other 5 V lead.

### 5.9 Sensor rail regulator (`U2`, AMS1117-3.3, SOT-223)

Pins facing the label, left to right: **GND, VOUT, VIN**; tab = VOUT. Swapping VIN and VOUT puts
5 V on the sensor rail, so this footprint is checked with the meter before the first sensor is
plugged in.

```text
 +5V_SRC ── U2 VIN ──┤C_REG_IN 10 µF (+)── GND
            U2 GND ── GND
            U2 VOUT (tab) ── +3V3_SENS ──┤C_REG_OUT 22 µF (+)── GND ── D_LED_SENS via R_LED_SENS 470 Ω
```

Solder the tab to a copper area of at least a few square centimetres on the copper side, or fit a
clip-on heatsink: 0.5 W at the SCD41 burst is fine on copper, marginal in free air. The 22 µF
output capacitor is required for stability, not optional.

### 5.10 Optional: 5 V domain gate (`Q_PM_N` S8050 + `Q_PM_P` S8550)

Lets firmware switch the whole SPS30 domain off and on: hard reset of a hung sensor, true zero
power in a sleep schedule, and a commissioning default of **off** until the bus is proven. It is
high-side, so the SPS30's ground and I²C reference never float — the reason a low-side MOSFET is
the wrong tool here even though it is the one in stock in quantity.

```text
 F1 out ── Q_PM_P emitter
           Q_PM_P base ──┬── R_PM_EB 10 kΩ ── F1 out        (holds the PNP off with the ESP32 in reset)
                         └── R_PM_C 1 kΩ ── Q_PM_N collector
           Q_PM_P collector ── J_INA VIN+  (→ shunt → +5V_PM)

 GPIO27 (pad 6) ── R_PM_B 4.7 kΩ ── Q_PM_N base ── R_PM_PD 100 kΩ ── GND
                                    Q_PM_N emitter ── GND
```

GPIO27 HIGH → `Q_PM_N` on → pulls the PNP base low through 1 kΩ (about 4 mA) → `Q_PM_P` saturates
and passes the domain with roughly 0.1–0.2 V drop at 100–200 mA. GPIO27 LOW or floating (boot,
reset, flashing) → domain **off**. Fits within the 0.3 V margin discussed under `D_EXT` only if the
external supply is set to 5.2 V; on USB with the gate fitted, measure `TP_5V` during fan start-up
before trusting the SPS30's readings. The S8550 is rated 1.5 A; a 2N3906 (200 mA) is not enough
headroom once a fan sits on the same domain, so use the S8550. Verify E/B/C from the datasheet:
the S8xxx family's pin order differs from the 2N series.

If this block is left out, `F1` wires straight to `J_INA VIN+` and GPIO27 goes to the INA226 alert
or stays unwired.

### 5.11 Optional: AUX switched 5 V output (`J_AUX`, IRLB8721 + 1N5819)

The ENV-01 lesson was sensors starving in still air inside a housing. A small fan drawing air
past the SCD41 and SHT41, pulsed a few seconds before each measurement, is the cheapest fix, and
the stock now has both the driver parts and a fan: one small axial fan, marking dictated as
**"EXAV-XV-B0"** (spelling unverified). Its rated voltage, frame size, current and lead count are
still to be read from the label. It goes on `J_AUX` only if the label says 5 V; a 12 V fan does not
start reliably at 5 V and would need its own supply, not this domain.

```text
 +5V_PM ── J_AUX +  ──┬── D_AUX 1N5819 cathode (band)      flyback across the load
 J_AUX −  ────────────┴── D_AUX anode
 J_AUX −  ── Q_AUX drain
             Q_AUX source ── GND
 GPIO26 (pad 7) ── R_AUX_G 100 Ω ── Q_AUX gate ── R_AUX_PD 100 kΩ ── GND
```

Low-side is correct here because a two-wire fan has no logic connection to anything. If the fan
turns out to have a tacho or PWM lead, leave that lead unconnected: a tacho output referenced to
a switched ground is meaningless, and the speed is set by `Q_AUX` anyway. The load current runs
through `F1` and the INA226 shunt with the SPS30, so the 0.5 A hold current is the shared budget:
a 40 mm 5 V fan at ≈ 100 mA plus the SPS30's 80 mA peak leaves room; above about 150 mA of fan
current the next PPTC up (RXEF075) is the right `F1`, with a fresh look at the USB budget. Log the
fan's start-up current on the INA226 during commissioning, the same way as the SPS30's. PWM at a few hundred hertz to a few kHz on
GPIO26 sets the speed. Fit `D_AUX` with the band toward `J_AUX +`; a MOSFET without the flyback
diode sees the fan's inductive kick on every off-edge.

Any other 5 V load — a relay, a heater for a future outdoor variant — uses the same three-part
driver, which is why it gets a designator now even if the header stays empty.

## 6. Reference designators and bill of materials from stock

| Ref | Part | From stock |
| --- | --- | --- |
| `U1` | ideaspark ESP32 1.14" TFT (second board, variant to confirm) | 1 of 2 |
| `U2` | AMS1117-3.3 SOT-223 | 1 of 4 |
| `F1` | RXEF050 PPTC, 0.5 A hold | 1 of 10 |
| `D_EXT` | 1N5822 Schottky 3 A | 1 of 5 |
| `D_AUX` (optional) | 1N5819 Schottky 1 A | 1 of 10 |
| `Q_BEEP`, `Q_AUX` (optional) | IRLB8721 N-MOSFET, TO-220 | 2 of 10 |
| `Q_PM_N` (optional) | 2N3904 or S8050 NPN | transistor kit |
| `Q_PM_P` (optional) | S8550 PNP | transistor kit |
| `R_SDA`, `R_SCL` | 330 Ω | E24 kit |
| `R_PU_SDA`, `R_PU_SCL` | 3.3 kΩ (adjust after rise-time measurement) | E24 kit |
| `R_PM_EB` | 10 kΩ | E24 kit |
| `R_BEEP_PD`, `R_AUX_PD`, `R_PM_PD` | 100 kΩ | E24 kit |
| `R_BEEP_G`, `R_AUX_G` | 100 Ω | E24 kit |
| `R_PM_B` | 4.7 kΩ | E24 kit |
| `R_PM_C`, `R_RAD_TX` | 1 kΩ | E24 kit |
| `R_LED_SENS` | 470 Ω | E24 kit |
| `R_LED_5V` | 1 kΩ | E24 kit |
| `C_VEML`, `C_SHT`, `C_ENS`, `C_SCD`, `C_SPS`, `C_RAD`, `C1` | 100 nF ceramic | ceramic assortment |
| `C_REG_IN` | 10 µF electrolytic | Elko assortment |
| `C_REG_OUT` | 22 µF electrolytic | Elko assortment |
| `C_SCD_BULK`, `C_SPS_BULK`, `C_RAD_BULK` | 100 µF electrolytic | Elko assortment |
| `C2` | 10–47 µF electrolytic at U1 3V3 | Elko assortment |
| `D_LED_SENS`, `D_LED_5V` | red or green 3 mm LED | LED assortment |
| `J_VEML` 5, `J_SHT` 4, `J_ENS` 8, `J_SCD` 4, `J_INA` 8, `J_RAD` 4, `J_BEEP` 3 | female header strips | header stock |
| `J_SPS` | 5-pin header + JST ZH 1.5 mm pigtail (to source) | **not in stock** |
| `J_5V_EXT`, `J_AUX` | 2-pin screw terminal or header | stock |
| `JP_5V_SRC` | 3-pin header + jumper | stock |
| `TP_*` | six test-point pins | stock |
| Sensors | SCD41, SPS30, ENS160+AHT20, SHT41, VEML7700, CJMCU-226, LD2450 | 1 each |
| Fan on `J_AUX` (optional) | small axial fan, marking dictated as "EXAV-XV-B0"; rating to read from the label | 1 |

Not used on purpose: the 1N4148 (no small-signal clamping needed; the series resistors and the
transistor stages do that job), the 1N4007 family (too slow and too lossy for a 5 V rail), the
IRLZ34 (the IRLB8721 has the lower on-resistance and both jobs are trivial for either), and the
fuse holders with glass fuses (the PPTC self-resets; a glass fuse belongs on the mains side of the
enclosed PSU, R2-06).

## 7. Build order

1. Dry-fit U1 (Room perfboard step 1). Stop if the rows do not seat at 10 pitches.
2. Socket strips, then the ground rail and the two 3.3 V rails as separate wires. Verify
   continuity, and verify **no** continuity between `+3V3_CTRL`, `+3V3_SENS` and `+5V_SRC`.
3. `U2` with `C_REG_IN`/`C_REG_OUT`; power from the bench PSU at 5 V through `J_5V_EXT` and
   `D_EXT` with U1 **removed** and confirm 3.3 V at `TP_3V3S` before anything hangs on that rail.
4. I²C branch: `R_SDA`, `R_SCL`, `R_PU_SDA`, `R_PU_SCL`, `TP_SDA`, `TP_SCL`, then the sensor-rail
   headers `J_VEML`, `J_SHT`, `J_ENS`, `J_SCD`, `J_INA` and their 100 nF capacitors.
5. `C_SCD_BULK` at `J_SCD`. Observe polarity on every electrolytic.
6. Buzzer: `Q_BEEP`, `R_BEEP_G`, `R_BEEP_PD`, `J_BEEP`.
7. Sleeve every 5 V lead in heat shrink. Then the 5 V domain: `J_5V_EXT`, `D_EXT`, `JP_5V_SRC`
   (open), `F1`, the gate block if fitted, `J_SPS` with its capacitors, the `VIN+`/`VIN−` wiring
   through `J_INA`, `D_LED_5V`.
8. Presence block, 5 V domain: `R_RAD_TX`, `C_RAD_BULK`, `C_RAD`, `J_RAD`; sleeve its 5 V lead.
9. AUX block if fitted: `Q_AUX`, `R_AUX_G`, `R_AUX_PD`, `D_AUX`, `J_AUX`.
10. Remaining test points and `D_LED_SENS`.

Commissioning is staged exactly as in R2-01: sensor rail alone and a bus scan, then the 5 V domain
with the INA226 logging the SPS30 start-up, then the radar, then the AUX output. Stop conditions
are Room's, unchanged: disconnect for any wrong rail voltage, a hot part, 5 V on any I²C or GPIO
net, or a lead order that is uncertain.

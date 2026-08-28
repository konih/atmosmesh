# ROOM-01 wiring, protection, assembly and commissioning plan

This is the complete construction contract for the room carrier. It defines the intended nets and
safe build order, but it is **not approval to apply power**. The board is built by hand on a 31 × 27
perfboard ([`perfboard.md`](perfboard.md)), not fabricated.

The controller, VEML7700, SHT41, D-SUN PIR and buzzer connector orders are now operator-confirmed.
Still outstanding: the controller's physical row spacing, the SDS011 module's own connector order,
and every 5 V measurement. Never infer pin order or connector orientation from a generic product
photograph.

## Design decisions

### One 3.3 V domain for the controller and I²C sensors

The VEML7700 and SHT41 are powered directly from the Ideaspark board's confirmed `3V3` output. No
series diode and no 50 Ω sensor-supply resistor network is fitted. Every pull-up on this bus —
`R_PU_SDA`/`R_PU_SCL` and any on a breakout — references the same 3.3 V the sensors run from, so
dropping sensor VDD would drag the pull-ups down with it and buy no protection, while adding a
droop path under the SHT41's measurement current bursts. Direct 3.3 V keeps sensor VDD and every
pull-up on one rail.

An earlier revision justified this by the integrated OLED holding SDA/SCL at 3.3 V. That reasoning
was wrong for this hardware: the ideaspark 1.14 inch board's display is SPI and never touches
GPIO21/22. The conclusion stands, the reason does not.

Protection is instead provided by keyed connectors, staged measurements, local decoupling and
330 Ω series resistors in the *external* SDA/SCL branch. Those resistors limit current during a
signal short or wiring mistake and damp fast edges. They do **not** create appreciable measurement
noise. `C1`, `C3` and `C4` decouple power; their job is different from the series resistors.

`C1`, `C3`, `C4` and `C7` are **100 nF**. The recorded ceramic assortment in
[`elektronik-inventar.md`](../../../docs/elektronik-inventar.md) tops out at 100 nF, and D-022
already accepts anything from 47 nF to 220 nF here, so 100 nF is both stocked and correct. Do not
substitute electrolytics for these high-frequency ceramic capacitors.

### The carrier supplies the I²C pull-ups

Nothing on the controller board pulls SDA or SCL up: its display is SPI. `R_PU_SDA` and `R_PU_SCL`,
3.3 kΩ from `SDA_EXT`/`SCL_EXT` to 3.3 V, are therefore **required**, not optional. Without them a
build populated with only the SHT41, or with a breakout whose pull-ups are unfitted, leaves the bus
floating and every transaction fails.

They sit on the sensor side of `R_SDA`/`R_SCL` so the 330 Ω series resistors remain a fault-current
limit rather than part of the pull-up divider. If both breakouts carry their own 10 kΩ pull-ups the
parallel result is about 2.0 kΩ — roughly 1.6 mA at 3.3 V, inside the I²C 3 mA sink limit — so keep
them fitted. After assembly, measure the unpowered resistance from SDA to 3.3 V and SCL to 3.3 V,
then verify idle-high voltage when powered.


### Domain indicators

Both 5 V jumpers are default-open, and the build plan tells you to leave them open until the
measurements are recorded. Until now there was no way to see whether a domain was live.

| LED | Rail | Resistor | Current | Lit means |
| --- | --- | --- | ---: | --- |
| `D_LED_3V3` | `+3V3` | `R_LED_3V3` 470 Ω | ~2.8 mA | the board has 3.3 V |
| `D_LED_SDS` | `SDS_5V_PROTECTED` | `R_LED_SDS` 1 kΩ | ~3.0 mA | `JP_SDS_5V` is **closed** |
| `D_LED_PIR` | `PIR_5V_PROTECTED` | `R_LED_PIR` 1 kΩ | ~3.0 mA | `JP_PIR_5V` is **closed** |

Each 5 V indicator draws through its own jumper. That is the feature, not a side effect: the LED
cannot light unless the jumper it reports on is actually closed.

> **Use a red or green LED, not blue or white.** The sizing assumes a forward voltage near 2.0 V.
> A blue or white LED drops about 3.0 V, which leaves 0.3 V across `R_LED_3V3` on the 3.3 V rail —
> roughly 0.6 mA, and it will look dead. On the 5 V rails a blue LED still works but runs dim.

Anode to the rail through the resistor, cathode to GND. On a bare LED the **short lead and the flat
on the rim are the cathode**; do not infer polarity from lead length alone on a used part.

### Resistor sizing

The operator holds a complete E24 kit, so every value below is the electrically chosen one, not the
nearest thing in a drawer.

| Ref | Value | Why this value |
| --- | ---: | --- |
| `R_PU_SDA`, `R_PU_SCL` | 3.3 kΩ | Rise time is the binding constraint on hand-wired perfboard. At 100 kHz I²C allows 1000 ns. Assuming ~200 pF of hand-soldered bus capacitance, 4.7 kΩ gives 940 ns — right on the limit — while 3.3 kΩ gives 660 ns. Sink stays at 1.0 mA alone, or 1.6 mA with two 10 kΩ breakout pull-ups in parallel, against the 3 mA I²C limit. **Changed from 4.7 kΩ when the build moved to perfboard.** |
| `R_SDA`, `R_SCL` | 330 Ω | Fault-current limiting is the primary job: 3.3 V / 330 Ω = 10 mA, comfortably under the 40 mA GPIO maximum. The cost is a divider against the pull-ups when the ESP32 drives low — sensors then see 3.3 V × 330 / (2030 + 330) = 0.46 V, still well under the 0.99 V I²C VIL. Dropping to 220 Ω would improve that margin but push fault current to 15 mA for no real gain. |
| `R_SDS_RX`, `R_SDS_TX` | 1 kΩ | Sized to bound a UART driver fight to 3.3 V / 1 kΩ ≈ 3.3 mA against the 40 mA maximum. Signal cost is nil: 1 kΩ into a few hundred pF settles in ~200 ns against a 104 µs bit at 9600 baud. Also limits clamp current to ~1.4 mA if a 5 V module is ever fitted by mistake. |
| `R_PIR_IN` | 10 kΩ | Base drive for `Q_PIR`. Collector current is only 3.3 V / 10 kΩ = 0.33 mA through `R_PIR_PU`, needing ~33 µA of base current for a forced β of 10. 10 kΩ supplies (3.3 − 0.7) / 10 kΩ = 260 µA — deep saturation with 8× margin, while loading the PIR output with a quarter of a milliamp. |
| `R_PIR_PU` | 10 kΩ | Collector pull-up holding GPIO33 high when the PIR is idle. Keeps `Q_PIR` collector current at 0.33 mA, which is what makes the 10 kΩ base resistor comfortable. |
| `R_PIR_PD` | 100 kΩ | Defines `Q_PIR`'s base if the PIR output floats or is removed. Ten times `R_PIR_IN`, so it costs about 9% of the base drive and cannot prevent turn-on. |
| `R_LED_3V3` | 470 Ω | 3.3 V rail less a ~2.0 V red LED leaves 1.3 V; 1.3 V / 470 Ω = 2.8 mA. Bright enough to read across a room, negligible against the rail budget. |
| `R_LED_SDS`, `R_LED_PIR` | 1 kΩ | 5 V rail less a ~2.0 V LED leaves 3.0 V; 3.0 V / 1 kΩ = 3.0 mA. Deliberately the same order as the 3.3 V indicator so all three read at similar brightness. |
| `R_BEEP_S` | 100 Ω | The one value that is safe across all three transducer types, so it stays until the ohmmeter test names the load. Piezo: `Xc` is about 3.5 kΩ at 2.3 kHz, so 100 Ω costs no audible volume. Active: a can drawing ~25 mA looks like ~130 Ω, so 100 Ω leaves it only ~1.9 V of the 3.3 V — enough to be weak, which is exactly what the step-down rule is for. Magnetic worst case: ~28 mA for 50 ms, under the 40 mA absolute maximum. **Never raise it** — on the active and magnetic loads a larger value is silence, not protection. |

### Five-pin VEML7700 connector

The confirmed module header order is `VIN`, `3Vo`, `GND`, `SCL`, `SDA`.

| `J_VEML` pin | Module label | Carrier connection | Rule |
| ---: | --- | --- | --- |
| 1 | `VIN` | direct `3V3` | Supply the breakout here, even though it accepts a wider VIN range |
| 2 | `3Vo` | **NC** | Regulator output; never drive it and never tie it to `3V3` |
| 3 | `GND` | GND | Common 0 V reference |
| 4 | `SCL` | `SCL_EXT` after `R_SCL` 330 Ω | Pulled up by `R_PU_SCL` 3.3 kΩ |
| 5 | `SDA` | `SDA_EXT` after `R_SDA` 330 Ω | Pulled up by `R_PU_SDA` 3.3 kΩ |

The VEML7700 address is `0x10`. Place `C3` 220 nF between `VIN` and GND beside this connector.

## Complete net-by-net wiring

### ideaspark ESP32 1.14 inch TFT LCD board (`U1`)

Pin names and order are confirmed from
[`docs/hardware/ideaspark-esp32-tft-pinout.png`](../../../docs/hardware/ideaspark-esp32-tft-pinout.png).
Socket pads 1–15 are the left column and 16–30 the right column, both counted from the USB/button
end:

| Pad | Name | Pad | Name |
| ---: | --- | ---: | --- |
| 1 | VIN | 16 | 3V3 |
| 2 | GND | 17 | GND |
| 3 | GPIO13 | 18 | **GPIO15 — TFT CS** |
| 4 | **GPIO12 — flash strap** | 19 | **GPIO2 — TFT DC** |
| 5 | GPIO14 | 20 | **GPIO4 — TFT RST** |
| 6 | GPIO27 | 21 | GPIO16 / RX2 |
| 7 | GPIO26 | 22 | GPIO17 / TX2 |
| 8 | GPIO25 | 23 | GPIO5 |
| 9 | GPIO33 | 24 | **GPIO18 — TFT SCLK** |
| 10 | **GPIO32 — TFT backlight** | 25 | GPIO19 |
| 11 | GPIO35 | 26 | GPIO21 / SDA |
| 12 | GPIO34 | 27 | **GPIO3 — RX0** |
| 13 | GPIO39 / VN | 28 | **GPIO1 — TX0** |
| 14 | GPIO36 / VP | 29 | GPIO22 / SCL |
| 15 | EN | 30 | **GPIO23 — TFT MOSI** |

Bold pins are off-limits to the carrier. The row spacing is still assumed to be 25.4 mm — measure
the actual board before cutting a socket or committing a hole pattern.

| Controller signal | Carrier net/use |
| --- | --- |
| `3V3` | Direct 3.3 V rail for sensors, pull-ups, buzzer and `C1`/`C2` |
| GND | Common ground; both controller GND pins join the ground rail |
| USB/`5V` or `VIN` | `+5V_USB_CONFIRMED`; PIR path only, behind default-open `JP_PIR_5V` |
| GPIO21 (pad 26) | Controller side of `R_SDA` 330 Ω |
| GPIO22 (pad 29) | Controller side of `R_SCL` 330 Ω |
| GPIO25 | `R_BEEP_IN` 2.2 kΩ to buzzer-driver base |
| GPIO33 | Active-low PIR input: Q_PIR collector plus `R_PIR_PU` 10 kΩ to 3.3 V |
| GPIO2, 4, 15, 18, 23, 32 | **Never wire.** The board's own 1.14 inch TFT drives them |
| GPIO12 | **Never wire.** Flash-voltage strap; high at boot can stop the board starting |
| GPIO1, GPIO3, EN | **Never wire.** CP2102 USB-UART and reset |

Fit `C1` 220 nF and `C2` 10–47 µF from 3.3 V to GND close to the controller sockets. `C2` is
polarized: positive to 3.3 V, negative to GND.

### External I²C branch

```text
                                        R_PU_SDA 3k3 ── +3V3
                                                     │
GPIO21 (pad 26) ── R_SDA 330 Ω ── SDA_EXT ───────────┼─ VEML7700 SDA
                                                     └─ SHT41 SDA

                                        R_PU_SCL 3k3 ── +3V3
                                                     │
GPIO22 (pad 29) ── R_SCL 330 Ω ── SCL_EXT ───────────┼─ VEML7700 SCL
                                                     └─ SHT41 SCL
```

The 330 Ω resistors belong near the ESP32 branch point. Do not put capacitors on SDA or SCL.
Operate at 100 kHz for first commissioning. Expected addresses are VEML7700 `0x10` and SHT41
`0x44`. There is no third I²C device: the display is SPI, so a scan that reports `0x3C`/`0x3D`
means something unexpected is on the bus.

### SHT41 (`J_SHT`)

Operator-confirmed order, 2026-08-28.

| Pin | Module label | Carrier connection |
| ---: | --- | --- |
| 1 | `VIN` | direct 3.3 V |
| 2 | `GND` | GND |
| 3 | `SCL` | `SCL_EXT` |
| 4 | `SDA` | `SDA_EXT` |

An earlier revision had pins 3 and 4 as SDA then SCL. Fitting the module against that order would
have crossed the bus and left every transaction failing.

Fit `C4` 220 nF directly between SHT41 VCC and GND at `J_SHT`. Place the module at a ventilated
board edge, away from the ESP32 regulator, PIR regulator, buzzer and enclosure heat sources.

### D-SUN PIR protected interface

The D-SUN header order is operator-confirmed as **GND, OUT, VCC** (2026-08-28). Its supply voltage
and output swing are still unmeasured, so keep `JP_PIR_5V` open until measured.

| Pin | Module label | Carrier net |
| ---: | --- | --- |
| 1 | `GND` | `GND` |
| 2 | `OUT` | `PIR_OUT`, into `R_PIR_IN` 10 kΩ |
| 3 | `VCC` | `PIR_5V_PROTECTED` |

An earlier revision had this header as 5 V / GND / OUT. Fitting the module against that order would
have put 5 V on its ground pin.

```text
confirmed USB/5V ── JP_PIR_5V DEFAULT OPEN ── D_PIR 1N5819 ── PIR protected VCC
                                                              │
                                                            C5 10 µF
                                                              │
                                                             GND

PIR OUT ── R_PIR_IN 10 kΩ ── Q_PIR base
Q_PIR base ── R_PIR_PD 100 kΩ ── GND
Q_PIR emitter ── GND
Q_PIR collector ── GPIO33
GPIO33 ── R_PIR_PU 10 kΩ ── 3.3 V
```

`D_PIR` is in the PIR 5 V *power* path only. Fit its band/cathode toward `PIR_5V_PROTECTED`.
The NPN prevents PIR output voltage from reaching GPIO33 directly and inverts the logic:

| PIR state | Q_PIR | GPIO33 |
| --- | --- | --- |
| inactive/output low | off | HIGH |
| active/output high | on | LOW |

Firmware must therefore treat GPIO33 LOW as motion **once this carrier exists**. Verify the actual
transistor's C/B/E order with a datasheet or diode-test measurement; do not trust flat-face folklore.

Until then the module's OUT reaches GPIO33 directly on the dev board and motion is HIGH, so
`atmosmesh-room-v1` defaults to active-high and switches with `-DATMOSMESH_ROOM_PIR_ACTIVE_LOW`.
Building the carrier means setting that flag; leaving it unset on a built carrier inverts occupancy
without failing anything, which is why the polarity is a named flag rather than a constant.

> **The protection is part of the transistor, not just the inversion.** `Q_PIR` is what keeps PIR
> output voltage off GPIO33, and this module's supply and output swing are still unmeasured. On a
> direct dev-board wire that protection is absent: measure OUT idle and triggered before trusting
> it to a GPIO, or run the module from 3.3 V if it will.

### SDS011 particulate sensor (`J_SDS`)

5 V fan and laser, **3.3 V TTL** UART at 9600 8N1. The sensor reports autonomously at 1 Hz.

| Pin | Carrier net | Goes to |
| ---: | --- | --- |
| 1 | `SDS_5V_PROTECTED` | sensor 5 V, behind `JP_SDS_5V` |
| 2 | `GND` | sensor GND |
| 3 | `SDS_RXD` | sensor **RXD**, from GPIO17/TX2 through `R_SDS_TX` 1 kΩ |
| 4 | `SDS_TXD` | sensor **TXD**, to GPIO16/RX2 through `R_SDS_RX` 1 kΩ |

> **The sensor is terminated by the 4-wire cable from its USB2TT004 adapter kit** (operator-confirmed
> 2026-08-28), not by the bare 7-pin header. That cable remaps the header, and **wire colours vary
> between kits** — so a colour is a claim, not evidence, and no colour convention is recorded here on
> purpose.
>
> **Ring the cable out instead.** With everything unpowered, put a continuity meter between each
> cable end and the pads of the sensor's own 7-pin header, and label the wire by the header position
> it reaches:
>
> | Header pin | Signal | Carrier net |
> | ---: | --- | --- |
> | 3 | 5 V | `SDS_5V_PROTECTED` |
> | 5 | GND | `GND` |
> | 6 | RXD (into the sensor) | `SDS_RXD` |
> | 7 | TXD (out of the sensor) | `SDS_TXD` |
>
> Pins 1 (NC), 2 and 4 (the PWM outputs) are unused. Four continuity readings close this item
> permanently; a photograph or a remembered colour order does not.

#### The UART must be crossed

```text
SDS011 TXD ── R_SDS_RX 1 kΩ ── GPIO16 / RX2   (pad 21)
SDS011 RXD ── R_SDS_TX 1 kΩ ── GPIO17 / TX2   (pad 22)
```

On 2026-08-17 this was wired straight through on the bench: the sensor's TXD landed on the ESP32's
TX2. Both are push-pull outputs, so they fought on one net for roughly 10 ms in every second, and
the ESP32 GPIO absolute maximum is 40 mA. `generate_project.py` now refuses to emit a
straight-through SDS011 UART, and the refusal is mutation-proved.

The 1 kΩ series resistors are the additional protection. They bound a driver fight to about
3.3 V / 1 kΩ ≈ **3.3 mA** instead of a short. At 9600 baud a bit is 104 µs, while 1 kΩ against a
few hundred pF of wiring is a couple of hundred nanoseconds, so they cost nothing in signal terms.

#### Where the 5 V comes from

`JP_5V_SRC` selects the source for `+5V_DOMAIN`, which feeds both `JP_PIR_5V` and `JP_SDS_5V`.

| Shunt | Source | Use |
| --- | --- | --- |
| pins 1–2 | `+5V_USB_CONFIRMED`, the board's `VIN` | normal standalone operation |
| pins 2–3 | `+5V_EXT`, from `J_5V_EXT` | bench bring-up and any run with the SDS011 fan |

A 3-pin header takes **exactly one** 2-pin shunt, so the two sources cannot be bridged. That
matters: bridging them would push a bench supply back into the host's USB port. `generate_project.py`
refuses to flatten this into two independent jumpers.

**Use the external supply for first power-up.** The operator's lab supply has an adjustable current
limit, and a current-limited bring-up is the cheapest protection this project can buy — it turns a
wiring error from a destroyed part into a supply that simply refuses to deliver. Set the limit just
above the expected draw, not at the supply's maximum.

Feeding the 5 V domain externally also **removes the shared-rail problem** rather than measuring
around it. `spec-comparison.md` puts the USB rail near 650 mA peak coincidence before the SDS011
fan is added, and says 700 mA is not comfortable. With the shunt on 2–3 the fan is not on the USB
rail at all. `J_5V_EXT` GND is common with the board — connect it, or nothing has a return path.

#### 5 V supply — and why there is no diode here

`JP_SDS_5V` is **DEFAULT OPEN**, the same pattern as `JP_PIR_5V`. `C6` 10 µF and `C7` 220 nF sit on
the protected side.

There is deliberately **no series Schottky**, unlike the PIR rail. The SDS011 minimum is **4.7 V**
(`spec-comparison.md`), and a 1N5819 drops roughly 0.3–0.4 V at fan current — landing under the
minimum before any USB droop is counted. The jumper provides the same isolation at zero volts.
`generate_project.py` refuses to place a diode on this rail.

**Two power facts are assumed and must be measured before this jumper is closed:**

1. `+5V_USB_CONFIRMED` comes from the board's `VIN` pin (pad 1). On many dev boards `VIN` sits
   behind a diode from USB VBUS, so it may already be near 4.6–4.7 V under load. Measure `VIN` with
   Wi-Fi active before trusting the 4.7 V floor.
2. `spec-comparison.md` already puts the shared 5 V rail at roughly **650 mA peak coincidence**
   without the SDS011, and says 700 mA is not comfortable. Adding a fan to a USB-fed carrier is a
   power-budget claim, not a given.

The SDS011 ripple specification is < 20 mV, and `C6` is **470 µF** to meet it. The electrolytic
assortment runs to 1000 µF, so this is no longer a stock compromise — an earlier 10 µF value was
never shown to hold 20 mV against a fan load. Still measure the rail with a scope during
commissioning with the HANTEK DSO2D15 recorded in
[`elektronik-inventar.md`](../../../docs/elektronik-inventar.md); a chosen value is not a measured
one. The same instrument settles the other assumption on this board: `R_PU_SDA`/`R_PU_SCL` were
sized against an *assumed* ~200 pF of bus capacitance, and the real I²C rise time can be measured
once the perfboard exists.

> **Close `JP_SDS_5V` before applying USB power, not while live.** Closing it onto a charged rail
> dumps the inrush of a discharged 470 µF into the shared 5 V supply and can brown out the ESP32.

### Buzzer (`J_BEEP`)

Operator-confirmed 2026-08-28: a no-name **Keyes 3-pin breakout**, black cylinder with a single
hole, header order **S / VCC / −**. The operator inspected the breakout on 2026-08-28: there are
**no active parts on the little PCB**, only the cylinder and the header.

That does not make it a passive element — the oscillator is inside the can, not on the board.
AtmosMesh v1 drives this same part from GPIO25 with a bare 50 ms `digitalWrite(HIGH)`
(`firmware/src/products/atmosmesh_v1.cpp`), a DC level with no `tone()` and no PWM, and
`firmware/README.md` records that as working boot and PIR-edge behaviour. A piezo disc is a
capacitor, so a DC step gives one tick per edge rather than a 50 ms tone; a passive magnetic coil
would click *and* draw far more from the pin than a GPIO should source. Only a **self-oscillating**
transducer produces the sound v1 actually ships, so no external transistor is fitted.

| Pin | Module label | Carrier net |
| ---: | --- | --- |
| 1 | `S` | `BEEP_S`, from GPIO25 through `R_BEEP_S` 100 Ω |
| 2 | (unlabelled) `VCC` | direct 3.3 V — a no-op on this module class, see below |
| 3 | `−` | GND |

```text
GPIO25 (pad 8) ── R_BEEP_S 100 Ω ── S
                              3V3 ── VCC
                              GND ── −
```

Logic is **active HIGH**, matching v1. There is no flyback diode: a self-oscillating can keeps its
own switching behind its own internals, so no inductive kick reaches the header.

On this module class the transducer sits directly between `S` and `−`, which leaves the middle pin
**not connected** and makes the 3.3 V feed a no-op. It stays wired because it is harmless and it
covers the variant that does use it, but nothing on the carrier may assume it powers anything: the
buzzer's entire operating current is sourced by GPIO25.

> **Settle the transducer type with one ohmmeter reading before Stage 5**, taken across `S` and `−`
> with the module off the carrier and nothing powered:
>
> | Reading | Transducer | Consequence |
> | --- | --- | --- |
> | open / megohms | piezo disc | needs bounded 2–4 kHz PWM, not DC; `R_BEEP_S` barely affects volume |
> | ~16–42 Ω | passive magnetic coil | DC drive is forbidden; needs PWM **and** a low-side NPN |
> | hundreds of Ω to kΩ, **and different when the leads are swapped** | active, internal oscillator | the expected result; v1's DC drive is correct |
>
> The swapped-lead asymmetry is the tell — it is the ohmmeter seeing a semiconductor junction inside
> the can. The reasoning above says this *should* read as active. The measurement is what turns that
> inference into a fact, and it costs one probe touch.

> **If the buzzer is silent, do not raise the drive — lower the resistor, one E24 step at a time.**
> Some Keyes buzzer boards put the sounder straight across `S` and `−` with no onboard transistor.
> On such a module 100 Ω drops most of the supply and it will barely click. Step down through the
> kit — 100 → 47 → 22 Ω — measuring the current into `S` during a beep at each step, and stop at the
> largest value that gives acceptable volume. The current must stay **under 20 mA**; the ESP32 GPIO
> absolute maximum is 40 mA. A wire link is the last resort, not the first move, and if even 22 Ω
> exceeds 20 mA then fit a low-side NPN on the `−` pin rather than driving `S` harder.

`R_BEEP_S` starts at 100 Ω deliberately: the protective value is the default, and the bench
measurement is what authorises reducing it.

### Stage 1 — controller alone

1. Keep all modules removed and `JP_PIR_5V` open.
2. Power the Ideaspark board through USB only, outside the carrier. Measure its labelled `3V3`, GND
   and possible USB/5 V pin. The 3.3 V measurement must be approximately 3.3 V.
3. Remove USB, insert the controller in the verified orientation, and power by USB again.
4. Measure `TP_3V3`, `TP_5V`, SDA and SCL relative to `TP_GND`. Stop immediately if 5 V appears on
   3.3 V, SDA, SCL, GPIO25 or GPIO33.
5. Verify the I²C bus is idle-high and that a scan finds no device before the sensors are fitted.

### Stage 2 — VEML7700

1. Remove USB. Connect `VIN`, `GND`, `SCL`, `SDA`; leave `3Vo` physically isolated.
2. Power USB and verify idle SDA/SCL near 3.3 V.
3. Scan for `0x10` and read a stable lux response under dark and bright conditions.
4. Confirm the TFT backlight does not dominate the lux result; add or adjust the opaque divider.

### Stage 3 — SHT41

1. Remove USB, attach the SHT41 using its photographed pin order, then restore USB.
2. Scan for both `0x10` and `0x44`. Missing devices must be reported as unavailable,
   never numeric zero.
3. Compare temperature/humidity to a nearby reference after thermal settling. Touching or breathing
   on the sensor may be used only as a response test, not calibration evidence.

### Stage 4 — PIR

1. Characterize the PIR separately: required supply, idle output and active output voltage.
2. With USB removed, connect the PIR and close `JP_PIR_5V` only if its 5 V requirement and the
   controller's USB/5 V source pin are confirmed.
3. Restore USB. Measure `PIR_5V_PROTECTED`, Q_PIR base and GPIO33 in idle and motion states.
4. Confirm GPIO33 never exceeds 3.3 V and logic is active-low. Exercise repeated motion and timeout.

### Stage 5 — buzzer

1. Leave `D_BEEP` empty. Connect the buzzer with confirmed polarity and 3.3 V supply.
2. Drive GPIO25 for at most 250 ms. Identify active versus passive behavior as described above.
3. Measure current and verify Q_BEEP remains cool. For passive piezo, test bounded 2–4 kHz PWM.

### Stage 6 — combined run

Run all functions for at least 30 minutes while checking 3.3 V stability, sensor presence, TFT
updates, PIR transitions, buzzer behavior and component temperature. Then test USB power removal and
restoration. Do not leave the station unattended until this passes and the wiring photographs are
reviewed.

## Stop conditions

Disconnect USB immediately for any wrong rail voltage, hot part, smell, unstable reset loop, 5 V on
an I²C/GPIO net, missing ground, unexpected continuity to `3Vo`, or transistor whose lead order is
uncertain. Correct and repeat from the unpowered checks; never troubleshoot by moving live wires.

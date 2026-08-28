# ROOM-01 wiring, protection, assembly and commissioning plan

This is the complete construction contract for the provisional 60×80 mm room carrier. It defines
the intended nets and safe build order, but it is **not fabrication approval**. The exact Ideaspark
controller, SHT41, D-SUN PIR and buzzer still require readable front/back photographs and physical
measurements. Never infer pin order or connector orientation from a generic product photograph.

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

The PCB/BOM uses 220 nF for `C1`, `C3` and `C4` because those parts are available. Values from
47 nF through 220 nF are electrically acceptable here; do not substitute electrolytics for these
high-frequency ceramic capacitors.

### The carrier supplies the I²C pull-ups

Nothing on the controller board pulls SDA or SCL up: its display is SPI. `R_PU_SDA` and `R_PU_SCL`,
4.7 kΩ from `SDA_EXT`/`SCL_EXT` to 3.3 V, are therefore **required**, not optional. Without them a
build populated with only the SHT41, or with a breakout whose pull-ups are unfitted, leaves the bus
floating and every transaction fails.

They sit on the sensor side of `R_SDA`/`R_SCL` so the 330 Ω series resistors remain a fault-current
limit rather than part of the pull-up divider. If both breakouts carry their own 10 kΩ pull-ups the
parallel result is about 2.4 kΩ — roughly 1.4 mA at 3.3 V, inside the I²C 3 mA sink limit — so keep
them fitted. After assembly, measure the unpowered resistance from SDA to 3.3 V and SCL to 3.3 V,
then verify idle-high voltage when powered.

### Five-pin VEML7700 connector

The confirmed module header order is `VIN`, `3Vo`, `GND`, `SCL`, `SDA`.

| `J_VEML` pin | Module label | Carrier connection | Rule |
| ---: | --- | --- | --- |
| 1 | `VIN` | direct `3V3` | Supply the breakout here, even though it accepts a wider VIN range |
| 2 | `3Vo` | **NC** | Regulator output; never drive it and never tie it to `3V3` |
| 3 | `GND` | GND | Common 0 V reference |
| 4 | `SCL` | `SCL_EXT` after `R_SCL` 330 Ω | Pulled up by `R_PU_SCL` 4.7 kΩ |
| 5 | `SDA` | `SDA_EXT` after `R_SDA` 330 Ω | Pulled up by `R_PU_SDA` 4.7 kΩ |

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
                                        R_PU_SDA 4k7 ── +3V3
                                                     │
GPIO21 (pad 26) ── R_SDA 330 Ω ── SDA_EXT ───────────┼─ VEML7700 SDA
                                                     └─ SHT41 SDA

                                        R_PU_SCL 4k7 ── +3V3
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

Firmware must therefore treat GPIO33 LOW as motion. Verify the actual transistor's C/B/E order
with a datasheet or diode-test measurement; do not trust flat-face folklore.

### Buzzer driver

```text
3.3 V ── buzzer +
buzzer - ── Q_BEEP collector
Q_BEEP emitter ── GND
GPIO25 ── R_BEEP_IN 2.2 kΩ ── Q_BEEP base
Q_BEEP base ── R_BEEP_PD 100 kΩ ── GND
```

Keep the transistor even though the black cylinder is likely piezo. It protects GPIO25 from load
current and keeps the buzzer off during reset. `D_BEEP` is **DNP** for a piezo buzzer. Fit a 1N4001
only after confirming a magnetic buzzer; then place it across the buzzer with band/cathode at 3.3 V
and anode at Q_BEEP collector.

Initial identification drive: 3.3 V only, 250 ms maximum. A continuous tone with steady HIGH
suggests an active buzzer; only clicks suggest a passive piezo requiring roughly 2–4 kHz PWM.
Disconnect if the transistor or buzzer warms or current is unexpectedly high.

## Double-sided through-hole board layout

- Mount the Ideaspark board in female sockets; do not solder the controller directly.
- Leave the antenna end at or beyond the carrier edge. No wire, copper fill, component, mounting
  hardware or enclosure metal may occupy the marked antenna region.
- Put `J_VEML` at the light-facing edge and add an opaque divider between sensor and TFT.
- Put `J_SHT` at a ventilated edge and thermally isolate it from the controller and buzzer.
- Put the PIR and beeper at enclosure edges; keep the 5 V PIR region visually separated.
- Use the component side for sockets, keyed connectors and insulated crossovers. Use the solder side
  for short point-to-point links and wider ground/power buses.
- Use black for GND, orange for 3.3 V, red for confirmed 5 V, blue for SDA and yellow for SCL.
- Label `VIN`, `3Vo NC`, `GND`, `SCL`, `SDA`, `3V3`, `5V`, and both transistor orientations on silk
  or with permanent wire labels.
- Check whether the chosen protoboard has isolated pads or connected strips. Cut unwanted strips
  before fitting components and confirm each cut with a meter.

## Assembly sequence

1. Print the schematic and BOM. Mark the physical board's component side and solder side.
2. Measure the Ideaspark row spacing, overall outline, antenna end and USB overhang. Compare every
   controller label against `U1`; stop on any mismatch.
3. Photograph both sides of SHT41, PIR and buzzer markings. Record each connector order.
4. Fit only the four mounting holes, controller sockets and empty keyed module connectors. Check
   mechanical clearance without inserting modules.
5. Fit resistors and the default-open jumper. Meter every value before soldering; verify no solder
   bridge across `JP_PIR_5V`.
6. Identify transistor leads, then fit Q_PIR and Q_BEEP. Record the measured C/B/E order.
7. Fit `D_PIR`; verify band orientation twice. Leave `D_BEEP` empty and mark it DNP.
8. Fit `C1`, `C3`, `C4` as 220 nF. Fit polarized `C2` and `C5` only after checking polarity.
9. Install ground and 3.3 V buses. Install the physically separated 5 V PIR path last.
10. Wire I²C and signal nets. Keep exposed solder-side links short; use insulated wire for every
    crossing. Do not route beneath the antenna.
11. Clean flux, inspect both faces with magnification and photograph the completed unpowered board.

## Unpowered acceptance checks

Perform these with the ESP32 and every module removed and `JP_PIR_5V` open.

1. Confirm no continuity between 5 V and 3.3 V.
2. Confirm neither power rail is shorted to GND. Capacitor charging may make the resistance climb;
   a persistent near-zero reading is a fault.
3. Confirm `J_VEML.2` (`3Vo`) has no continuity to any carrier net.
4. Confirm `J_VEML.1` and `J_SHT.1` connect directly to `TP_3V3` without diode/resistor drop.
5. Confirm GPIO21 reaches external SDA only through 330 Ω; GPIO22 reaches external SCL only
   through 330 Ω. Neither may connect to 5 V.
6. Confirm GPIO33 connects only to Q_PIR collector and the 10 kΩ 3.3 V pull-up.
7. Confirm GPIO25 reaches only the 2.2 kΩ buzzer base resistor.
8. Confirm transistor emitters and pull-down bottoms reach GND.
9. Confirm `D_PIR` direction and both electrolytic polarities.
10. Confirm every reserved boot/UART pin is unloaded.

## Staged power and commissioning

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

# ROOM-01 wiring, protection, assembly and commissioning plan

This is the complete construction contract for the provisional 60×80 mm room carrier. It defines
the intended nets and safe build order, but it is **not fabrication approval**. The exact Ideaspark
controller, SHT41, D-SUN PIR and buzzer still require readable front/back photographs and physical
measurements. Never infer pin order or connector orientation from a generic product photograph.

## Design decisions

### One 3.3 V domain for the controller and I²C sensors

The VEML7700 and SHT41 are powered directly from the Ideaspark board's confirmed `3V3` output. No
series diode and no 50 Ω sensor-supply resistor network is fitted. A supply drop would put the
sensors below the integrated OLED's 3.3 V I²C pull-up voltage. During low supply or transient load,
SDA/SCL could then exceed a sensor's `VDD + input-clamp margin` and inject current through its input
protection. Direct 3.3 V keeps sensor VDD and every I²C pull-up referenced to the same rail.

Protection is instead provided by keyed connectors, staged measurements, local decoupling and
330 Ω series resistors in the *external* SDA/SCL branch. Those resistors limit current during a
signal short or wiring mistake and damp fast edges. They do **not** create appreciable measurement
noise. `C1`, `C3` and `C4` decouple power; their job is different from the series resistors.

The PCB/BOM uses 220 nF for `C1`, `C3` and `C4` because those parts are available. Values from
47 nF through 220 nF are electrically acceptable here; do not substitute electrolytics for these
high-frequency ceramic capacitors.

### No extra I²C pull-ups

The integrated OLED and the confirmed Adafruit-style VEML7700 breakout have onboard pull-ups.
Additional pull-ups are deliberately omitted. Too many parallel pull-ups lower the effective
resistance and force devices to sink more current. After assembly, measure the unpowered resistance
from SDA to 3.3 V and SCL to 3.3 V, then verify idle-high voltage when powered.

### Five-pin VEML7700 connector

The confirmed module header order is `VIN`, `3Vo`, `GND`, `SCL`, `SDA`.

| `J_VEML` pin | Module label | Carrier connection | Rule |
| ---: | --- | --- | --- |
| 1 | `VIN` | direct `3V3` | Supply the breakout here, even though it accepts a wider VIN range |
| 2 | `3Vo` | **NC** | Regulator output; never drive it and never tie it to `3V3` |
| 3 | `GND` | GND | Common 0 V reference |
| 4 | `SCL` | `SCL_EXT` after `R_SCL` 330 Ω | GPIO22 side remains on the controller/OLED bus |
| 5 | `SDA` | `SDA_EXT` after `R_SDA` 330 Ω | GPIO21 side remains on the controller/OLED bus |

The VEML7700 address is `0x10`. Place `C3` 220 nF between `VIN` and GND beside this connector.

## Complete net-by-net wiring

### Ideaspark ESP32-WROOM-32 with integrated OLED (`U1`, provisional)

The carrier assumes the common 30-pin, 25.4 mm row-spacing layout only to make a reviewable KiCad
starting point. Confirm actual row spacing and all named pins before routing or fitting sockets.

| Controller signal | Carrier net/use |
| --- | --- |
| `3V3` | Direct 3.3 V rail for sensors, pull-ups, buzzer and `C1`/`C2` |
| GND | Common ground; both controller GND pins join the ground rail |
| USB/`5V` or `VIN` | `+5V_USB_CONFIRMED`; PIR path only, behind default-open `JP_PIR_5V` |
| GPIO21 | OLED SDA plus controller side of `R_SDA` 330 Ω |
| GPIO22 | OLED SCL plus controller side of `R_SCL` 330 Ω |
| GPIO25 | `R_BEEP_IN` 2.2 kΩ to buzzer-driver base |
| GPIO33 | Active-low PIR input: Q_PIR collector plus `R_PIR_PU` 10 kΩ to 3.3 V |
| GPIO0, 1, 2, 3, 12, 15 | Leave unloaded; boot/programming pins are not carrier signals |

Fit `C1` 220 nF and `C2` 10–47 µF from 3.3 V to GND close to the controller sockets. `C2` is
polarized: positive to 3.3 V, negative to GND.

### External I²C branch

```text
GPIO21 / onboard OLED SDA ── R_SDA 330 Ω ── SDA_EXT ─┬─ VEML7700 SDA
                                                      └─ SHT41 SDA

GPIO22 / onboard OLED SCL ── R_SCL 330 Ω ── SCL_EXT ─┬─ VEML7700 SCL
                                                      └─ SHT41 SCL
```

The 330 Ω resistors belong near the ESP32 branch point. Do not put capacitors on SDA or SCL.
Operate at 100 kHz for first commissioning. Expected addresses are VEML7700 `0x10`, SHT41 `0x44`,
and normally OLED `0x3C` or `0x3D`; scan and record the actual OLED address.

### SHT41 (`J_SHT`, carrier order until its photograph is confirmed)

| Pin | Carrier connection |
| ---: | --- |
| 1 | direct 3.3 V |
| 2 | GND |
| 3 | `SDA_EXT` |
| 4 | `SCL_EXT` |

Fit `C4` 220 nF directly between SHT41 VCC and GND at `J_SHT`. Place the module at a ventilated
board edge, away from the ESP32 regulator, PIR regulator, buzzer and enclosure heat sources.

### D-SUN PIR protected interface

The PIR power pin and output behavior are not yet confirmed. Keep `JP_PIR_5V` open until measured.

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
- Put `J_VEML` at the light-facing edge and add an opaque divider between sensor and OLED.
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
5. Verify only the onboard OLED on I²C and record its address.

### Stage 2 — VEML7700

1. Remove USB. Connect `VIN`, `GND`, `SCL`, `SDA`; leave `3Vo` physically isolated.
2. Power USB and verify idle SDA/SCL near 3.3 V.
3. Scan for `0x10` and read a stable lux response under dark and bright conditions.
4. Confirm OLED light does not dominate the lux result; add or adjust the opaque divider.

### Stage 3 — SHT41

1. Remove USB, attach the SHT41 using its photographed pin order, then restore USB.
2. Scan for both `0x10` and `0x44` plus the OLED. Missing devices must be reported as unavailable,
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

Run all functions for at least 30 minutes while checking 3.3 V stability, sensor presence, OLED
updates, PIR transitions, buzzer behavior and component temperature. Then test USB power removal and
restoration. Do not leave the station unattended until this passes and the wiring photographs are
reviewed.

## Stop conditions

Disconnect USB immediately for any wrong rail voltage, hot part, smell, unstable reset loop, 5 V on
an I²C/GPIO net, missing ground, unexpected continuity to `3Vo`, or transistor whose lead order is
uncertain. Correct and repeat from the unpowered checks; never troubleshoot by moving live wires.

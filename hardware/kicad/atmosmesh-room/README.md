# AtmosMesh Room carrier — provisional design

**DO NOT ENERGISE THIS BOARD YET.** The controller and every module connector order are now
identified, but no supply voltage or output swing has been measured on the 5 V modules. This project is a protected, reviewable starting point, not a wiring
approval.

The controller is the **ideaspark ESP32 1.14 inch TFT LCD board** (ESP32-WROOM-32), confirmed from
[`docs/hardware/ideaspark-esp32-tft-pinout.png`](../../../docs/hardware/ideaspark-esp32-tft-pinout.png).
Its 30 pins and their order are therefore no longer provisional: pads 1–15 are the left column and
16–30 the right column, both counted from the USB/button end. What remains unmeasured is the
physical **row spacing**: 25.4 mm is still an assumption, so MEASURE THE ROW SPACING before
cutting a socket or committing a hole pattern.

**The integrated display is SPI, not I²C.** It occupies GPIO23 (MOSI), GPIO18 (SCLK), GPIO15 (CS),
GPIO2 (DC), GPIO4 (RST) and GPIO32 (backlight). GPIO21/22 carry no onboard device, so the carrier
supplies the only I²C pull-ups it has.

## Files

| File | Purpose |
| --- | --- |
| `atmosmesh-room.kicad_pro` | KiCad 10 project; open this file |
| `atmosmesh-room.kicad_sch` | Reviewable electrical contract |
| `atmosmesh-room.kicad_pcb` | **Parked.** Kept only as a netlist/silkscreen cross-check — not the build target |
| `perfboard.md` | **The build target**: 31×27 hole plan, orientation rules and build order |
| `validate_room_perfboard.py` | Deterministic structural check on the perfboard plan |
| `room.pretty/`, `fp-lib-table`, `sym-lib-table` | Local THT library contract |
| `BOM.csv` | Values, fitting policy and verification notes |
| `wiring.md` | Connector contract and staged pre-power checks |
| `validate_room_carrier.py` | Deterministic structural safety check |
| `generate_project.py` | Reproducible project generator; KiCad source remains editable |
| `requirements-generator.txt` | Pinned generator-only Python dependency |

## Safety architecture

- ESP32 GPIO and `3V3` must **never receive 5 V**.
- **Never wire GPIO23, 18, 15, 2, 4 or 32** — the dev board's own TFT drives them. GPIO12 is the
  flash-voltage strap (high at boot can stop the board starting) and GPIO1/GPIO3 are the CP2102
  USB-UART used by `task flash` and `task monitor`. `generate_project.py` refuses to emit a net on
  any of them and `validate_room_carrier.py` fails if one appears.
- GPIO21/SDA and GPIO22/SCL reach the external sensor branch through 330 Ω series resistors. Those
  resistors limit fault current and damp edges; they are not a measurement noise source.
- `R_PU_SDA` and `R_PU_SCL` (3.3 kΩ to 3.3 V) are the bus pull-ups. They are required because the
  display is SPI: nothing on the controller board pulls I²C up. They sit on the sensor side of the
  330 Ω resistors so the series resistance stays a fault limit rather than part of the pull-up
  divider. With both breakouts populated the parallel result is roughly 2.0 kΩ, about 1.6 mA at
  3.3 V, inside the I²C 3 mA sink limit, so they stay fitted rather than DNP. Every resistor value
  on this board is justified in `wiring.md`'s sizing table.
- Both sensors receive direct 3.3 V and local 220 nF decoupling. There is deliberately no diode or
  resistor in a sensor supply: a breakout's pull-ups reference its own VDD, so dropping sensor VDD
  drags the pull-ups down with it and buys nothing, while adding a droop path under the SHT41's
  measurement current bursts. The build uses 100 nF for `C1`, `C3`, `C4`
  and `C7` — the confirmed ceramic stock value, inside D-022's own 47–220 nF range.
- The confirmed five-pin VEML7700 connector is `VIN`, `3Vo`, `GND`, `SCL`, `SDA`: `VIN` receives
  3.3 V and `3Vo` is regulator output and remains NC.
- PIR power can come only from a confirmed USB/5 V pin, through `JP_PIR_5V` which is **DEFAULT
  OPEN**, then a 1N5819. PIR OUT drives a 2N3904 through 10 kΩ; GPIO33 sees only the collector and a
  10 kΩ pull-up to 3.3 V. Motion logic is active-low.
- The buzzer is a confirmed Keyes 3-pin module, header order `S`/`VCC`/`−`. GPIO25 drives `S`
  directly through `R_BEEP_S` 100 Ω, active HIGH, exactly as AtmosMesh v1 drives the same part. The
  low-side NPN and flyback diode are gone: an internally-driven module keeps the inductive kick
  behind its own transistor. If the buzzer stays silent the module may be unbuffered — see the
  measurement rule in `wiring.md` before changing anything.
- `JP_5V_SRC` selects where `+5V_DOMAIN` comes from: the board's `VIN` (pins 1–2) or an external
  bench supply on `J_5V_EXT` (pins 2–3). A 3-pin header takes exactly one shunt, so the two sources
  can never be bridged back into the host's USB port. Use the external, current-limited supply for
  first power-up and whenever the SDS011 fan runs.
- The SDS011 takes 5 V through `JP_SDS_5V`, **DEFAULT OPEN**, with **no series diode**: its minimum
  is 4.7 V and a Schottky at fan current drops below that. Its UART is crossed — sensor TXD to
  GPIO16/RX2, GPIO17/TX2 to sensor RXD — each through a 1 kΩ series resistor that bounds a driver
  fight to about 3.3 mA. The generator refuses both a straight-through UART and a diode on that
  rail.
- Three indicator LEDs show which domains are live. `D_LED_SDS` and `D_LED_PIR` draw **through**
  their own default-open jumpers, so a lit LED is direct evidence that the jumper it reports on is
  closed. Use red or green: a blue or white LED's ~3.0 V forward drop leaves too little across
  `R_LED_3V3` on the 3.3 V rail.
- No Zener clamp is used. This is now positively evidenced rather than assumed: the Zener
  assortment in `docs/elektronik-inventar.md` is the 1N47xx series starting at **1N4733 = 5.1 V**,
  which clamps far too late to protect a 3.3 V GPIO. A 3.3 V part (1N4728) is not held.

## The build target is perfboard

The operator builds this by hand on a **31 × 27 hole** perfboard, so no board is fabricated. The
schematic is the electrical contract and [`perfboard.md`](perfboard.md) is the build plan.

`atmosmesh-room.kicad_pcb` is **parked**: it is still generated from the netlist and still carries
the safety silkscreen, which is why its checks remain in `validate_room_carrier.py`, but its
placement is not maintained as a fabrication candidate. Do not order it.

## Placement and routing state

The VEML7700 sits at a board edge and must be optically shielded from the TFT backlight. The SHT41 sits at
the opposite ventilated edge, away from the ESP32 regulator, PIR electronics and buzzer. The 5 V
PIR block is fenced at the right edge. The antenna clearance is marked on `User.Drawings` and silk;
its final copper keepout/overhang cannot be fixed until the exact board orientation is photographed.

Copper is intentionally unrouted because the Ideaspark row spacing and orientation, plus each
module connector order, are unverified. This prevents a plausible-looking provisional drawing
from being mistaken for a fabrication-ready board. Do not "finish the ratsnest" by assuming a
generic ESP32 DevKit image.

The complete net table, double-sided through-hole placement guidance, assembly order, unpowered
checks, staged commissioning and stop conditions are in [`wiring.md`](wiring.md).

## Validation

From this directory:

```sh
python3.12 -m venv .generator-venv
.generator-venv/bin/pip install -r requirements-generator.txt
.generator-venv/bin/python generate_project.py
python3 validate_room_carrier.py
kicad-cli sch erc --severity-all --exit-code-violations \
  -o atmosmesh-room-erc.rpt atmosmesh-room.kicad_sch
kicad-cli pcb drc --severity-all --schematic-parity --refill-zones \
  -o atmosmesh-room-drc.rpt atmosmesh-room.kicad_pcb
kicad-cli sch export pdf -o atmosmesh-room-schematic.pdf atmosmesh-room.kicad_sch
```

DRC reports the deliberately unrouted nets as unconnected; schematic parity must remain clean and
there must be no other DRC violation. Routing stays blocked until the exact physical parts are
verified.

# AtmosMesh Room carrier — provisional design

**DO NOT FABRICATE OR ENERGISE THIS BOARD YET.** The exact Ideaspark board, module pin orders and
beeper type have not been established from readable **front and back photographs**. This project is
a protected, reviewable starting point, not a wiring approval.

The board is a 60×80 mm, 1.6 mm, two-copper-layer carrier for through-hole assembly. The Ideaspark
ESP32/OLED board is represented by two provisional 1×15 female sockets at 25.4 mm row spacing.
Measure the actual board, photograph both sides and compare every printed pin before ordering.

## Files

| File | Purpose |
| --- | --- |
| `atmosmesh-room.kicad_pro` | KiCad 10 project; open this file |
| `atmosmesh-room.kicad_sch` | Reviewable electrical contract |
| `atmosmesh-room.kicad_pcb` | 60×80 mm placement and unrouted safety-reviewed netlist |
| `room.pretty/`, `fp-lib-table`, `sym-lib-table` | Local THT library contract |
| `BOM.csv` | Values, fitting policy and verification notes |
| `wiring.md` | Connector contract and staged pre-power checks |
| `validate_room_carrier.py` | Deterministic structural safety check |
| `generate_project.py` | Reproducible project generator; KiCad source remains editable |
| `requirements-generator.txt` | Pinned generator-only Python dependency |

## Safety architecture

- ESP32 GPIO and `3V3` must **never receive 5 V**.
- The onboard OLED remains directly on GPIO21/SDA and GPIO22/SCL. Only the external I²C branch
  passes through 330 Ω series resistors.
- Both sensors receive direct 3.3 V and local 220 nF decoupling. There is deliberately no diode or
  resistor in a sensor supply: lowering sensor VDD while the onboard OLED pulls I²C to 3.3 V could
  forward-bias a sensor input clamp. The build uses 220 nF for `C1`, `C3` and `C4`; 47–220 nF is
  electrically acceptable for these high-frequency decouplers.
- The confirmed five-pin VEML7700 connector is `VIN`, `3Vo`, `GND`, `SCL`, `SDA`: `VIN` receives
  3.3 V and `3Vo` is regulator output and remains NC. Its onboard pull-ups mean no extra I²C
  pull-ups are fitted.
- PIR power can come only from a confirmed USB/5 V pin, through `JP_PIR_5V` which is **DEFAULT
  OPEN**, then a 1N5819. PIR OUT drives a 2N3904 through 10 kΩ; GPIO33 sees only the collector and a
  10 kΩ pull-up to 3.3 V. Motion logic is active-low.
- The likely piezo buzzer starts from 3.3 V and uses an NPN low-side driver. `D_BEEP` is DNP for a
  piezo; fit 1N4001 only if a magnetic buzzer is positively identified, with its band/cathode at
  3.3 V.
- No Zener clamp is used. The known inventory starts at 5.1 V, which clamps too late for a 3.3 V
  GPIO.

## Placement and routing state

The VEML7700 sits at a board edge and must be optically shielded from OLED light. The SHT41 sits at
the opposite ventilated edge, away from the ESP32 regulator, PIR electronics and buzzer. The 5 V
PIR block is fenced at the right edge. The antenna clearance is marked on `User.Drawings` and silk;
its final copper keepout/overhang cannot be fixed until the exact board orientation is photographed.

Copper is intentionally unrouted because the Ideaspark row spacing, orientation and pin order, plus
each module connector order, are unverified. This prevents a plausible-looking provisional drawing
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

# AtmosMesh Aura — sensor carrier board

The pigtail is being replaced by a carrier. This document says what the board must do **today**
(two sensors) and what it must leave room for **later**, so that adding a sensor is a plug rather
than a redesign.

> Written after three separate bus failures on hand-wired pigtails in one afternoon, including one
> that killed both modules at once. The carrier exists because loose wire is the least reliable
> part of this build, not because the circuit is complicated.

## Fitted now

| Device | Address | Supply | Notes |
| --- | --- | ---: | --- |
| SGP41 | `0x59` | 3V3 | VOC + NOx Index. Pulsed hotplate |
| BME280 | `0x76` | 3V3 | temperature, humidity, pressure. Also the SGP41's compensation source |

Bus: `GPIO27` = SDA, `GPIO22` = SCL, 100 kHz, from **CN1**.

## The pin budget — why the board is shaped this way

The CYD leaves exactly three pins, and one of them cannot drive anything:

| Header | Pins | Usable for |
| --- | --- | --- |
| **CN1** (4-pin) | GND, **GPIO22**, **GPIO27**, 3V3 | the I²C bus. This is the whole sensor budget |
| P3 (4-pin) | GND, **GPIO35**, GPIO22, GPIO21 | GPIO35 as a spare **input only**; GPIO21 is the backlight |

**There is no 5 V on either header.** Anything needing 5 V has to take it from the board's USB
input rail, which is a soldered tap, not a connector.

## Leave room for these four

Not fitted, no parts committed, but each is cheap to allow for now and expensive to retrofit.

| Future part | What it needs from the carrier | Address |
| --- | --- | --- |
| **SCD41** — real NDIR CO₂ | a 3V3 rail that tolerates a **205 mA** pulse, and 100 µF of local bulk | `0x62` |
| **SPS30** — particulates | **5 V**, ~60 mA plus fan inrush, and 100 µF | `0x69` |
| **VEML7700** — ambient light | nothing special; one more I²C drop | `0x10` |
| **Presence radar** | **GPIO35** routed to a header, and 5 V if it is an LD2450 | — |

None of these collide with each other or with the two fitted parts.

### The radar is the interesting constraint

GPIO35 is input-only, so a radar has to be one of:

- **LD2410S** — its `OT2` pin is a plain digital presence output. One wire into GPIO35, no UART.
  This is the clean fit.
- **LD2450** — UART only, but GPIO35 can serve as **RX**: the LD2450 streams target data unprompted
  and needs nothing sent back. Costs the 5 V tap as well.

Presence is worth allowing for beyond the reading itself: it lets the screen blank in an empty
room, which is what makes a bright panel acceptable on a bedside table.

## What the board must do

1. **Break the bus out three more times.** Four identical 4-pin drops (`3V3 / GND / SDA / SCL`) —
   two populated, two spare. A future module is then a plug.
2. **One set of pull-ups, not five.** Each breakout carries its own ~10 kΩ. Two in parallel is
   ~5 kΩ and fine; five would be ~2 kΩ, legal but pointlessly stiff. Fit **one** 4.7 kΩ pair on the
   carrier and **remove the on-module pull-ups** from every module but one. Decide this once, here,
   rather than per module later.
3. **Route GPIO35 from P3 to a 3-pin header** (`3V3 / GND / GPIO35`). Unpopulated is fine; the
   routing is what is expensive later.
4. **Provide a 5 V pad, fed from the CYD's USB rail** — unpopulated, clearly labelled, and
   **metered before anything is connected to it**. Its location on the CYD is not yet confirmed and
   must be found on the actual board.
5. **Footprints for bulk capacitance** beside each drop: 100 µF plus 100 nF. Fit only the 10 µF at
   the SGP41 today; the rest stay empty until a part needs them.
6. **Keep the BME280 off the board, on a short lead.** Everything else can sit on the carrier, but
   the temperature reading is the one a recipient will check against their own senses, and the
   ESP32, TFT driver and backlight all run warm. This is AU-09's whole subject.

## Power, when the board is fully populated

| Load | Typical | Peak |
| --- | ---: | ---: |
| ESP32 + Wi-Fi TX | ~120 mA | ~250 mA |
| Display + backlight | ~100 mA | ~150 mA |
| SCD41 | ~15 mA | **205 mA** |
| SPS30 | ~60 mA | inrush on fan start |
| Radar | ~1 mA | ~118 mA |

Coincident peaks exceed a 500 mA USB2 port. A fully-populated carrier wants a proper supply and
its **own 3.3 V regulator fed from 5 V** — not the CYD's 3V3, which is already carrying the ESP32
and the panel. Today, with two low-power sensors, CN1's 3V3 is entirely adequate; the regulator
footprint is for later.

## Verify before first power

Unchanged from the pigtail procedure, and it is not optional — it is what three dead buses in one
afternoon cost:

1. CN1 unplugged, board on USB: meter each CN1 pin against the USB shell. Exactly one pin at
   **~3.3 V** (pin 4) and one at **0 V** (pin 1).
2. Confirm the carrier's connector **cannot be seated reversed**, or key it so it cannot. A flipped
   4-pin JST puts 3V3 on GND and destroys every module on the board at once.
3. With the carrier connected and sensors fitted, the boot scan must print `0x59` and `0x76` and
   nothing else.

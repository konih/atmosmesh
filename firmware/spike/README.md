# AtmosMesh Aura — one-day bring-up spike

**This is not the product and it is not a second firmware.** It is a throwaway ESPHome
configuration whose only job is to answer, in one day and with no C++ written, five questions that
the [Aura design](../../docs/design/atmosmesh-aura.md) otherwise leaves open until story AU-09.

Chosen by the operator on 2026-09-20 in response to the
[adversarial review](../../agent-context/inbox/2026-09-20-aura-adversarial-review.md), whose
verdict was BLOCK. The review's CRITICAL C3 was that ESPHome had never been evaluated, and an
argument about it cannot be settled by more argument. This spike settles it with a board.

Delete this directory once the questions below have answers recorded in `inventory.md`.

## What it answers

| # | Question | Otherwise answered at | How this answers it |
| --- | --- | --- | --- |
| Q1 | Which display controller is fitted, ILI9341 or ST7789? | AU-01 | A full-width RGB test card. Geometry and colour identify the part empirically |
| Q2 | Does the CN1 I²C bus work, and do all three sensors ACK? | AU-02 | `scan: true` prints every responding address at boot |
| Q3 | How bad is the self-heating offset? | **AU-09** | A live template sensor showing AHT20 (on the hot gas module) minus BME280 (on the pigtail) |
| Q4 | Is typing a password on resistive touch tolerable for a gift? | AU-05 | An LVGL keyboard and text field. A human types a real password and judges |
| Q5 | Is ESPHome's LVGL good enough to skip seven stories of C++? | never | You look at it |

Q3 and Q5 are the valuable ones. Q3 is the risk §4.1 calls *"a broken gift"* and the plan does not
test until story nine. Q5 is the BLOCK.

## Running it

ESPHome is **not** part of this repo's toolchain and this spike does not make it one. It is
installed in the workspace tooling directory alongside PlatformIO:

```bash
# one-off, if the venv is not there yet
python3 -m venv /home/koni/Projects/PlatformRelay/.tooling/python/esphome
/home/koni/Projects/PlatformRelay/.tooling/python/esphome/bin/pip install esphome

# validate the config without a board
.tooling/python/esphome/bin/esphome config firmware/spike/aura-bringup.yaml

# flash over USB — use ONE of the two USB sockets, never both
.tooling/python/esphome/bin/esphome run firmware/spike/aura-bringup.yaml
```

**Before flashing, take the stock backup seriously.** Unit 1's is already at
`PlatformRelay/.tooling/firmware-backups/esp32-2432s028_20500d34463c_stock-factory_2026-09-19.bin`.
If you use the *other* unit, back it up first — its MAC is not yet recorded anywhere.

Run it **once with no Wi-Fi in range or the AP unjoined.** That is requirement G4, and the sensors
and display must work anyway. If they do not, that is a finding.

## Wiring for the spike

Only the CN1 4-pin header is used. Nothing here needs 5 V and nothing has a fan.

| CN1 pin | To |
| --- | --- |
| GND | both sensor modules' GND |
| GPIO27 | both modules' SDA |
| GPIO22 | both modules' SCL |
| 3V3 | both modules' VIN/VCC |

Put the **BME280 at the far end of the pigtail**, as physically distant from the board as the cable
allows, and leave the **ENS160+AHT20 module wherever is convenient** — the point of Q3 is to measure
the difference between a sensor in still, warm air and one out in the room.

Expected I²C scan: `0x38` (AHT20, fixed), `0x52` or `0x53` (ENS160), `0x76` (BME280, SDO low).
A missing address is a wiring or pull-up problem, not a dead sensor. The breakouts carry their own
10 kΩ pull-ups, so do not add more until a scan has actually failed.

## If the display stays black or wrong

In order, because the cheap checks come first:

1. **Blank, backlight on** — the panel is being lit but not addressed. Suspect the SPI pin map
   before anything else; it is community reference, not measured.
2. **Photographic-negative colours** — uncomment `invert_colors: true`.
3. **Garbled, shifted, or a dead band at one edge** — switch `model: ILI9341` to `model: ST7789V`.
   The dual USB sockets on these units point that way, and it is the fault the `cyd-dashboard`
   firmware already showed on this board.
4. **Taps land in the wrong place** — expected. Adjust the `calibration:` block and record the
   numbers; the product will need them stored per unit regardless.

## Record the answers here, then in `inventory.md`

Fill this in as you go. An empty cell is an honest answer; a guessed one is not.

| Question | Result | Notes |
| --- | --- | --- |
| Q1 display controller | | ILI9341 / ST7789V, and whether `invert_colors` was needed |
| Q1 which unit | | MAC, so this is attributable to a board |
| Q2 I²C addresses seen | | verbatim from the boot log |
| Q2 pull-ups needed? | | were the on-module 10 kΩ enough |
| Q3 **offset, AHT20 − BME280** | | after ≥ 30 min warm, steady state. **This is the AU-09 number** |
| Q3 BME280 vs a real thermometer | | the offset that actually matters to a recipient |
| Q4 typing verdict | | tolerable / irritating / unacceptable as a gift |
| Q4 touch calibration values | | the four numbers that worked |
| Q5 LVGL verdict | | is this close enough to skip the bespoke build |
| ENS160 warm-up observed | | how long until TVOC/AQI stopped being nonsense |
| LVGL `buffer_size` that worked | | **direct input to the RAM budget the review says is missing** |
| Flash size of the built image | | ESPHome prints it. Compare against a 1.28 MB OTA slot |

## What this spike deliberately does not do

- **No MQTT, no Home Assistant, no credentials.** There is nothing to leak from this file, and the
  fleet broker password must not appear in it — see D-038 for the decision about the *product*.
- **No LDR reading.** ESP32 errata 3.11: sampling ADC1 briefly pulls GPIO36/39 low, and those are
  the touch IRQ and MISO. Mixing that in would confound Q4. Test night-dimming separately, after
  the touch verdict is in.
- **No controller ID-register read.** ESPHome cannot do it, which is itself a small finding for Q5.
  Empirical identification is good enough to unblock the design; AU-01 can still read the register
  properly if the bespoke route wins.
- **It does not settle the flash and RAM budget.** It gives two real data points — a working
  `buffer_size` and an image size — where the design currently has estimates.

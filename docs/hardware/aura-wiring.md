# AtmosMesh Aura — sensor wiring (CYD CN1)

Bench wiring for the [ESPHome spike](../../firmware/spike/README.md) and, later, the product.
Two I²C modules on one bus, on the only header the CYD gives us.

> **Read this first.** The CN1 pin order below comes from the community reference, not from a meter
> on *your* board. Reversing 3V3 and GND will destroy both sensor modules. **Verify with a
> multimeter before connecting anything** — the procedure is at the bottom and takes one minute.

## The bus

Nothing here needs 5 V, nothing has a fan, and nothing draws enough to trouble the CYD's LDO.

```text
        CYD  ESP32-2432S028
   ┌──────────────────────────┐
   │                      CN1 │  4-pin 1.25 mm JST (the kit pigtail fits this)
   │                    ┌─────┤
   │                    │ 1 ● │──── GND ───────────────┬──────────────┐
   │                    │ 2 ● │──── GPIO22 = SCL ──────┼───────┬──────┼───────┐
   │                    │ 3 ● │──── GPIO27 = SDA ──────┼──┬────┼──────┼────┐  │
   │                    │ 4 ● │──── 3V3 ───────────┬───┼──┼────┼──────┼──┐ │  │
   └────────────────────┴─────┘                    │   │  │    │      │  │ │  │
                                                   │   │  │    │      │  │ │  │
                        ENS160 + AHT20 module ─────┴───┴──┴────┘      │  │ │  │
                        ┌───────────────────────┐                     │  │ │  │
                        │ VIN  ←── 3V3          │                     │  │ │  │
                        │ GND  ←── GND          │   ENS160 @ 0x52     │  │ │  │
                        │ SCL  ←── GPIO22       │   AHT20  @ 0x38     │  │ │  │
                        │ SDA  ←── GPIO27       │   (fixed)           │  │ │  │
                        │ 3V3  ── leave open    │                     │  │ │  │
                        │ CS   ── leave open    │   CS/ADD/INT are    │  │ │  │
                        │ ADD  ── leave open    │   not needed for    │  │ │  │
                        │ INT  ── leave open    │   I²C at 0x52       │  │ │  │
                        └───────────────────────┘                     │  │ │  │
                                                                      │  │ │  │
                        BME280 (purple GY-BM E/P 280) ────────────────┴──┴─┴──┘
                        ┌───────────────────────┐
                        │ VCC  ←── 3V3          │
                        │ GND  ←── GND          │   BME280 @ 0x76
                        │ SCL  ←── GPIO22       │
                        │ SDA  ←── GPIO27       │   CSB and SDO are already
                        │ CSB  ── leave open    │   strapped on this board:
                        │ SDO  ── leave open    │   CSB→VCC (I²C), SDO→GND (0x76)
                        └───────────────────────┘
```

Four wires leave CN1 and fan out to both modules in parallel. That is all.

## Addresses — no collisions

| Device | Address | Set by |
| --- | --- | --- |
| AHT20 | `0x38` | fixed, not selectable |
| ENS160 | `0x52` | `ADD` pin left open/low. Tie `ADD` high for `0x53` |
| BME280 | `0x76` | `SDO` strapped to GND on the purple board |

The spike expects exactly these three. A boot scan showing all of them is Q2 answered.

## Placement matters more than the wiring

This is the part that decides whether the temperature reading is honest.

- **BME280 as far from the board as the pigtail allows**, out in moving room air. It is the
  reading the screen shows.
- **ENS160+AHT20 wherever is convenient.** Its AHT20 sits next to the ENS160's hotplate and only
  feeds the ENS160's own compensation — it is never displayed.
- Keep both off the back of the PCB. The ESP32, the TFT driver and the backlight all run warm, and
  a sensor pressed against them reads several degrees high. The spike's
  *"Self-heating offset (AHT20 − BME280)"* sensor exists to put a number on exactly this.

## Pull-ups: fit nothing

Both breakouts carry their own ~10 kΩ pull-ups, so the bus already sees roughly 5 kΩ per line with
both attached. **Do not add the 4.7 kΩ the design sketched** — that would land near 2.4 kΩ, which
is legal but pointlessly stiff. Only revisit this if a scan actually fails.

## Verify before you connect — one minute, saves two modules

With the CYD powered over USB and **nothing** plugged into CN1:

1. Black probe on a known ground (the USB shell or a GND pad), red probe on each CN1 pin in turn.
2. Expect exactly one pin at **~3.3 V** and one at **0 V**. The two signal pins will read
   somewhere in between or float — that is fine, they are idle I²C lines with pull-ups.
3. **Confirm the 3.3 V pin is the one this document calls pin 4, and the 0 V pin is pin 1.**
   If the order is reversed — and pigtail wire colours are not trustworthy — re-map before wiring.
4. Note what you measured in `inventory.md`. That turns the community pin map into a measured one
   for this board, which is the whole point of the exercise.

If anything is unexpected, stop and say so rather than connecting. The modules cost little; the
hour spent working out why a bus is dead costs more.

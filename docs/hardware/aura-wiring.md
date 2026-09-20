# AtmosMesh Aura — sensor wiring (CYD CN1)

Bench wiring for the [ESPHome spike](../../firmware/spike/README.md) and, later, the product.
Two I²C modules on one bus, on the only header the CYD gives us.

> **Read this first.** The CN1 pin order below comes from the community reference, not from a meter
> on *your* board. Reversing 3V3 and GND will destroy both sensor modules. **Verify with a
> multimeter before connecting anything** — the procedure is at the bottom and takes one minute.

Supersedes the four-module ENS160 + AHT20 + BME280 + VEML7700 arrangement. The gas sensor is now
the SGP40 alone and the light sensor is gone; see D-036 for why.

## The bus

Nothing here needs 5 V, nothing has a fan, and nothing draws enough to trouble the CYD's LDO.

```text
        CYD  ESP32-2432S028
   ┌──────────────────────────┐
   │                      CN1 │  4-pin 1.25 mm JST (the kit pigtail fits this)
   │                    ┌─────┤
   │                    │ 1 ● │──── GND ──────────┬──────────────┬─────────┐
   │                    │ 2 ● │──── GPIO22 = SCL ─┼────┬─────────┼────┐    │
   │                    │ 3 ● │──── GPIO27 = SDA ─┼─┬──┼─────────┼─┐  │    │
   │                    │ 4 ● │──── 3V3 ──────┬───┼─┼──┼───┬─────┼─┼──┼─┐  │
   └────────────────────┴─────┘               │   │ │  │   │     │ │  │ │  │
                                              │   │ │  │   │     │ │  │ │  │
                              10 µF ──────────┴───┼─┼──┼───┘     │ │  │ │  │
                              (at the branch,     │ │  │         │ │  │ │  │
                               + to 3V3, − to GND)│ │  │         │ │  │ │  │
                                                  │ │  │         │ │  │ │  │
        SGP40  "GY-SGP40"  @ 0x59  ───────────────┴─┴──┘         │ │  │ │  │
        ┌──────────────────────────┐                             │ │  │ │  │
        │ VCC / VIN  ←── 3V3       │   NOT photo-verified.       │ │  │ │  │
        │ GND        ←── GND       │   Read the silkscreen and   │ │  │ │  │
        │ SCL        ←── GPIO22    │   check for a regulator     │ │  │ │  │
        │ SDA        ←── GPIO27    │   before connecting.        │ │  │ │  │
        └──────────────────────────┘                             │ │  │ │  │
                                                                 │ │  │ │  │
        BME280  "GY-BM E/P 280"  @ 0x76  ────────────────────────┴─┴──┴─┴──┘
        ┌──────────────────────────┐
        │ VCC  ←── 3V3             │   PHOTO-VERIFIED 2026-09-04.
        │ GND  ←── GND             │   6-pin, front order:
        │ SCL  ←── GPIO22          │     VCC GND SCL SDA CSB SDO
        │ SDA  ←── GPIO27          │   No regulator → 3.3 V ONLY.
        │ CSB  ── leave open       │   CSB→VCC (I²C) and SDO→GND (0x76)
        │ SDO  ── leave open       │   are already strapped on the board.
        └──────────────────────────┘
```

| CN1 pin | Signal | To |
| ---: | --- | --- |
| 1 | GND | both modules' GND, capacitor − |
| 2 | GPIO22 | both modules' **SCL** |
| 3 | GPIO27 | both modules' **SDA** |
| 4 | 3V3 | both modules' VCC/VIN, capacitor + |

Four wires leave CN1 and fan out to both modules in parallel. That is all.

## Addresses — no collisions

| Device | Address | Set by |
| --- | --- | --- |
| SGP40 | `0x59` | fixed, not selectable (the SGP41 shares it — one or the other, never both) |
| BME280 | `0x76` | `SDO` strapped to GND on the purple board |

A boot scan showing exactly these two, and nothing else, is Q2 answered.

## Verify the SGP40 breakout before it goes on the bus

Its `inventory.md` row is operator-dictated, not photo-verified: supply and pin order are
**unconfirmed**. Before soldering, read the board and check three things.

- **Pin order.** `GY-` boards are not consistent between `VCC GND SCL SDA` and `VIN GND SDA SCL`.
  Trust the silkscreen on the part in your hand, not this diagram.
- **Regulator.** Look for a `662K`-marked SOT-23-5, the same XC6206-class LDO as the SHT41 board.
  Either way **3V3 is the correct feed**: the SGP40 die runs 1.7–3.6 V, so even after an LDO's
  dropout it stays well inside range. Never feed it 5 V on a "5 V tolerant" assumption.
- **Pull-ups.** If it carries a fitted `103` array, it contributes ~10 kΩ per line like the BME280.
  If the host-side pads are empty — as on the SHT41 board — the BME280's pull-ups carry the bus
  alone at ~10 kΩ, which is still fine at 100 kHz.

## Capacitors: one, and one place it must not go

- **Fit one 10 µF** across 3V3/GND where the pigtail branches to the two modules. The SGP40's
  hotplate is pulsed, and down thin pigtail wire that pulse droops the rail at the sensor — which
  modulates the very temperature the measurement depends on. The cap keeps the droop local.
- **100 nF at each module's supply pins is optional.** The photo-verified BME280 carries exactly
  one capacitor and no regulator; adequate for a part drawing microamps, but if the iron is already
  hot it removes a class of flaky behaviour you cannot diagnose inside a sealed gift.
- **Never put a capacitor on SDA or SCL.** I²C allows 400 pF of total bus capacitance and a long
  pigtail already spends part of it. Extra capacitance there slows the edges and turns a working
  bus into intermittent NAKs.

## Pull-ups: fit nothing

Both breakouts are expected to carry their own ~10 kΩ, so the bus sees roughly 5 kΩ per line.
**Do not add the 4.7 kΩ the original design sketched** — that would land near 2.4 kΩ, which is
legal but pointlessly stiff. Only revisit this if a scan actually fails.

## Placement matters more than the wiring

This is the part that decides whether the temperature reading is honest.

- **BME280 as far from the board as the pigtail allows**, out in moving room air. It is the reading
  the screen shows, and it is also the SGP40's humidity/temperature compensation source.
- **SGP40 near it but not touching it.** It is the only heater left in the build now that the
  ENS160 is gone, and it is a small one — which is precisely why dropping the ENS160 turned the
  self-heating question from a design risk into a footnote.
- Keep both off the back of the PCB. The ESP32, the TFT driver and the backlight all run warm, and
  a sensor pressed against them reads several degrees high.

## Verify before you connect — one minute, saves two modules

With the CYD powered over USB and **nothing** plugged into CN1:

1. Black probe on a known ground (the USB shell or a GND pad), red probe on each CN1 pin in turn.
2. Expect exactly one pin at **~3.3 V** and one at **0 V**. The two signal pins will read somewhere
   in between or float — that is fine, they are idle I²C lines with pull-ups.
3. **Confirm the 3.3 V pin is the one this document calls pin 4, and the 0 V pin is pin 1.**
   If the order is reversed — and pigtail wire colours are not trustworthy — re-map before wiring.
4. Note what you measured in `inventory.md`. That turns the community pin map into a measured one
   for this board, which is the whole point of the exercise.

If anything is unexpected, stop and say so rather than connecting. The modules cost little; the
hour spent working out why a bus is dead costs more.

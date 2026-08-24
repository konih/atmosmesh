# Power architecture

Station target (operator update 2026-08-14) and bench constraints. Chip-level currents:
[spec-comparison.md](spec-comparison.md). Decision: [D-005](../../agent-context/decisions.md).

## Target station supply

```text
230 V AC  →  enclosed isolated 5 V AC/DC  →  5 V rail
                                              ├─ ESP32 DevBoard VIN / 5V
                                              │     └─ onboard LDO → 3.3 V
                                              │           ├─ ESP32-WROOM-32
                                              │           ├─ OLED (small)
                                              │           ├─ BMP280 (small)
                                              │           └─ DHT22 / AM2302 (small)
                                              ├─ SDS011 VCC (5 V)
                                              └─ MQ135 heater / module VCC (5 V)

USB: serial/flash only. Do not assume USB 5 V and VIN 5 V may be connected at once
until the actual DevBoard OR-ing is confirmed from photos.
```

- ESP32 GPIOs stay **3.3 V logic**. A device may be *powered* from 5 V; a 5 V *signal* must not hit a GPIO.
- MQ135 analog out needs a measured divider before GPIO34.
- SDS011 UART is 3.3 V TTL (datasheet) — power 5 V, signals 3.3 V.
- Do **not** parallel the SANMIM 3.3 V AC/DC with the ESP32 `3V3` pin.
- Relays are **not** in the AtmosMesh MVP (D-004). Do not size this rail for relay coils.

## Candidate 5 V module — not yet verified

Inventory name: open AC/DC board marked **5V07 / 12V04**.

That family is commonly sold as two variants on similar silkscreen:

| Marking | Typical output | Typical current | Typical power |
| --- | --- | --- | --- |
| 5V07 | 5 V | 700 mA | 3.5 W |
| 12V04 | 12 V | ~400 mA | ~4.8 W |

Until a multimeter reading on the **DC output** (no ESP32 attached) shows a stable **~5 V**, this board is not the 5 V rail. A 12 V result must not feed SDS011, MQ135, or a 5 V-only DevBoard pin.

Typical vendor ripple for the 5 V / 700 mA variant is on the order of **60 mV** at 50 % load. SDS011 asks for **< 20 mV**. Extra filtering or a quieter supply may be required even if the voltage is 5 V.

SANMIM **SM-PLG06A / SM-104-3.3V-02** is a 3.3 V AC/DC. Spare only; not required while OLED + BMP280 + DHT22 stay on the ESP32 LDO (tens of milliamps).

## Current budget if ESP32 VIN shares the 5 V rail

Planning numbers from manufacturer sheets, not yet measured on the modules.

| Load | Rail | Planning current |
| --- | --- | --- |
| ESP32 Wi-Fi TX (LDO does not reduce current) | 5 V → 3.3 V | 180–240 mA |
| Mini OLED | 3.3 V from ESP32 LDO | ~10–20 mA |
| BMP280 + DHT22 | 3.3 V from ESP32 LDO | < 5 mA |
| SDS011 | 5 V | 70 mA ± 10 mA rated; treat **200 mA** as supply headroom (> 1 W spec) |
| MQ135 heater | 5 V | ≤ 190 mA continuous |
| **Peak coincidence (planning)** | **5 V** | **~650 mA** |

A **700 mA / 3.5 W** module has almost no margin once ESP32, SDS011 and MQ135 share it. Fan start, Wi-Fi TX and heater overlap can hit constant-current / foldback. **700 mA is enough for SDS011+MQ135 alone** (ESP32 on USB). **700 mA is not a comfortable single-rail station supply.**

Onboard AMS1117-class LDOs are thermally limited: \(P \approx (5-3.3)\times I_{3V3}\). The planned 3.3 V sensor+OLED load is fine. Do not hang extra 3.3 V amps on `3V3`; then add a separate 5→3.3 V regulator.

## Bench vs station (non-negotiable)

1. Photograph the AC/DC board (front/back, AC pads vs DC pads).
2. **Enclose the primary (230 V) side** before it is energised. No mains on a breadboard. Open AC/DC modules are not 5 V regulator bricks.
3. Measure DC output **unloaded**, then with a dummy load. Proceed only if it is stable ~5 V.
4. Until that is done, keep the **ESP32 on USB** (already proven) and do not feed VIN from the open module.
5. Confirm the DevBoard has a labelled `VIN` / `5V` input (not `3V3`) before attaching the rail.
6. For flashing: prefer USB with VIN disconnected until USB/VIN coexistence is verified on *this* board.

## 3.3 V from the ESP32 pin

Allowed for the MVP 3.3 V set (OLED, BMP280, DHT22). Not allowed as a dump for 5 V heaters, SDS011, or relays.

## AtmosMesh Grove YL-38 switched 3.3 V

This is separate from the ESP32 station supply above. Grove stays USB-powered and uses its 3V3 rail
for the small modules. The YL-38 corrosion-control design is provisional until the exact MOSFET is
identified from its marking and datasheet.

```text
3V3 ── P-channel MOSFET source
         drain ────────────── YL-38 VCC
         gate  ── ~1 kΩ ───── D1/GPIO5 (active LOW)
           └──── 100 kΩ ───── source/3V3 (fail-safe OFF pull-up)

YL-38 AO ── 47 kΩ ── A0
                       ├── 15 kΩ ── GND
                       └── optional 100 nF / 104 ── GND
YL-38 GND ─────────────────── GND; DO unused
```

Select a P-channel enhancement MOSFET with logic-level performance specified at
`VGS=-2.5/-3.3 V`. **Do not infer source/drain/gate order:** record the exact marking and use its
datasheet. GPIO5 drives the gate only; never feed YL VCC from a GPIO. Firmware sets the gate latch
HIGH before enabling OUTPUT, but the external 100 kΩ gate-source resistor is what keeps the switch
off during reset/boot. If VCC is directly tied to 3V3, software cannot remove probe power.

The 47 kΩ / 15 kΩ divider maps 3.3 V to about 0.80 V. This is conservative for a bare ESP8266 ADC,
but NodeMCU-style boards may already include an A0 divider, changing total attenuation and raw
counts. Treat the result as uncalibrated `soil_adc_raw`, never moisture percent. Normal commanded
on-time is 120 ms every 30 s (0.4%); fail-off is 250 ms (less than 0.9% per nominal interval).

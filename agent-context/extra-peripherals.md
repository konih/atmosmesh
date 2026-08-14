# Extra peripherals — live pin map (PCB handoff)

**Date:** 2026-08-14. **Do not edit KiCad from a firmware session.** PCB session adds
connectors only. Live breadboard GPIOs below are **occupied** — do not reuse.

OLED mux-48 did **not** fill the glass; operator still sees **two rows** of live text (plus the
lower-band packing). Treat the module as **128×32 visible**. Keep the 3-line lower-band UI.
Firmware on `fix/oled-u8g2-sds011-listen` is the live image.

**TFT dropped.** Do not add `J_TFT`. GPIO22 was sketched as TFT RST — **free for the mic**.

---

## Occupied GPIOs (do not steal)

| Function | GPIO | Notes |
| --- | --- | --- |
| OLED I²C SDA | 5 | 3V3 VCC |
| OLED I²C SCL | 4 | |
| BMP280 Wire1 SDA | 21 | |
| BMP280 Wire1 SCL | 19 | Also VSPI default SCK |
| AM2302 | 18 | |
| SDS011 UART2 RX | 16 | 5 V on sensor VCC only |
| SDS011 UART2 TX | 17 | |
| MQ135 ADC | 34 | Input-only, already divided |
| Beeper SIG | **25** | Live 3-pin |
| PIR D-SUN SIG | **33** | Live 3-pin. **Not 27** |
| HC-20 / DC-20 SIG | **22** | Live 3-pin **digital DO**. **Not ADC** |
| UART0 USB | 1 / 3 | Flash/monitor only |
| Boot | 0 | Keep free |
| Strapping | 2 | Keep free (download) |

Strap caution: **GPIO12** (flash voltage), **GPIO15** (MTDO). Do not use GPIO12.

---

## Authoritative live extras (operator wired)

All three are **3-pin: VCC, GND, SIG**. Never 5 V into a GPIO. VCC **3V3** unless the module is
5 V-powered with a **3.3 V SIG**.

| Device | SIG | Notes |
| --- | --- | --- |
| Beeper | **GPIO25** | Output. Firmware: 50 ms HIGH at boot (`beep: boot`); 50 ms on PIR rising edge. |
| PIR D-SUN | **GPIO33** | Digital input, `INPUT_PULLDOWN`. Was reserved 27 — **use 33**. Serial `pir: motion` / `pir: idle` (~50 ms debounce). OLED `P` on line 0 when motion if it fits. |
| HC-20 microphone | **GPIO22** | Digital SIG (sound-detect **DO**). Operator also said **DC-20** (same 3-pin class; SIG on 22 unless they give another pin). Serial `mic: sound` / `mic: quiet`. |

### GPIO22 is not analog

**GPIO22 is not an ADC pin** on ESP32 (ADC1 is 32, 33, 34, 35, 36, 39). HC-20/DC-20 on D22 is
**digital SIG / DO**, not analog AO. Firmware uses `digitalRead`. Input `INPUT_PULLDOWN` (or
leave floating if the module already drives the line). Do **not** wire an analog AO to GPIO22.

GPIO33 is ADC-capable but the PIR uses it as **digital** only.

---

## PCB session — add these headers only

Keep J1 OLED. **No J_TFT.**

| Ref | Type | Pin 1 → last | Nets / silk |
| --- | --- | --- | --- |
| J_BEEP | 1×3 | VCC, GND, SIG | `BEEP_VCC` (3V3 default, jumper to +5V if buzzer is 5 V **power** only), `GND`, `BEEP_SIG` = **GPIO25**. Silk: beeper. |
| J_PIR | 1×3 | VCC, GND, SIG | `PIR_VCC` (3V3 default; jumper to +5V only if the board is HC-SR501), `GND`, `PIR_OUT` = **GPIO33**. Silk: D-SUN PIR. Pin order VCC/GND/SIG to match the live 3-pin class (photograph the module if OUT sits in the middle). |
| J_MIC | 1×3 | VCC, GND, SIG | `MIC_VCC` 3V3, `GND`, `MIC_SIG` = **GPIO22**. Silk: **HC-20/DC-20**. Digital DO only — **no AO pad**. |

`BEEP_VCC` / `PIR_VCC`: 3V3 with optional jumper to `+5V`. Signals never 5 V.

GPIO leftover after this map: **12** (avoid), **14 / 13 / 15 / 23 / 26 / 27 / 32**, **35 / 36 / 39**
(ADC1 input-only). Do not assign them without a new live pin from the operator.

---

## Identity notes (do not block these headers)

- **Beeper:** active vs passive unknown. GPIO25 HIGH for 50 ms is enough to prove the pin.
- **PIR:** D-SUN 3-pin; mini AM312-class vs HC-SR501 (pots). Photograph size/pots before locking
  VCC jumper default if it looks like SR501.
- **HC-20 / DC-20:** not a standard I2S MEMS PN. Live wiring is 3-pin SIG on GPIO22 → treat as
  LM393-class **DO**. Do not add I2S (SCK/WS/SD) copper.

---

## Firmware (this session)

Implemented on `fix/oled-u8g2-sds011-listen` (worktree `atmosmesh-oled-u8g2`): pin constants
`kBeeperGpio=25`, `kPirGpio=33`, `kMicGpio=22`. Native-tested debounce + serial labels. Flash
`/dev/cu.usbserial-0001`. Do not merge to `main` from the KiCad tree in the same change.

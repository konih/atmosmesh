# Extra peripherals — live pin map (PCB handoff)

**Date:** 2026-08-14. **Do not edit KiCad from a firmware session.** PCB session adds
connectors only. Live breadboard GPIOs below are **occupied** — do not reuse.

OLED mux-48 did **not** fill the glass; operator still sees **two rows** of live text (plus the
lower-band packing). Treat the module as **128×32 visible**. Keep the 3-line lower-band UI.
Firmware on `fix/oled-u8g2-sds011-listen` is the live image.

**TFT dropped.** Do not add `J_TFT`. GPIO22 was sketched as TFT RST, then as digital mic DO —
**GPIO22 is now free.** Mic moved to **GPIO35 analog AO**.

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
| Mic analog AO | **35** | Live 3-pin **ADC1**. Input-only. **Not 22** |
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
| Microphone | **GPIO35** | Analog **AO** on ADC1, 11 dB atten. **Input-only — no output.** VCC **3V3**; AO must stay **≤3.3 V**. Serial `mic: raw=…` every ~750 ms. `mic: sound` / `mic: quiet` when raw crosses **800** (~645 mV at 3.3 V FS). Was GPIO22 digital DO. |

### GPIO35 is analog (input-only)

**GPIO35 is ADC1** (with 32, 33, 34, 36, 39). It is **input-only**: no `pinMode(OUTPUT)`, no
`digitalWrite`, no internal pull. Firmware uses `analogRead` at **11 dB** (~3.3 V full scale).
Do **not** put 5 V on AO. GPIO22 is **not** the mic.

GPIO33 is ADC-capable but the PIR uses it as **digital** only.

---

## PCB session — add these headers only

Keep J1 OLED. **No J_TFT.**

| Ref | Type | Pin 1 → last | Nets / silk |
| --- | --- | --- | --- |
| J_BEEP | 1×3 | VCC, GND, SIG | `BEEP_VCC` (3V3 default, jumper to +5V if buzzer is 5 V **power** only), `GND`, `BEEP_SIG` = **GPIO25**. Silk: beeper. |
| J_PIR | 1×3 | VCC, GND, SIG | `PIR_VCC` (3V3 default; jumper to +5V only if the board is HC-SR501), `GND`, `PIR_OUT` = **GPIO33**. Silk: D-SUN PIR. Pin order VCC/GND/SIG to match the live 3-pin class (photograph the module if OUT sits in the middle). |
| J_MIC | 1×3 | VCC, GND, SIG | `MIC_VCC` 3V3, `GND`, `MIC_SIG` = **GPIO35 analog AO**. Silk: mic AO. **Not GPIO22.** |

`BEEP_VCC` / `PIR_VCC`: 3V3 with optional jumper to `+5V`. Signals never 5 V.

GPIO leftover after this map: **12** (avoid), **14 / 13 / 15 / 23 / 22 / 26 / 27 / 32**, **36 / 39**
(ADC1 input-only). Do not assign them without a new live pin from the operator.

---

## Identity notes (do not block these headers)

- **Beeper:** active vs passive unknown. GPIO25 HIGH for 50 ms is enough to prove the pin.
- **PIR:** D-SUN 3-pin; mini AM312-class vs HC-SR501 (pots). Photograph size/pots before locking
  VCC jumper default if it looks like SR501.
- **Mic:** live wiring is analog **AO** on GPIO35. Do not add I2S (SCK/WS/SD) copper. Do not treat
  as LM393 digital DO.

---

## Firmware (this session)

Implemented on `fix/oled-u8g2-sds011-listen` (worktree `atmosmesh-oled-u8g2`): pin constants
`kBeeperGpio=25`, `kPirGpio=33`, `kMicGpio=35`. Analog ADC1 11 dB; sound threshold raw **800**.
Flash `/dev/cu.usbserial-0001`. Do not merge to `main` from the KiCad tree in the same change.
**Do not edit KiCad from firmware.** Carrier copper may still show MIC on GPIO22 until a later
KiCad session moves `MIC_SIG` to GPIO35.

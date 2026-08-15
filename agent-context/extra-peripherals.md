# Extra peripherals — live pin map (PCB handoff)

**Date:** 2026-08-14. **Do not edit KiCad from a firmware session.** PCB session adds
connectors only. Live breadboard GPIOs below are **occupied** — do not reuse.

OLED mux-48 did **not** fill the glass; operator still sees **two rows** of live text (plus the
lower-band packing). Treat the module as **128×32 visible**. Keep the 3-line lower-band UI.
Firmware on `main` is the live image.

**TFT dropped.** Do not add `J_TFT`. **GPIO22 is free.**

**No microphone. No clap / sound-detect.** Do not wire GPIO35 or GPIO22 for audio. The HC-20/DC-20
module is unused.

**VEML7700 lux** (part **not fitted yet**): I²C addr **0x10** on **Wire1 with BMP280** —
SDA=**GPIO21**, SCL=**GPIO19**, VCC=**3V3**, GND. No address clash (BMP **0x76**, VEML **0x10**).
OLED stays Wire GPIO5/GPIO4.

---

## Occupied GPIOs (do not steal)

| Function | GPIO | Notes |
| --- | --- | --- |
| OLED I²C SDA | 5 | 3V3 VCC |
| OLED I²C SCL | 4 | |
| BMP280 + VEML7700 Wire1 SDA | 21 | Shared sensor I²C |
| BMP280 + VEML7700 Wire1 SCL | 19 | Also VSPI default SCK |
| AM2302 | 18 | |
| SDS011 UART2 RX | 16 | 5 V on sensor VCC only |
| SDS011 UART2 TX | 17 | |
| MQ135 ADC | 34 | Input-only, already divided |
| Beeper SIG | **25** | Live 3-pin |
| PIR D-SUN SIG | **33** | Live 3-pin. **Not 27** |
| UART0 USB | 1 / 3 | Flash/monitor only |
| Boot | 0 | Keep free |
| Strapping | 2 | Keep free (download) |

Strap caution: **GPIO12** (flash voltage), **GPIO15** (MTDO). Do not use GPIO12.

---

## Authoritative live extras (operator wired)

Beeper and PIR are **3-pin: VCC, GND, SIG**. Never 5 V into a GPIO. VCC **3V3** unless the module is
5 V-powered with a **3.3 V SIG**.

| Device | Bus / SIG | Notes |
| --- | --- | --- |
| Beeper | **GPIO25** | Output. Firmware: 50 ms HIGH at boot (`beep: boot`); 50 ms on PIR rising edge. |
| PIR D-SUN | **GPIO33** | Digital input, `INPUT_PULLDOWN`. Was reserved 27 — **use 33**. Serial `pir: motion` / `pir: idle` (~50 ms debounce). OLED `P` on line 0 when motion if it fits. |
| VEML7700 | **SDA=21 SCL=19** addr **0x10** | **Not fitted yet.** Shares J2 / `SDA_SENS` / `SCL_SENS` with BMP280. VCC 3V3. Serial `veml7700: not found (ok until fitted)` until `begin()` succeeds; then `veml7700: lux=…`. OLED `-- lx` / `123 lx` on the hPa line. |

GPIO33 is ADC-capable but the PIR uses it as **digital** only.

---

## PCB session — add these headers only

Keep J1 OLED. **No J_TFT. No J_MIC.**

| Ref | Type | Pin 1 → last | Nets / silk |
| --- | --- | --- | --- |
| J_BEEP | 1×3 | VCC, GND, SIG | `BEEP_VCC` (3V3 default, jumper to +5V if buzzer is 5 V **power** only), `GND`, `BEEP_SIG` = **GPIO25**. Silk: beeper. |
| J_PIR | 1×3 | VCC, GND, SIG | `PIR_VCC` (3V3 default; jumper to +5V only if the board is HC-SR501), `GND`, `PIR_OUT` = **GPIO33**. Silk: D-SUN PIR. Pin order VCC/GND/SIG to match the live 3-pin class (photograph the module if OUT sits in the middle). |
| J_VEML | 1×4 | VIN, GND, SCL, SDA | `+3V3`, `GND`, `SCL_SENS` (GPIO19), `SDA_SENS` (GPIO21). Silk: VEML7700. Shares BMP280 I²C. **No new GPIO.** |

`BEEP_VCC` / `PIR_VCC`: 3V3 with optional jumper to `+5V`. Signals never 5 V.

GPIO leftover after this map: **12** (avoid), **14 / 13 / 15 / 23 / 22 / 26 / 27 / 32 / 35**, **36 / 39**
(ADC1 input-only). Do not assign them without a new live pin from the operator.

---

## Identity notes (do not block these headers)

- **Beeper:** active vs passive unknown. GPIO25 HIGH for 50 ms is enough to prove the pin.
- **PIR:** D-SUN 3-pin; mini AM312-class vs HC-SR501 (pots). Photograph size/pots before locking
  VCC jumper default if it looks like SR501.
- **VEML7700:** lux, I²C 0x10. Not fitted on the breadboard yet. Do not add I2S or a mic/clap
  header. Do not steal a GPIO for it.

---

## Firmware (this session)

On `main`: `kBeeperGpio=25`, `kPirGpio=33`, `kVeml7700Address=0x10` on Wire1 GPIO21/19. No clap,
no mic, GPIO22/35 unused. Flash `/dev/cu.usbserial-0001`.
**Do not edit KiCad from firmware.**

# Decisions and open questions

## Accepted decisions

### D-001 — Mini I²C OLED is the MVP display

- **Status:** Accepted
- **Reason:** Low pin count, low memory use, sufficient local feedback.
- **Consequence:** The 480×320 Raspberry Pi TFT is excluded from the MVP. Station firmware
  drives an SSD1306 over I²C (prefer 0x3C, then 0x3D) on GPIO5/GPIO4. The I²C 1602 LCD is not
  the product display.

### D-002 — MQ135 is not CO₂

- **Status:** Accepted
- **Rule:** Publish only a raw or normalized relative gas/air-quality trend.
- **Forbidden labels:** `co2`, `co2_ppm`, `ppm`, or any health/safety claim derived from MQ135.

### D-003 — Separate power domains during the bench phase

- **Status:** Superseded for the *station* by D-005. Still binding on the *bench* until the 5 V
  AC/DC module is enclosed and measured.
- **Rule (bench):** ESP32 stays on its confirmed USB input. 5 V sensors stay off that USB 5 V rail.
  Grounds are common; positive rails are not joined until D-005 measurement gates pass.

### D-004 — No mains control

- **Status:** Accepted
- **Reason:** The project is a measurement station, not a mains automation device.
- **Clarification:** An *enclosed, isolated* AC/DC module as the station 5 V source is power, not
  control. Open mains PCBs, breadboard-mains, and switching mains loads/relays remain forbidden.

### D-005 — Station 5 V rail feeds VIN and 5 V sensors

- **Status:** Accepted as the station target; not yet energised.
- **Rule:** 230 V AC → enclosed isolated 5 V AC/DC → one 5 V rail → ESP32 `VIN`/`5V` and the
  devices that actually need 5 V (SDS011, MQ135). The DevBoard LDO makes 3.3 V for the ESP32 and
  small 3.3 V loads (OLED, BMP280, DHT22). GPIOs remain 3.3 V; 5 V *signals* need level shifting
  or a divider. Details: `docs/hardware/power.md`.
- **Candidate PSU:** open module marked `5V07 / 12V04`. Measure DC out before any ESP32
  connection. Same silkscreen family is sold as 5 V / 700 mA *and* 12 V / ~400 mA.
- **Counterpoints (kept):** (1) 700 mA is tight if ESP32 Wi-Fi TX, SDS011 and MQ135 heater coincide
  (~650 mA planning). (2) Vendor ripple ~60 mV vs SDS011 < 20 mV. (3) USB+VIN together can
  back-feed until this DevBoard is identified. (4) Open 230 V boards must be enclosed first.
- **Consequence:** SANMIM SM-104-3.3V-02 is spare, not paralleled with ESP32 `3V3`. Relays stay
  out of the power budget.

### D-006 — PlatformIO for firmware; GPIO5/GPIO4 is the display I²C bus

- **Status:** Accepted for toolchain. Display hardware on this bus is the SSD1306 (D-001), not LCD.
- **Rule:** Operator wired the mini I²C OLED to DevBoard **D5 (GPIO5)** and **D4 (GPIO4)**. Firmware
  treats D5 as SDA and D4 as SCL, prefers address **0x3C** then **0x3D**, and will swap the pin
  mapping once if the first mapping finds no OLED.
- **Power:** OLED VCC must be **3.3 V** (ESP32 `3V3`), not 5 V. GPIO5 wants idle-high at boot; the
  I²C pull-up is usually compatible.
- **Toolchain:** PlatformIO + Arduino, board `esp32dev`. Host-side Unity tests run on `native`.
  ESPHome remains a later option for station YAML if we want it; we do not maintain two stacks now.
- **Supersedes:** Earlier LCD 1602 dummy-text bring-up on the same pins. That panel is not the
  station display.

## Open decisions

### OQ-001 — Firmware framework

- **Status:** Resolved by D-006 — PlatformIO + Arduino (`esp32dev`), native Unity tests.
- **Revisit if:** ESPHome would materially simplify the full station (MQTT, HA discovery) after the
  bench sensors work.

### OQ-002 — Kubernetes packaging

- **Options:** Helm values over upstream charts; Kustomize; a small umbrella chart.
- **Decision trigger:** Capture the target cluster's ingress, storage, secret, DNS, and GitOps
  conventions in RLS-06.

### OQ-003 — Metrics path

- **Options:** Home Assistant history only; MQTT exporter to Prometheus; dedicated time-series
  storage.
- **Decision trigger:** Agree retention and dashboard needs before RLS-06 implementation.

### OQ-004 — MQ135 in MVP

- **Options:** Include as an experimental trend; omit until burn-in and safe ADC measurement are
  proven.
- **Decision trigger:** RLS-03 bench evidence.

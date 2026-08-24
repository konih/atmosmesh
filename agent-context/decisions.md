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

### D-007 — MQTT contract is id-based with Home Assistant discovery

- **Status:** Accepted (RLS-05 implementation).
- **Rule:** Topics use station id `atmosmesh-0001` under `home/air/…` — never a room name. Product
  id is `atmosmesh-v1`. Publish one JSON state topic plus retained availability (LWT `offline`).
  Announce entities with Home Assistant MQTT discovery (`homeassistant/…/atmosmesh_0001/…`);
  republish discovery on every reconnect (kumulus Mosquitto persistence is off). Wi-Fi/MQTT are
  async and must not block SDS011 UART drain or the OLED. Credentials via gitignored `.envrc`
  (preferred) or `secrets.hpp` — never committed. No lux entity until a light sensor is fitted.
  MQ135 stays `gas_index`, never CO₂.
- **Supersedes:** Draft `home/air/wohnzimmer/…` topic candidates in older architecture notes.

### D-008 — AtmosMesh Grove is the ESP8266 AtmosMesh v1.5 variant

- **Status:** Accepted (operator, 2026-08-24).
- **Identity:** Product name **AtmosMesh Grove**, stable product ID `atmosmesh-grove-v1.5`, product
  variant `atmosmesh-v1.5`, and default station ID `atmosmesh-grove-0001` are separate concepts.
  Device identity remains ID-based, never a room name.
- **Architecture:** Grove has a thin target-specific entrypoint and explicit product profile. It
  shares native-tested health/display/domain utilities while PlatformIO source filters keep the
  existing ESP32 runtime behavior intact. This slice moves that large composition root to an
  explicit product path but does not refactor it into a thin profiled entrypoint or copy its
  hardware stack into the ESP8266 app.
- **Boundary:** This first Grove slice measures DHT11 temperature/humidity and BMP180 pressure/
  temperature and renders a 128×32 SSD1306 display. It does not claim ESP32-only sensors.

### D-009 — Grove v1.5 wiring and boot constraint

- **Status:** Accepted from operator wiring plus read-only board probe, 2026-08-23/24.
- **Controller evidence:** ESP8266EX, 26 MHz crystal, 4 MB flash; ROM loader and AT firmware
  responded before the authorized AtmosMesh replacement. NodeMCU-style D labels are used.
- **I²C:** OLED and BMP180 share SDA=`D2`/GPIO4 and SCL=`D3`/GPIO0. Firmware must explicitly call
  `Wire.begin(4, 0)` and target the 128×32 display. All three modules use 3.3 V.
- **Boot caveat:** `D3`/GPIO0 is a boot strap and must remain high during reset. The present wiring
  is documented, not silently changed. If the bus or a module holds it low, the ESP8266 enters its
  ROM download mode instead of starting AtmosMesh.
- **DHT11 assumption:** DATA=`D5`/GPIO14 follows the agreed wiring proposal but has not been
  physically re-verified. It is a named profile constant, not an implicit library default.
- **Flashing:** After operator authorization and independent review, the coordinator successfully
  flashed AtmosMesh Grove on 2026-08-24. The former AT firmware was replaced; AtmosMesh v1 and its
  ESP32 were untouched.

### D-010 — Grove analog sensors are deferred

- **Status:** Accepted.
- **Scope:** YL-69/YL-38 soil probe, uncalibrated LDR and MAX4466 microphone are follow-on work;
  they are not described as fitted or working in the first Grove image.
- **ADC constraint:** ESP8266 has one ADC channel. The bare chip input range is 0–1.0 V, while some
  NodeMCU-style boards add an input divider. The exact board circuit must be confirmed before any
  analog output is connected or an ADC architecture is selected.
- **Claims:** LDR and microphone values remain relative unless calibrated. YL-69 corrosion control
  requires switched power; the three analog sources must never be tied together.

## Additional accepted decision

### D-011 — One PlatformIO project with explicit product composition roots

- **Status:** Accepted after independent review; see
  [ADR-0001](../docs/adr/0001-multi-product-firmware-composition.md).
- **Decision:** AtmosMesh v1 and Grove v1.5 remain independent, first-class products. Each has one
  named composition root, canonical build environment and compile-time identity profile. Shared
  host-testable code remains outside product roots.
- **Compatibility:** `esp32dev`, `esp8266-grove`, `task build` and `task build-grove` remain aliases
  during migration. Product IDs describe hardware contracts and do not change with routine fixes.
- **Identity metadata:** Stable product ID, product variant and station ID are separate values for
  every profile. AtmosMesh v1 uses product ID `atmosmesh-v1`, variant `esp32-full-station` and
  default station ID `atmosmesh-0001`; Grove retains the D-008 values.
- **Boundary:** Moving the ESP32 root does not authorize behavior changes or claim it is already a
  thin composition layer. Shared extraction remains incremental work.

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

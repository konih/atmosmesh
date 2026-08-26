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
- **DHT11:** DATA=`D5`/GPIO14 is a named profile constant. Valid runtime frames on 2026-08-24 prove
  communication on the configured input; observed values are not calibration or accuracy evidence.
- **Flashing:** After operator authorization and independent review, the coordinator successfully
  flashed AtmosMesh Grove on 2026-08-24. The former AT firmware was replaced; AtmosMesh v1 and its
  ESP32 were untouched.

### D-010 — Grove A0 sensors are deferred

- **Status:** Accepted as the earlier boundary; YL-38 deferral superseded by D-015/V15-06.
- **Scope:** MAX4466 remains dropped from the Grove plan. The LDR moved to the separately approved
  D7 RC interface in D-012. YL-38 is now implemented only under D-015's raw/divided/switched rules.
- **ADC constraint:** ESP8266 has one ADC channel. The bare chip input range is 0–1.0 V, while some
  NodeMCU-style boards add an input divider. The exact board circuit must be confirmed before any
  analog output is connected or an ADC architecture is selected.
- **Claims:** YL-69 corrosion control requires switched power. Its later analog path must not be tied
  to another analog source.

### D-012 — Grove light uses a bounded D7 RC response time

- **Status:** Accepted from installed operator wiring, 2026-08-24.
- **Wiring:** `3V3 → LDR → 1 kΩ series → D7/GPIO13 measurement node`; 100 nF from the node to GND.
- **Measurement:** Firmware briefly drives D7 low to discharge the capacitor, releases D7 as an
  input, then cooperatively measures time-to-high with a hard timeout. It must not busy-wait long
  enough to starve Wi-Fi, MQTT, sensing or display work.
- **Semantics:** The only valid value is uncalibrated `light_charge_us`; lower means brighter.
  Timeout, disconnected and saturated/immediate states are unavailable—not zero, lux or percent.
- **Boundary:** This story leaves A0 unused; D-015/V15-06 later assigns it only to divided YL AO.
  MAX4466 will not be used.

### D-013 — Grove reuses the ID-based MQTT contract with a thin ESP8266 transport

- **Status:** Accepted by operator, 2026-08-24.
- **Contract:** Grove uses product ID `atmosmesh-grove-v1.5`, station ID
  `atmosmesh-grove-0001`, retained availability/LWT, reconnect discovery replay and optional
  `light_charge_us`. Its four discovery entities are temperature, humidity, pressure and explicitly
  uncalibrated RC charge/response time in microseconds. D-015/V15-06 extends this contract with a
  fifth optional raw soil ADC diagnostic; it does not change the four original entities.
- **Architecture:** Shared host-tested identity/topic/discovery/session utilities are parameterized;
  ESP8266WiFi/PubSubClient remains a thin product transport. Existing ESP32 wrappers and runtime
  behavior stay intact.
- **Failure behavior:** Missing credentials or network/broker loss must not stop local sensors/OLED.
  Reconnect attempts use bounded backoff. DNS plus TCP share a 1000 ms transport budget and the
  MQTT response wait is separately bounded to one second. Pending state clears only after its state
  publish succeeds, not when an action is merely planned. Secrets remain generated and gitignored.

### D-014 — Grove local health uses a two-channel polarity-aware LED

- **Status:** Accepted from operator-installed wiring, 2026-08-24.
- **Wiring:** Red=`D6`/GPIO12 and green=`D0`/GPIO16, each through its own approximately 330 Ω
  resistor. Default is common-cathode (HIGH turns a channel on); build flag
  `ATMOSMESH_GROVE_LED_COMMON_ANODE=1` selects common-anode (LOW turns a channel on). Startup must
  state the compiled polarity and levels. No WS2812 library is used.
- **Meaning:** Red means a core sensor or explicit acquisition fault. Amber (red+green) means core
  sensors are valid but MQTT is offline or unconfigured. Green means core sensors and MQTT are
  healthy. An uncalibrated/missing light or not-yet-sampled soil value alone is not a fault.

### D-015 — Grove soil is raw, PNP-switched and sampled every 30 seconds

- **Status:** Accepted from operator wiring contract, 2026-08-24.
- **Power:** The operator confirmed and physically wired a 2N3906 PNP high-side switch:
  emitter=`3V3`, collector=YL-38 VCC, base=`D1`/GPIO5 through 2.2 kΩ, with an external 100 kΩ
  base-to-emitter pull-up. GPIO5 drives only the transistor base and must never source YL-38 power.
  Base LOW enables the switch; initialization establishes HIGH/OFF before enabling the output. If
  YL VCC is tied directly to 3V3, firmware cannot prevent continuous power and must not claim duty
  cycling.
- **Analog:** YL AO reaches A0 through 47 kΩ top / 15 kΩ bottom. The installed 100 nF (`104`) from
  A0 to GND filters noise; DO is unused and ground is common. Raw counts depend on whether the
  specific NodeMCU board also contains an onboard A0 divider, so values are `soil_adc_raw`
  only—never moisture percent or calibrated moisture.
- **Cadence:** The operator requested no more than 2 Hz. Soil changes slowly, so firmware uses a
  conservative 30 s start-to-start interval, 100 ms settle and a small bounded averaged sample set,
  then turns power off immediately. MQTT transport work is deferred while the sensor is powered.

### D-016 — Grove live OLED uses all four rows for descriptive measurements

- **Status:** Accepted from operator display feedback, 2026-08-24.
- **Page:** The 128×32 live page has no product/name row and no anonymous `D1B1` health bits. Its
  four rows label DHT11 temperature/humidity, BMP180 pressure, raw RC light response in microseconds
  and raw soil ADC respectively.
- **Missing values:** Each missing value retains its metric label and is never rendered as numeric
  zero. Serial diagnostics and the bi-color LED remain the detailed health channels.
- **Boundary:** The page does not add BMP temperature to the public contract, convert light to lux,
  convert soil to moisture, or change AtmosMesh v1.

### D-019 — MQTT remains AtmosMesh's sole transport; ESPHome native API declined

- **Status:** Accepted; see
  [ADR-0002](../docs/adr/0002-mqtt-vs-esphome-native-api-transport.md).
- **Trigger:** Answers OQ-001's revisit condition — operator asked for ESPHome's native API
  (Home Assistant's direct protobuf/Noise protocol, not MQTT) as a second, switchable transport.
- **Decision:** MQTT (D-007/D-013) stays the only transport across all products, including the new
  Aqua variant. Declined both hand-rolling the native API protocol and adopting the one real
  existing device-side library found (`BentuinoESPHomeAPI`: AGPL-3.0 vs. this project's MIT
  license, ~2.5 months old, unaudited vendored crypto, and an ESP8266 RAM footprint that does not
  fit alongside Aqua's own sensors/OLED/MQTT).
- **If revisited:** Any future transport choice is compile-time-switchable only (a second
  canonical PlatformIO environment), never a runtime toggle — two resident network stacks do not
  fit in ESP8266 RAM. See ADR-0002's three explicit revisit triggers.

### D-020 — Aqua live OLED uses a 4-row layout at 6×10, distinct from D-016

- **Status:** Accepted as part of AQ-01 implementation, 2026-08-26.
- **Page:** The 128×64 live page reuses AtmosMesh v1's existing
  `U8G2_SSD1306_128X64_ALT0_F_HW_I2C` constructor (no new duplicated constructor path) with the
  `u8g2_font_6x10_tf` font at baselines 14/28/42/56 px — taking advantage of the doubled vertical
  resolution over Grove's 128×32 glass rather than packing eight thin rows. Rows: SHT41
  temperature, SHT41 humidity, raw water-probe ADC, and an MQTT connectivity status line.
- **Missing values:** Each missing value keeps its metric label (`T:--`, `RH:--`, `Water:--`) and
  is never rendered as numeric zero, mirroring D-016's rule. `aqua_oled_lines()`
  (`aqua_status.hpp/cpp`) is host-tested the same way `grove_oled_lines()` is.
- **Boundary:** This decision is Aqua-specific. It does not amend D-016, and it does not change
  Grove v1.5's or AtmosMesh v1's own OLED pages.

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

- **Status:** Resolved by D-006 — PlatformIO + Arduino (`esp32dev`), native Unity tests. Revisit
  trigger fired 2026-08-26 (operator asked for ESPHome native API alongside MQTT) and was
  evaluated in [D-019](#d-019--mqtt-remains-atmosmeshs-sole-transport-esphome-native-api-declined)
  / ADR-0002 — still resolved, not reopened.
- **Revisit if:** ESPHome would materially simplify the full station (MQTT, HA discovery) after the
  bench sensors work. See ADR-0002's three specific revisit triggers for what would change this.

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

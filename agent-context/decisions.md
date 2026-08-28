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
  resistor. Physical diagnostic evidence confirmed the installed LED is common-anode (LOW turns a
  channel on), so that is the canonical Grove default. An explicit build override selects
  common-cathode (HIGH turns a channel on) for alternative hardware. Startup must state the
  compiled polarity and levels. No WS2812 library is used.
- **Meaning:** The physical color map remains red/amber/green; D-018 supersedes the original simple
  health-only priority with calibration-safe soil/system semantics. Intentionally unconfigured MQTT
  is neutral, while configured-but-offline MQTT is red.

### D-018 — Grove soil LED status requires validated directional calibration

- **Status:** Accepted from operator status request, 2026-08-24.
- **Calibration:** Default is disabled: raw YL-38 ADC remains unclassified. Compile-time metadata
  may enable calibration only with an explicit direction (`higher-is-wetter` or
  `lower-is-wetter`) plus raw dry and acceptable cutoffs in 0..1023. Higher-is-wetter requires
  `dry < acceptable`; lower-is-wetter requires `dry > acceptable`. Unknown direction,
  out-of-range, equal or direction-inconsistent cutoffs are invalid and fail safe to amber.
- **Classification:** At/beyond the dry cutoff in the dry direction is `dry`; strictly between
  cutoffs is `needs-watering`; at/beyond the acceptable cutoff in the wet direction is
  `acceptable`. Missing before a completed sample is amber; explicit acquisition failure is red.
- **Priority:** Core readings are unknown, not erroneous, until the first acquisition attempt; with
  MQTT intentionally unconfigured this boot/pre-sample state follows missing soil to amber. After
  an actual DHT/BMP attempt, a core sensor error is red. Core/acquisition error is red, then
  configured-but-offline MQTT is red, then
  calibrated dry is red. Missing/unclassified/invalid/warning soil is amber. Calibrated acceptable
  soil with otherwise healthy system is green. Intentionally unconfigured MQTT is neutral.
- **Truth boundary:** Serial exposes `soil-led-status`, raw value, validation state, direction and
  cutoffs. MQTT/HA continue publishing raw `soil_adc_raw` only—never moisture percentage/device
  class. Thresholds remain disabled until operator measurements establish direction and cutoffs.

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

### D-017 — Grove OLED/LED visual test is an explicit temporary build mode

- **Status:** Accepted from operator diagnostic request, 2026-08-24.
- **Decision:** The canonical Grove image remains the D-016 four-row live display and normal health
  LED. A separately named PlatformIO environment and Taskfile workflow may compile
  `ATMOSMESH_GROVE_VISUAL_DIAGNOSTIC=1`; after a successful OLED initialization that image fills
  the complete logical 128×32 framebuffer and refuses later live-page redraws.
- **LED:** The same diagnostic repeats red, green, amber (red+green) and off at two seconds per
  phase. It derives actual D6/D0 levels from the existing polarity-aware LED abstraction, logs every
  transition, and uses unsigned elapsed-time arithmetic across `millis()` wraparound.
- **Observability:** Startup serial states that the full-area diagnostic is selected and names the
  logical `128x32` geometry before OLED probe/init, including error paths. A separate result says
  whether the fill was applied or not applied because the OLED was missing/init failed. Sensors,
  status LED and networking may continue running behind the fixed white test screen.
- **Boundary:** This diagnostic does not alter AtmosMesh v1, the canonical Grove build, wiring,
  controller selection or geometry. Flashing and restoring images remain explicit operator actions
  after independent review; the diagnostic may remain installed until the operator explicitly asks
  to restore normal firmware. A filled logical framebuffer and commanded LED levels are not by
  themselves proof of physical pixels or colors.

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

### D-021 — Grove soil calibration enabled with operator-supplied thresholds

- **Status:** Accepted from operator calibration request, 2026-08-26.
- **Values:** `lower-is-wetter`, dry cutoff raw ADC `200`, acceptable cutoff raw ADC `100` — raw
  200 or higher is `dry` (red), raw 100 or lower is `acceptable`/well-watered (green), the band
  between is `needs-watering` (amber). Satisfies D-018's `dry > acceptable` ordering rule for the
  lower-is-wetter direction.
- **Scope:** Enabled only on the canonical `atmosmesh-grove-v1_5` PlatformIO environment (and its
  `esp8266-grove`, `-visual-diagnostic`, `-common-cathode` variants) via `firmware/platformio.ini`
  build flags. `env:native` keeps calibration compiled disabled so
  `test_soil_calibration_defaults_disabled_and_validates_both_directions` still exercises the
  fail-safe default.
- **Supersedes:** D-018's "thresholds remain disabled until operator measurements establish
  direction and cutoffs" — that measurement is this decision.

### D-022 — Room I²C sensors share direct 3.3 V; external signals use 330 Ω series protection

- **Status:** Accepted for the provisional ROOM-01 design, 2026-08-28.
- **Power:** VEML7700 VIN and SHT41 VCC connect directly to the Ideaspark board's confirmed 3.3 V
  output. The rejected alternative was a series Schottky diode or about 50 Ω per sensor: both add a
  droop path under the SHT41's measurement current bursts and protect nothing, because every pull-up
  on this bus references the same 3.3 V the sensors run from and falls with them.
- **Superseded rationale, 2026-08-28:** the original reason was that the integrated OLED holds
  SDA/SCL at 3.3 V while a lowered sensor VDD forward-biases an input clamp. That is false for this
  hardware — the ideaspark 1.14 inch board's display is SPI and never touches GPIO21/22 (D-024).
  The decision is unchanged; only its justification is.
- **Signals:** GPIO21/SDA and GPIO22/SCL reach only the external sensor branch through individual
  330 Ω resistors. They limit fault current and damp edges; they are not treated as a measurement
  noise source. `C1`, `C3` and `C4` provide power decoupling. **Revised 2026-08-28 to 100 nF**, the
  confirmed ceramic stock value in `docs/elektronik-inventar.md`, still inside the 47–220 nF range
  this decision allows.
- **VEML7700:** Operator-confirmed five-pin header is VIN/3Vo/GND/SCL/SDA. VIN gets 3.3 V and
  `3Vo` is the breakout regulator output and must stay NC. Bus pull-ups are now fitted on the
  carrier itself — see D-023.
- **Boundary:** This does not approve energising. Connector orders are now confirmed, but the
  controller's physical row spacing and both 5 V modules' supply voltage and output swing are still
  unmeasured.

### D-023 — The Room carrier supplies its own I²C pull-ups

- **Status:** Accepted 2026-08-28, replacing the "no extra pull-ups" position inside D-022.
- **Rule:** `R_PU_SDA` and `R_PU_SCL`, 3.3 kΩ from `SDA_EXT`/`SCL_EXT` to 3.3 V, are **required**
  parts, not optional ones.
- **Value revised 2026-08-28** from 4.7 kΩ to 3.3 kΩ when the build moved to perfboard. Rise time
  binds: at 100 kHz I²C allows 1000 ns, and 4.7 kΩ against roughly 200 pF of hand-soldered bus
  capacitance gives 940 ns — on the limit. 3.3 kΩ gives 660 ns. The operator holds a complete E24
  kit, so no value here is a stock compromise; every choice is tabulated in `wiring.md`.
- **Why:** the previous position assumed an integrated I²C OLED supplied pull-ups. The identified
  board's display is SPI (D-024), so nothing on the controller pulls SDA or SCL up. Without these
  resistors a build populated with only the SHT41 — or with a breakout whose pull-ups are unfitted —
  leaves the bus floating and every transaction fails.
- **Placement:** on the sensor side of the 330 Ω series resistors, so `R_SDA`/`R_SCL` stay a
  fault-current limit rather than part of the pull-up divider.
- **Loading:** with both breakouts carrying their own 10 kΩ pull-ups the parallel result is about
  2.0 kΩ, roughly 1.6 mA at 3.3 V. That is inside the I²C 3 mA sink limit, so they stay fitted
  rather than DNP.

### D-024 — The ideaspark display is SPI, and six GPIOs are off-limits

- **Status:** Accepted 2026-08-28 on operator evidence
  (`docs/hardware/ideaspark-esp32-tft-pinout.png`).
- **Board:** the Room controller is the **ideaspark ESP32 1.14 inch TFT LCD board**
  (ESP32-WROOM-32), not an integrated-OLED variant. Its 30 pins and their order are confirmed;
  pads 1–15 are the left column and 16–30 the right column, both from the USB/button end.
- **Reserved by the display:** GPIO23 (MOSI), GPIO18 (SCLK), GPIO15 (CS), GPIO2 (DC), GPIO4 (RST)
  and GPIO32 (backlight). The carrier must never reach them.
- **Reserved by the board:** GPIO12 is the flash-voltage strap and high at boot can stop the board
  starting; GPIO1/GPIO3 are the CP2102 USB-UART that `task flash` and `task monitor` need; `EN` is
  reset.
- **Enforcement:** `generate_project.py` raises rather than emit a net on a reserved pin, and
  `validate_room_carrier.py` fails if a reserved GPIO appears in the net list. Both were
  mutation-proved on 2026-08-28.
- **Consequence:** knowing the pin order makes the socket *more* dangerous, not less — a builder can
  now confidently reach the wrong pin. Hence the silkscreen and the two gates.
- **Still unverified:** the physical row spacing. 25.4 mm remains an assumption to be measured.

### D-025 — The Room buzzer is driven straight from GPIO25, with no low-side transistor

- **Status:** Accepted 2026-08-28 on operator identification, superseding the low-side-NPN bullet
  inside ROOM-01's acceptance criteria.
- **Part:** a no-name **Keyes 3-pin breakout**, black cylinder with a single hole, header order
  **S / VCC / −**. It is the same part AtmosMesh v1 uses.
- **Rule:** `S` is driven from GPIO25 through `R_BEEP_S` 100 Ω, `VCC` takes 3.3 V, `−` takes GND.
  Logic is active HIGH. `Q_BEEP`, `R_BEEP_IN`, `R_BEEP_PD` and the `D_BEEP` flyback are removed.
- **Why:** `firmware/README.md` records v1 driving this part with a bare 50 ms
  `digitalWrite(HIGH)` on GPIO25, so it is an active module with its own driver. The previous design
  presented a 2-pin header and switched the load's low side, which does not match a 3-pin S/VCC/−
  module at all — `S` would have been left unconnected. No flyback is fitted because an
  internally-driven module keeps the inductive kick behind its own transistor.
- **Residual risk and its bench rule:** some Keyes buzzer boards put the sounder straight across
  `S` and `−` with no onboard transistor. On such a module 100 Ω makes it barely click. The
  resistor therefore starts at the *protective* value; only a measured current under 20 mA
  authorises replacing it with a wire link. The ESP32 GPIO absolute maximum is 40 mA.

### D-026 — The SDS011 joins the Room carrier with series UART resistors and a diodeless 5 V jumper

- **Status:** Accepted 2026-08-28. Operator asked for the SDS011 that already worked on AtmosMesh v1,
  with additional protection.
- **UART:** crossed and protected. Sensor TXD reaches GPIO16/RX2 through `R_SDS_RX` 1 kΩ; GPIO17/TX2
  reaches sensor RXD through `R_SDS_TX` 1 kΩ. Both legs are hard-wired — the operator chose this
  over a default-open TX jumper so laser duty-cycling stays available without rework.
- **Why 1 kΩ:** the 2026-08-17 bench fault put the sensor's push-pull TXD on the ESP32's push-pull
  TX2, and they fought for roughly 10 ms in every second against a 40 mA absolute maximum. 1 kΩ
  bounds that fight to about 3.3 mA. At 9600 baud a bit is 104 µs while 1 kΩ into a few hundred pF
  settles in a couple of hundred nanoseconds, so the protection is free in signal terms.
- **Enforcement:** `generate_project.py` refuses to emit a straight-through SDS011 UART. Mutation-
  proved by swapping the header's RXD/TXD nets.
- **5 V, and the rejected diode:** `JP_SDS_5V` is default-open, with `C6` 10 µF and `C7` 220 nF on
  the protected side. There is **no series Schottky**, breaking from the PIR rail's 1N5819 idiom:
  the SDS011 minimum is 4.7 V and a Schottky drops about 0.3–0.4 V at fan current, landing under the
  minimum before any USB droop. The jumper isolates at zero volts. The generator refuses a diode on
  this rail, also mutation-proved.
- **Assumptions that gate closing the jumper:** `+5V_USB_CONFIRMED` comes from `VIN`, which on many
  dev boards sits behind a diode from USB VBUS and may already read 4.6–4.7 V under load;
  `spec-comparison.md` already puts the shared rail near 650 mA peak coincidence without this
  sensor; and `C6` at 10 µF is not shown to meet the < 20 mV ripple specification against a fan.
  All three are measurements, not conclusions.

### D-027 — The Room build target is a 31 × 27 perfboard; the PCB is parked

- **Status:** Accepted 2026-08-28 on operator direction: "KiCad PCB is not needed right now — we
  only need the schematic", and the board is hand-soldered on perfboard.
- **Board:** 31 × 27 holes at 2.54 mm, sold as 7 × 9 cm. The **hole count is authoritative**; the
  extra millimetres are unholed lead-in strips at both edges.
- **Consequence:** `atmosmesh-room.kicad_pcb` is parked. It is still generated from the netlist and
  still carries the safety silkscreen that `validate_room_carrier.py` checks, but its placement is
  no longer maintained as a fabrication candidate and it must not be ordered.
- **New artifacts:** `perfboard.md` is the build plan and `validate_room_perfboard.py` gates it. A
  build plan with no gate would be exactly the "gates that check nothing" pattern this repo keeps
  hitting; its five assertions are mutation-proved.
- **Orientation is the new principal hazard:** the pinout sheet is a top view and soldering happens
  from the copper side, where left and right invert. Two connector orders on this design were
  already found reversed. The wiring table is therefore authoritative **by pin name**, and the
  validator refuses to let that warning be edited out.
- **Row spacing:** dry-fitting the board into the grid settles it in seconds — 25.4 mm is exactly
  10 pitches. That is step 1 of the build plan, and it discharges the open item in D-024.

### D-028 — Bulk capacitance is chosen for the load, not for the drawer

- **Status:** Accepted 2026-08-28 after `docs/elektronik-inventar.md` was added to the repository.
- **What changed:** the electrolytic assortment covers 0.1 µF to 1000 µF in 24 values, so the
  earlier "matches existing stock" reasoning no longer applies to bulk capacitors.
  - `C6`, the SDS011 5 V bulk: **10 µF → 470 µF**. Its ripple specification is < 20 mV against a fan
    load, and 10 µF was never shown to meet it.
  - `C5`, the PIR 5 V bulk: 10 µF → 100 µF.
  - `C2`, the controller 3.3 V bulk: 10–47 µF → 100 µF.
- **Ceramics went the other way:** the ceramic assortment stops at 100 nF, so `C1`, `C3`, `C4` and
  `C7` move from 220 nF to **100 nF** — inside the range D-022 already permits, and actually held.
- **Consequence:** `JP_SDS_5V` must be closed **before** USB power is applied. Closing it onto a
  live rail dumps a discharged 470 µF into the shared 5 V supply and can brown out the ESP32.
- **Zener position re-confirmed, not changed:** the Zener stock is the 1N47xx series starting at
  1N4733 (5.1 V). That is too late for a 3.3 V GPIO and no 1N4728 is held, so the "no Zener clamp"
  rule stands on evidence rather than assumption.

### D-029 — Each supply domain gets a live indicator; the 5 V ones report through their own jumper

- **Status:** Accepted 2026-08-28, after the LED and E24 resistor stock was recorded.
- **Rule:** `D_LED_3V3` (470 Ω), `D_LED_SDS` and `D_LED_PIR` (1 kΩ each) indicate their rails.
- **Why:** both 5 V jumpers are default-open and the build plan requires them to stay open until
  measurements are recorded. There was no way to see whether a domain was live — on the one board
  in this project whose entire risk story is an unverified 5 V rail.
- **Topology matters:** the 5 V indicators are fed from `SDS_5V_PROTECTED` and `PIR_5V_PROTECTED`,
  i.e. *downstream* of their jumpers. A lit LED is therefore direct evidence that the jumper it
  reports on is closed, not merely that the board has power.
- **Colour is a constraint, not a preference:** the sizing assumes a forward voltage near 2.0 V. A
  blue or white LED drops about 3.0 V, leaving 0.3 V across `R_LED_3V3` on the 3.3 V rail — about
  0.6 mA, which reads as dead. Red or green only.
- **Cost:** about 3 mA per indicator, negligible against a rail already budgeted near 650 mA peak.

### D-030 — The SDS011 duty-cycles itself; no power switch is fitted

- **Status:** Accepted 2026-08-28, replacing the IRLZ34N proposal in ROOM-04.
- **Rule:** no MOSFET, no relay, no switched SDS011 supply. Duty-cycling is a firmware task using
  the sensor's own hibernation command.
- **Evidence:** `docs/hardware/datasheets/nova-sds011.pdf` lists "Manual hibernation (Sleep and wake
  up)" and "Timed hibernate (Cycle to work)" under Extended functionality, with sleep current below
  4 mA annotated "Lase&Fan sleep".
- **Why the MOSFET was rejected, independently of that:** a low-side switch opens the ground return
  while VCC stays at 5 V. That does not de-power the load — its ground floats up until current stops
  and the sensor sits partially biased. High-side switching is required and no P-channel part is in
  stock. This is recorded because the proposal was written into ROOM-04 before the topology was
  checked, and the same mistake is easy to repeat.
- **Boundary:** the byte-level command frames are not in the datasheet held here. Obtain Nova's
  separate control-protocol document before writing firmware; do not guess frame formats.

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

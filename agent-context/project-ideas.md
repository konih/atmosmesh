# Project ideas — backlog of unbuilt AtmosMesh concepts

Not stories. Nothing here is scoped, scheduled, or approved for wiring — these are candidate
product concepts, mostly brainstormed with Fable, kept in one place so a good idea doesn't get
lost between sessions. Promote one to a real story (`stories/<ID>.md`, added to `roadmap.md`) only
when the operator picks it to build.

Every idea here is expected to follow the project's existing discipline once (if ever) it becomes
a real story: raw-value-only reporting (no fabricated calibration/derived claims), fail-safe GPIO
defaults (power switches OFF before `pinMode`), the one shared MQTT contract/discovery pattern, and
photo-verifying parts before wiring — see [decisions.md](decisions.md) and
[../docs/hardware/inventory.md](../docs/hardware/inventory.md).

## Idea inventory

| Name | One-line hook | Key unused/spare parts it claims |
| --- | --- | --- |
| [Drum](#atmosmesh-drum) | Laundry/utility-room appliance-activity node | ESP32-C6, BMI160, INMP441, ADS1115, capacitive soil probes, BME280 |
| [Post](#atmosmesh-post) | Mailbox/gate outstation beyond Wi-Fi range | 433 MHz TX/RX pair, ESP32-C3 SuperMini, ESP32 DevKit, HC-SR501 |
| [Vent](#atmosmesh-vent) | Headless kitchen/bathroom extractor-fan companion | SGP41, BME280, ESP32-C6, second ENS160+AHT20 |
| [Hall](#atmosmesh-hall) | Stairwell/hallway PIR array for direction-of-travel | 5× HC-SR501, ESP32 DevKit, IRLB8721, BMI160, VEML7700 |
| [Chill](#atmosmesh-chill) | Second brain for the fridge/freezer compressor | ESP32-C6, ADS1115, DS18B20, BME280, current clamp (buy) |
| [Sprout](#atmosmesh-sprout) | Seedling-shelf / grow-light companion | ESP32-C3 SuperMini, SGP40, soil probes, DS18B20, VEML7700, IRLZ34 |
| [Ear](#atmosmesh-ear) | 868 MHz sniffer bridge for store-bought RF sensors | classic ESP32 DevKit, RFM12S, spare OLED |
| [Gift](#atmosmesh-gift) | Giftable CYD touchscreen air-quality + weather station, self-provisioning | ESP32-2432S028 CYD, ENS160+AHT20, BME280 |

---

### AtmosMesh Drum

Laundry/utility-room node. Every existing product answers "what is the air like here?"; nothing
answers "what are the machines doing, and is the floor wet?" Drum sits on the washer/dryer and
turns vibration, sound, and floor moisture into raw MQTT features Home Assistant can act on —
"wash idle 3 min after a spin burst, send the done notification," "dryer still tumbling at
23:40," "the tray under the machine just went wet." Publishes measurements only; "cycle finished"
stays a Home Assistant automation on raw numbers, exactly like the MQ135-is-not-CO2 rule.

- **ESP32-C6** — first C6 target in the fleet; its 802.15.4 radio leaves Thread/Zigbee open later.
- **BMI160** — magnetically mounted on the machine casing; per-axis RMS acceleration + a few
  band-energy bins (raw g, fixed windows). Spin/tumble/drain/idle each have distinct signatures.
- **INMP441** — I2S loudness envelope + coarse spectral bands in dBFS (not calibrated dB SPL).
  Catches the end-of-cycle beep and general utility-room noise level.
- **ADS1115 + spare capacitive soil probes** — four probes (drip tray, behind the machine, under
  the sink, floor drain) on one 16-bit ADC, raw counts, duty-cycled like Aqua's water probe.
- **BME280** — room humidity/pressure/temp; a badly-vented dryer shows as a humidity ramp
  correlated with the vibration timeline.
- **Spare 128×32 OLED** — local activity bar, wet/dry counts, MQTT link state.

### AtmosMesh Post

Mailbox and garden gate are out of Wi-Fi range and nothing in the fleet reaches them. A battery
ESP32-C3 sleeps until a reed switch (mailbox flap) or HC-SR501 (gate) wakes it, fires a short
433 MHz OOK burst (node id, event type, sequence counter, raw battery ADC) a few times, and sleeps
again. An indoor ESP32 DevKit with the 433 RX listens and republishes over the normal MQTT
contract, plus a packets-seen/sequence-gap counter so link health is a fact, not a guess. One-way
is fine: mail arrived, gate opened, shed door opened — Home Assistant does "mail delivered today."

- **433 MHz TX/RX pair** — first use; outstation sends, indoor bridge receives.
- **ESP32-C3 SuperMini** (spare) — deep-sleep sender; OLED left unpopulated.
- **ESP32 DevKit** (spare) — always-on indoor receiver bridge, mains-powered.
- **HC-SR501** (spare) — wake-on-approach at the gate; reed switch on the mailbox flap.
- **AMS1117 / resistor divider** — battery voltage as raw ADC only.
- **Worth ordering:** a reed switch + magnet (~$2) and an RXB6 superheterodyne 433 MHz receiver
  (~$3) — the regenerative RX in the current pair is noisy enough to flood the bridge with garbage.

### AtmosMesh Vent

A headless companion for the kitchen or bathroom extractor fan. Room and Spot see PM and gas
trend; nobody sees cooking, frying, cleaning-product, or shower-steam events specifically. Vent
publishes raw SGP41 VOC/NOx ticks (NOx catches gas-hob combustion, which SGP40 can't) plus BME280
humidity and its rate of change — never a fabricated "air quality index." Home Assistant switches
the fan smart plug on a VOC-tick jump or humidity slope and off after recovery. Deliberately no
OLED — it's a wall wart that disappears. The spare ENS160+AHT20 makes a second unit for the
bathroom, giving two independently-axised VOC readings to sanity-check against each other.

- **SGP41** — raw VOC + NOx signal ticks (first use in the fleet).
- **BME280** (spare) — humidity/pressure/temp; humidity compensation input to SGP41.
- **ESP32-C6** (spare) — headless, mains-powered.
- **Second ENS160+AHT20** — bathroom sibling unit, a second VOC axis.

### AtmosMesh Hall

Spot's radar knows *if* someone is present; nothing knows *which way they went*. Hall strings four
or five HC-SR501s along the stairwell and hallway, each on its own GPIO, publishing raw
per-sensor trigger timestamps only. Home Assistant derives "went down to the cellar," "front door
to kitchen," "bedroom to bathroom at 03:00." The one local action: a fail-safe stair LED strip
(IRLB8721, gate pulldown, default OFF before `pinMode`) that lights only at night on the first
trigger — an actual safety feature, not just a logger. A spare BMI160 on the front door leaf adds
an honest open/close vibration signature without needing a magnetic reed sensor.

- **5× HC-SR501** (spare) — direction-of-travel array.
- **ESP32 DevKit** (spare) — plenty of GPIOs for five PIRs plus the strip.
- **IRLB8721 + resistor stock** — fail-safe LED strip driver.
- **BMI160** (spare) — door-slam / open signature, raw g values only.
- **VEML7700** (spare) — ambient lux gate so the strip stays off by day.
- **Worth ordering:** ~1 m of 12 V warm-white LED strip (~$6) — the one part not in inventory, and
  what actually makes Hall a household feature rather than a pure logger.

### AtmosMesh Chill

A second brain for the fridge/freezer — the one appliance that runs 24/7 and fails silently. A
split-core current clamp on the compressor feed gives duty-cycle and stall detection; a DS18B20 in
the cabinet and a BME280 in the freezer drawer catch frost build-up and a door left ajar before
the food does. Publishes raw clamp millivolts (HA derives on/off and duty %), raw cabinet
temperature, and door-open seconds — never "energy cost" or "food safety." A compressor that never
rests, or rests too long, is the earliest warning of a dying fridge, and nothing in the fleet
watches it.

- **ESP32-C6** (spare) — host, mounted on top of the fridge near mains.
- **ADS1115** (spare) — reads the clamp's burden resistor at 16-bit, plus a spare channel for a
  second clamp (e.g. a chest freezer).
- **Spare DS18B20** — cabinet probe through the door seal.
- **BME280** (spare) — freezer-drawer humidity/frost proxy.
- **Reed switch** (from stock) + magnet — door state.
- **Worth ordering:** an SCT-013-000 100 A split-core current clamp (~$8) — non-invasive, no mains
  contact, matches the fleet's fail-safe values.

### AtmosMesh Sprout

A seedling-shelf and grow-light companion for an indoor propagator. VEML7700 under the lamp logs
raw lux every minute so HA can integrate a daily light total; soil probes in two trays report raw
ADC; a DS18B20 sits in the root zone where a heat mat actually matters. The untouched SGP40 lives
inside the closed propagator lid: a steady rise in raw VOC ticks in a sealed humid box is an
early, honest damping-off/mould signal without pretending to be a calibrated "mould ppm." An
IRLZ34 switches the grow light on an HA schedule, defaulting OFF at boot.

- **ESP32-C3 SuperMini OLED** (spare) — host; the tiny OLED shows lux and tray moisture.
- **SGP40** (untouched) — raw VOC ticks under the propagator lid.
- **2× capacitive soil probes** (remaining after Drum) — tray moisture.
- **Spare DS18B20, spare VEML7700, BME280** (spare) — root-zone temp, light, ambient reference.
- **IRLZ34 + flyback diode** (from stock) — 12 V LED grow-bar switch.
- **Worth ordering:** nothing required; a 12 V LED grow bar (~$15) if one isn't already owned.

### AtmosMesh Ear

The RFM12S finally gets a job: a receive-only 868 MHz FSK sniffer that adopts store-bought sensors
already common in homes (the LaCrosse/TFA IT+ family and many cheap fridge/outdoor thermometers
use exactly this RFM12-compatible framing). Every decoded frame publishes alongside its raw bytes
and RSSI, so unknown devices still land in MQTT as raw frames for later decoding. It isn't an
outstation — it's a bridge that turns other people's hardware into fleet members, the cheapest way
to add a freezer, greenhouse, or attic reading without building another node from scratch.

- **Classic ESP32 DevKit** (spare) — SPI host with headroom for frame buffers.
- **RFM12S** (untouched) — 868 MHz FSK receiver.
- **Spare 128×32 OLED** — last-heard sensor ID and RSSI.
- **Worth ordering:** a TFA 30.3180 / LaCrosse TX29DTH-IT sensor (~€12) as a known-good reference
  transmitter — it doubles as the freezer probe Chill would otherwise need to wire.


### AtmosMesh Gift

**This one has a full design doc already: [`../docs/design/atmosmesh-gift.md`](../docs/design/atmosmesh-gift.md),
and an open decision in [`INBOX.md`](INBOX.md).** It is further along than the rest of this file.

A desk object built on a Sunton `ESP32-2432S028` "Cheap Yellow Display" that shows indoor air
quality and outdoor weather, and is **set up entirely by the person who receives it** — Wi-Fi typed
on the touchscreen, location picked on the touchscreen, no serial cable, no repo, no account, no
API key. Every other AtmosMesh product assumes the operator, a build-time `secrets.hpp` and a Home
Assistant to talk to; this one assumes none of them, so the genuinely new subsystems are **runtime
credential storage in NVS** and **a settings UI**.

- **ESP32-2432S028 CYD** (2 unreserved) — 2.8" 240x320 touch TFT, RGB LED, LDR, speaker. Its real
  limitation is pins, not speed: **three free GPIOs, one input-only**, so every sensor is I²C on
  the CN1 header.
- **ENS160 + AHT20** (free, the spare of two) — the gas sensor, plus the AHT20 that feeds its
  compensation. Per D-036 its `eCO2` output *is* shown and drives the "open a window" verdict, but
  always labelled as derived (`eCO2 ~1240 est.`) — it is computed from VOC, not measured, and only
  the SCD41 may carry a bare CO2 label.
- **BME280** (free, 6 in stock) — the temperature and humidity the screen actually shows, on its
  own lead away from both the ENS160's hotplate and the board's heat.
- **Onboard RGB LED** — a slow ambient glow in the air-quality colour, readable across a room
  without looking at the screen.
- **Open-Meteo** (keyless, no signup) — outdoor conditions, forecast, and the outdoor PM2.5 / AQI
  that stands in for the fan-based particulate sensor a silent gift cannot have.

Deliberately no *measured* CO₂ (D-036): the SCD41 is the expensive part and the one free unit is
reserved for Room v2, and the brief is a gift rather than an instrument. The ENS160's derived eCO2
carries the ventilation prompt instead, labelled as an estimate. The input-only `GPIO35` stays free
for a ~EUR 20 MH-Z19C if a real NDIR reading is ever wanted.

# AtmosMesh Aura — design and architecture proposal

- **Status:** Proposal. Nothing here is approved, no part is reserved, no wiring is authorised.
- **Date:** 2026-09-20
- **Target hardware:** Sunton `ESP32-2432S028` "Cheap Yellow Display", 2 units on hand
- **Related:** [ADR-0001](../adr/0001-multi-product-firmware-composition.md) (multi-product firmware),
  [D-002](../../agent-context/decisions.md) (MQ135 is not CO₂),
  [D-007](../../agent-context/decisions.md) (MQTT contract),
  [D-019](../../agent-context/decisions.md) (MQTT is the sole transport),
  [inventory.md §CYD](../hardware/inventory.md), [power.md](../hardware/power.md)

## 1. What is being built, and the constraint that defines it

A self-contained desk object that shows indoor air quality and weather on a colour touchscreen,
and is **set up entirely by the person who receives it** — Wi-Fi typed on the screen, location
picked on the screen, no serial cable, no repo, no account, no API key.

That last clause is the whole design. Every other AtmosMesh product assumes the operator, a
`secrets.hpp` baked at build time, and a Home Assistant instance to talk to. A gift assumes none of
them. So the two genuinely new subsystems are **runtime credential storage** and **a settings UI**,
and the sensor and MQTT choices follow from giftability rather than from what the drawer contains.

### The giftability contract

| # | Rule | Consequence |
| --- | --- | --- |
| G1 | The recipient never sees a terminal | Wi-Fi, location and units are set on-screen |
| G2 | No account, no API key, no subscription | Open-Meteo (keyless) for outdoor data; never OpenWeatherMap |
| G3 | It is silent | No fan. This disqualifies every PM sensor with a fan — see §5 |
| G4 | It is useful with no network at all | Onboard sensors render offline; the network adds outdoor data, clock and optional MQTT |
| G5 | It never lies about a number | An estimate may be shown, and must be labelled as one: only a real NDIR-class part may carry a bare `CO₂` label (D-002, `inventory.md:415`). A stale reading is never a zero |
| G6 | It is recoverable without tools | On-screen "forget network"; a fallback setup access point |

## 2. Reference survey

Nine upstream projects were cloned to `references/` (gitignored; see `references/README.md` for the
table and the upstream URLs). What each one is actually worth:

| Project | Verdict | What to take |
| --- | --- | --- |
| witnessmenow/ESP32-Cheap-Yellow-Display | **The hardware source of truth.** `PINS.md`, `DisplayConfig/`, `TROUBLESHOOTING.md` | The pin map in §3, the two `User_Setup.h` variants, the boot-capacitor and USB-C errata |
| Heftie/esp32-2432S028R | **The provisioning model to copy structurally.** ESP-IDF | Its state machine: on-screen flow first-class, SoftAP captive portal as fallback, NVS `ssid`/`pass`, double-tap-to-arm "forget network", scan off the UI task |
| nicholaswilde/cyd-weather-station | **The UI model to copy.** LVGL 8, PlatformIO/Arduino, hand-written | Runtime-switchable palette token table, icons compiled as an LVGL *font* (recolourable, tiny) rather than PNG blobs, bottom tabview with swipe animation, themed `lv_chart`, `"--"` placeholders + a warning banner, RGB-LED status states with gamma-corrected PWM, backlight fade |
| HenrysCat/ESP32-2432S028-SCD40x | **The wiring precedent, not a UI.** Raw TFT_eSPI text | It confirms CN1 as the sensor header and `Wire.begin(27, 22)` on this exact board. Its UI redraws the whole screen each cycle and shows a dead sensor as frozen numbers — the anti-pattern G5 exists to prevent |
| limpens/esp32-2432S028R | LVGL 9 on ESP-IDF with `esp_lvgl_port` | The pattern of LVGL in its own task behind a mutex, so a slow I²C read cannot stall rendering |
| pangcrd/cyd-esp32-touchscreen-qr-web-ui | **Mostly a cautionary tale** | Borrow its live "Wrong Password" label. Avoid: it persists credentials *before* testing them, has no captive-portal fallback, no "forget network", and pins its keyboard at hardcoded pixel coordinates. Its QR code is not a Wi-Fi-join QR — it links to the device's own web page after it is already online |
| tzapu/WiFiManager | Reference only | Naming (`resetSettings()`), and the non-blocking `process()` pattern if a bespoke portal is ever abandoned |
| dcoric/weather-station | Weak reference | A second LVGL layout for comparison; its 3.4 MB of PNG icons is exactly what the font-icon trick avoids |

**Nothing from these repos gets imported.** They are read; AtmosMesh code is written.

## 3. The board, and what it actually allows

Confirmed on unit 1 by the 2026-09-19 USB probe (`inventory.md:594`): ESP32-D0WD-V3 rev 3.1,
**4 MB flash**, CH340 bridge. The display and touch controllers are **still unidentified**.

### Pin map (community reference — a hypothesis to verify at bring-up, not a datasheet)

| Function | GPIO |
| --- | --- |
| TFT (HSPI) | MISO 12, MOSI 13, SCK 14, CS 15, DC 2, backlight 21 (PWM) |
| Touch XPT2046 | CLK 25, MOSI 32, CS 33, MISO 39, IRQ 36 — its own bus; community uses a bit-banged driver |
| microSD (VSPI) | CS 5, SCK 18, MISO 19, MOSI 23 |
| RGB LED (active low) | R 4, G 16, B 17 |
| Speaker | 26 |
| LDR | 34 |
| BOOT button | 0 |

### The free-pin budget — the single most important fact

**Three pins, one of them input-only.**

| Header | Pins |
| --- | --- |
| **CN1** (4-pin PicoBlade) | GND, **GPIO22**, **GPIO27**, 3V3 — the designated sensor header |
| P3 (4-pin PicoBlade) | GND, **GPIO35** (input-only, no internal pull-up), GPIO22, GPIO21 (backlight — not usable) |

So: `GPIO22` and `GPIO27` as a general-purpose pair, `GPIO35` as a spare input, `GPIO0` (BOOT)
usable as a runtime button. **There is no free UART and no free DAC.** That means:

> **Every sensor on this product must be I²C, and they all share one bus on CN1.**

HenrysCat's working sketch uses `Wire.begin(27, 22)` — SDA 27, SCL 22. Adopt that orientation so
the precedent and the 4-pin pigtail colours line up.

**Pull-ups are an open question, not a settled one.** The community reports the ESP32's internal
pull-ups being adequate. This repo has evidence against relying on that: the photo-verified SHT41
breakout has its host-side pull-up pads **empty** (`inventory.md:423`), and Sensirion's own SCD4x
application circuit shows external 10 kΩ. Plan for 4.7 kΩ to 3V3 on both lines at the sensor end,
and measure before trusting a bus.

### Power

The CYD's 3.3 V LDO has little headroom once the panel and backlight are lit (`inventory.md:584`),
so the sensor choice in §5 is partly a power decision. **With the chosen set — ENS160 + AHT20 and
BME280 — this is not expected to be a problem:** the BME280 is a microamp part, and while the
ENS160 drives a metal-oxide hotplate and is therefore not one, it is nowhere near the 205 mA pulse
of the CO₂ sensor this design rejected. **Take the ENS160's actual figure from its datasheet and
then measure it at AU-02** — this repo does not take heater currents from memory
(`inventory.md:415`), and "not expected to be a problem" is a hypothesis until the rail is watched.

It matters only if a **CO₂ sensor is added** (§5, optional). For the record, from the Sensirion
SCD4x datasheet v1.5 Table 4 at VDD = 3.3 V — the part this design deliberately does *not* use:

| Mode | Average | Peak |
| --- | --- | --- |
| Periodic, 1 measurement / 5 s | 15 mA typ, 18 mA max | **175 mA typ, 205 mA max** |
| Low-power periodic, 1 / 30 s | 3.2 mA typ, 3.5 mA max | same peak |
| Single shot, 1 / 5 min (SCD41 only) | 0.45 mA typ, 0.5 mA max | same peak |

That 205 mA pulse, arriving while the ESP32 transmits and the backlight is at full, is the
brown-out scenario — and it is why any CO₂ option here takes its power from the **USB 5 V input,
not from CN1's 3V3**, with local decoupling (100 µF bulk + 100 nF) at the sensor end. The SCD4x
datasheet also asks for < 30 mV unloaded supply ripple and recommends the sensor have its own LDO.

### Errata to design around

- **Display controller unknown, and it has already bitten once.** `2432S028` ships with ILI9341
  *or* ST7789, differing in colour inversion, RGB/BGR order and column offset. The community
  heuristic is that **dual-USB boards (Micro-B + Type-C) are the ST7789 "CYD2USB" variant with
  inverted colours** — and the inventory records the operator's units as having *both* connectors
  (`inventory.md:567`). **Working hypothesis: these are ST7789, colour-inverted.**
  **2026-09-20: a unit running firmware from another project shows the left ~25 % of the panel
  blank.** In landscape that is 320 px across and 25 % of 320 is **80** — exactly the
  `CGRAM_OFFSET` an ST7789 config applies for a panel that does not need it. This is the predicted
  failure mode arriving on schedule, and the diagnosis is written up in
  `inventory.md` under "CYD — operator chip reading and a display fault". It is also *useful*:
  whichever setting renders full width identifies the controller and answers AU-01.
- **The touch controller is now confirmed**: the operator read `XPT2046` off the PCB on
  2026-09-20, so these are resistive `R` units, and §4.2's objection stands on evidence rather
  than on the supplied stylus. The display controller is **not** readable this way — it is
  chip-on-glass under the flex.
- **Never connect both USB ports at once** — one bridge, two sockets.
- Some Type-C sockets on this board lack CC resistors and will not enumerate with a C-to-C cable.
- Touch shares no bus with the TFT; use the bit-banged XPT2046 driver to avoid contention.
- **4 MB flash.** An Arduino + LVGL + Wi-Fi image is comfortably under 2 MB, so a dual-OTA partition
  layout fits — but only just, and it must be decided before the first flash, not after.

## 4. Honest objections to the premise

Per the workspace's adversarial-collaboration rule, three things about this brief are worth
challenging before any part is bought.

**1. Self-heating will corrupt the temperature, and this is the classic failure of every all-in-one
CYD air station.** The ESP32, the TFT driver and the backlight sit centimetres from anything
plugged into CN1. Expect a **+2 to +5 °C** offset on a sensor mounted against the board, and an
RH error that follows it. A gift that reads 26 °C in a 22 °C room is a broken gift, and no firmware
offset fixes it honestly across ambient conditions. **Design answer:** the sensors live on the far
end of the CN1 pigtail, physically separated from the board and in their own airflow — the
enclosure decision (§9) is therefore a *measurement* decision, not a cosmetic one.

**2. Resistive touch with a supplied stylus is not "modern and nice" to the hand.** The units on
hand are the `R` variant. A finger on a resistive panel needs deliberate pressure and does not
glide; next to any phone it feels dated, which is exactly the wrong impression for a gift. The
capacitive `C` variant (GT911) exists. **Recommendation:** if a CYD is being bought for the gift
rather than using the two on hand, buy the `C` variant. If the on-hand `R` units are used, the UI
must be designed for *pressed taps on large targets*, never for swipe gestures or small controls —
which meaningfully constrains §7.

**3. The CYD may simply be the wrong board for a sensor product.** Three free pins, a weak LDO and
a hot PCB are a display board's characteristics, not a sensor node's. It is a defensible choice —
it is cheap, it is the single most documented ESP32 display in existence, and two are already in a
drawer — but it is a choice, and the cost is paid in §3. An ESP32-S3 board with a separate panel
would cost more and lose all that documentation. **Recommendation: proceed with the CYD,** with the
constraints above written down rather than discovered later.

## 5. Sensors

**Operator decision, 2026-09-20 (D-036):** ENS160 + AHT20 for gas — exact measurements are not
required for this product — with BME280 for climate rather than spending the fleet's last SHT41.
The SCD41 (~€30–45, and the one unit in stock is reserved for Room v2) is out.

### The build — €0, both parts already free in the drawer

| Part | Measures | I²C addr | Stock | Role |
| --- | --- | --- | --- | --- |
| **ENS160 + AHT20** combo | ENS160: TVOC and an air-quality index. AHT20: T and RH | `0x52` or `0x53` (ADDR pin) + `0x38` (fixed) | 1 free (of 2; the other is Room v2's) | The gas sensor, and the ENS160's compensation climate source in one module — one cable for both |
| **BME280** | T, RH, pressure | `0x76` (SDO low) | 6 free | The **room** climate reading, on its own short lead away from the ENS160's hotplate. See below for why this is not redundant |

No address collisions: `0x52`/`0x53`, `0x38`, `0x76` are all distinct. No fan, no 5 V rail, nothing
needing a pin this board does not have. Cost: a 4-pin pigtail and two pull-up resistors.

**Why a BME280 when the module already carries an AHT20.** The AHT20 sits on the same small PCB as
the ENS160, whose metal-oxide hotplate runs warm by design. Its temperature is therefore a *sensor
compensation* input, not a room reading — it will read high, and how high depends on the ENS160's
duty cycle. §4.1 already says the board's heat is the main threat to an honest temperature; putting
the room's thermometer on the gas module repeats that mistake in miniature. So: **the AHT20 feeds
the ENS160's compensation, and the BME280 on its own lead is what the screen shows.** The firmware
must keep these two straight and never average them.

**The SHT41 stays on the shelf.** Exactly one is free, and it is the fleet's only spare across
Room, Room v2 and Spot. Against six free BME280s, and against an operator brief that says exact
measurements are not required, spending it here would be a poor trade — ±3 % RH versus ±1.8 % is
invisible on a gift's display. If a second Spot or a Room repair ever needs it, it is still there.

**Two things about the ENS160 that must be handled, not assumed:**

- **It needs warm-up.** ScioSense state roughly **3 minutes** before readings are meaningful, plus
  a longer first-use conditioning period. That is precisely what §8's `WARMING_UP` state exists
  for: on a gift's first power-on the screen must say "Warming up…" and show `"--"`, not a wrong
  number that later moves. Take the exact conditioning figure from the datasheet at AU-02 — this
  repo does not take heater or warm-up numbers from memory (`inventory.md:415`).
- **Its current is a hotplate current, not a microamp one.** It will not trouble the 3.3 V rail the
  way the SCD41's 205 mA pulse would, but the figure goes in `inventory.md` measured, not guessed.
  The module carries a regulator for the ENS160's 1.8 V core; **feed it 3V3** until that regulator
  and the board's pull-ups have been inspected.

### The eCO₂ register: displayed, but never labelled bare "CO₂"

**Operator direction, 2026-09-20:** the product wants *useful* information, not laboratory
accuracy — "CO₂ is high" is worth showing even when the ppm figure is not trustworthy. That is
the right call for this product, and an earlier draft of this document was wrong to exclude the
ENS160's `eCO₂` output entirely. What follows is the corrected rule and the evidence for it.

**What `eCO₂` actually is.** The ENS160 has four metal-oxide elements and **no CO₂-sensitive
element at all**. ScioSense are explicit about how the register is produced (datasheet v1.3 §5.2):

> "The ENS160 **reverses the proportional correlation of VOCs and CO₂, by providing a standardized
> output signal in ppmCO₂-equivalents from measured VOCs plus hydrogen**, thereby adhering to
> today's CO₂ standards."

and the front-page footnote: *"eCO2 = equivalent CO2 values for **compatibility with HVAC
ventilation standards**."* So `eCO₂` is the VOC measurement re-expressed in ppm units, on purpose,
so it can drive the ventilation logic that HVAC equipment already speaks.

**Why that is good enough here.** In a home the dominant source of both VOC bio-effluents and CO₂
is the same thing — people. They co-vary, which is why ScioSense's own Figure 3 shows `eCO₂`
tracking a reference NDIR sensor closely across two meeting sessions, and why their Figure 4 argues
`eCO₂` is *better* than NDIR in a bedroom or bathroom, because it also catches odours and
bio-effluents a pure CO₂ sensor is blind to. Driving a ventilation prompt from it is the sensor's
designed purpose, not an abuse of it. **So the Aura shows a stuffiness reading and does say "open a
window."**

**The one failure mode to know about, stated once and then designed around.** The correlation runs
through VOCs, so it breaks where the two decouple:

- **False alarm (harmless):** a squirt of window cleaner or a hot pan can push `eCO₂` past 2000
  while real CO₂ has not moved. The device says ventilate; ventilating is never wrong.
- **False quiet (the one that matters):** CO₂ from a source that emits little VOC — a gas hob, a
  wood burner, fermentation — climbs without moving `eCO₂` much. A device that promised "CO₂"
  would be silently wrong in exactly the case a CO₂ monitor is bought for.

That second case is why the **label** matters even though the **signal** is useful.

### The one hard rule on this part

**The reading is shown. The word "CO₂", unqualified, is not used for it.** This is a labelling
rule, not a precision rule, and it is narrower than the earlier draft:

| Allowed | Not allowed |
| --- | --- |
| A stuffiness / freshness band and arc driven by `eCO₂`, with a verdict including *"open a window"* | A bare `CO₂` label, or a headline number presented as a CO₂ measurement |
| The figure shown in the detail row as **`eCO₂ ~1200 (estimated from VOC)`** | `1200 ppm CO₂` |
| MQTT publication under an `eco2_estimated` key, with its own Home Assistant name | Publishing it as the `co2` entity, where Home Assistant and any dashboard will treat it as measured |
| A Settings "About the sensors" line explaining it is estimated | Silence about how it is produced |

The reason is recorded and pre-existing: `inventory.md:415` already states that the ENS160's eCO₂
is *"an eCO₂ estimate derived from VOC — not a CO₂ measurement; only the SCD41 may be labelled
CO₂."* [D-002](../../agent-context/decisions.md) governs the same ground for the MQ135. Note the
difference between the two parts, because it is real and the earlier draft blurred it: the MQ135's
"CO₂ ppm" is a hobby formula applied to an uncalibrated resistance and is simply invented, whereas
the ENS160's `eCO₂` is a vendor-engineered, NDIR-validated output with a documented purpose. The
ENS160 is allowed on screen. It is still not allowed to be called CO₂.

**`aura_view_model` carries the host test** — asserting that the eCO₂ value always reaches the UI
and MQTT through its estimated-label path, and that no code path emits it under a bare CO₂ name or
entity id. The test now guards the label rather than suppressing the value.

### Still free in stock, if a later revision wants better

| Part | Measures | I²C addr | Note |
| --- | --- | --- | --- |
| **Sensirion SGP41** | VOC Index + NOx Index, 1–500 | `0x59` | 1 free. A genuinely better gas sensor than the ENS160 — its gas-index algorithm self-baselines to the room and needs no conditioning ritual. Not chosen here because the ENS160 module brings its own climate sensor in the same package and exact measurement is not the brief. A straightforward upgrade later; it does not clash with anything above |
| **SGP40** | VOC Index only | `0x59` | 1 free. The SGP41's predecessor; same address, so the two cannot share a bus |
| **Sensirion SHT41** | T, RH | `0x44` | 1 free — **deliberately left on the shelf** as the fleet spare, see above |

### If CO₂ is wanted after all, the cheap way in

A true CO₂ measurement means NDIR, and **there is no cheap I²C NDIR part** — that is precisely why
the SCD41 costs what it does. The budget option is the **Winsen MH-Z19C** at roughly €15–25, about
half the SCD41:

- Real NDIR, 400–5000 ppm. **UART or PWM output — not I²C.**
- 5.0 V supply, < 40 mA average, 125 mA peak → it takes power from the **USB 5 V input**, not CN1.
- It fits this board's pin famine surprisingly well: read its **PWM output on GPIO35**, which is
  input-only and otherwise useless, and CN1's GPIO22/27 stay free for the I²C sensors.
- Costs: a coarser reading than the SCD41, an ABC auto-calibration that assumes the room hits
  fresh air weekly, a warm-up period, and a second power wire — so the pigtail stops being a
  plug-in job. Signals are 3.3 V TTL, so no level shifting is needed, but this must be confirmed
  against the actual module before anything is energised.

*Recommendation: ship the €0 ENS160 + AHT20 + BME280 build, and treat the MH-Z19C as a later
addition if the ventilation prompt turns out to be missed. Nothing in the chosen build forecloses
it — GPIO35 stays free precisely so it can be added without redesigning the pigtail.*

### Deliberately excluded, with reasons

| Part | Why not |
| --- | --- |
| **Any fan-based PM sensor** (SDS011, SPS30, PMS5003) | Audible, 5 V at 70–200 mA, needs a duct, and the SDS011 needs a UART this board does not have. Fails G3 outright. **Instead: show outdoor PM2.5/PM10 and European AQI from Open-Meteo's free air-quality API** — no hardware, no noise, no cost, and it is the number that actually varies day to day |
| **MQ135** | Not a CO₂ sensor (D-002), 5 V heater at ~190 mA the CYD's LDO cannot serve, and it needs an ADC pin. Every reason to reject it applies at once. It is also the *cheap gas sensor* this project has already decided against, on purpose — the SGP41 is the honest version of the same idea |
| **MQ-7, MQ-3** | Same family, same objections, and neither measures anything a home gift should claim |
| **VEML7700** | Redundant — the board has an LDR on GPIO34, which is enough to auto-dim a backlight |

### If particulates are non-negotiable

Buy a **Sensirion SEN55** — PM1/2.5/4/10 plus VOC, NOx, T and RH in one I²C part, replacing both
chosen sensors. It costs substantially more than the SCD41 that D-036 declined on price grounds,
so it only makes sense if particulates are the point rather than a nice-to-have. Two caveats change
the build: it **needs 5 V** (CN1 supplies only 3.3 V, so it must be tapped off the USB input
separately), and it **has a fan**, so G3 is being traded away deliberately rather than by accident.

## 6. Firmware architecture

AtmosMesh Aura becomes a fourth product under ADR-0001's existing four-layer scheme — one
PlatformIO project, one explicit composition root, shared host-tested domain code.

```text
firmware/
  include/atmosmesh/
    aura_pins.hpp            CYD pin map + CN1 I2C assignment, one place
    aura_profile.hpp         ProductProfile entry: atmosmesh-aura-v1
    air_band.hpp             TVOC / AQI -> named band + verdict wording [host-tested]
    reading_state.hpp        WARMING_UP | OK | STALE | FAULT            [host-tested]
    wifi_credentials.hpp     SSID/PSK validation, NVS record shape      [host-tested]
    provisioning.hpp         the provisioning state machine             [host-tested]
    aura_view_model.hpp      every string the UI draws, incl. "--"      [host-tested]
    aura_openmeteo.hpp       JSON -> outdoor reading struct             [host-tested]
    aura_mqtt_contract.hpp   AuraMqttState, extends the D-007 contract  [host-tested]
  src/
    air_band.cpp  reading_state.cpp  wifi_credentials.cpp
    provisioning.cpp  aura_view_model.cpp  aura_openmeteo.cpp     <- native env
    aura_display.cpp      LovyanGFX panel + LVGL bind      \
    aura_touch.cpp        XPT2046, bit-banged               |
    aura_sensors.cpp      ENS160+AHT20 + BME280, one bus   |  device only,
    aura_nvs_store.cpp    Preferences/NVS                   |  excluded from
    aura_net.cpp          Wi-Fi + NTP + Open-Meteo client   |  the native env
    aura_ui_*.cpp         one file per screen              /
    products/atmosmesh_aura_v1.cpp   the composition root
```

**The rule that makes this testable:** anything that decides *what* to show is a pure function in
the native env with Unity tests written first (AGENTS.md workflow step 3, GUIDELINES §A3). Anything
that touches a pin or a socket is a thin device-only file with no logic in it. `aura_view_model`
is the seam — it turns readings and state into the exact strings and colour-band enums the LVGL
layer paints, so the screen's behaviour on a dead sensor is a host test, not a bench observation.

**Two runtime tasks, not one loop.** Following limpens' pattern: LVGL owns a task and a mutex; a
second task polls I²C and the network and publishes into a small shared snapshot. A slow I²C
conversion or a stalled HTTP fetch must never freeze the UI — on a gift, a frozen screen reads as
broken hardware.

**Display abstraction — this is where the unknown controller gets handled.** Use **LovyanGFX**
rather than TFT_eSPI. TFT_eSPI selects its panel at *compile* time, so an ILI9341 build flashed to
an ST7789 unit gives inverted colours and a rebuild; LovyanGFX constructs its panel object at
runtime, so one image can probe the controller ID and configure inversion and RGB/BGR order on the
spot. With two units of unverified and possibly differing variants (§3), that is worth the cost of
not being able to copy nicholaswilde's TFT_eSPI setup verbatim. **LVGL 9**, since it is current and
nothing here depends on LVGL 8 source.

**No `secrets.hpp`.** The gift target must build and run with no credentials compiled in at all —
that is the point. Credentials live in NVS namespace `atmosmesh`, keys `wifi_ssid`, `wifi_pass`,
and optionally `mqtt_host` / `mqtt_user` / `mqtt_pass`. The existing products keep `secrets.hpp`
unchanged; this is an addition, not a migration.

**MQTT stays exactly as D-007 and D-019 define it,** and ships **disabled**. A gift recipient with
no broker must never see an error about one. Settings gains an "Advanced → Home Assistant" panel
that, when filled in, brings up the existing `mqtt_session` machinery with an `AuraMqttState` built
from the same `MqttReading{value, valid, age_ms}` shape the other products use — so a stopped
sensor reaches Home Assistant as *unavailable*, never as clean air.

## 7. Wi-Fi provisioning

Structurally Heftie's, with its two weaknesses fixed and one thing neither reference does.

```text
         ┌──────────────┐
  boot ─►│  LOAD_CREDS  │
         └──────┬───────┘
         none   │   found
      ┌─────────┴─────────┐
      ▼                   ▼
┌───────────┐      ┌─────────────┐  ok   ┌────────┐
│  WIZARD   │      │ CONNECTING  ├──────►│ ONLINE │
│ scan list │      └──────┬──────┘       └───┬────┘
│  keyboard │   timeout/  │                  │ link lost
└─────┬─────┘   auth fail │                  ▼
      │                   ▼            ┌───────────┐
      │            ┌─────────────┐     │ RETRYING  │
      │            │   FAILED    │     │ backoff   │
      │            │ reason shown│     └───────────┘
      │            └──────┬──────┘
      │                   │ 3 failures, or user taps "Set up with my phone"
      ▼                   ▼
   ┌────────────────────────────────┐
   │ SETUP_AP  SoftAP + captive     │
   │ portal + join QR on the screen │
   └────────────────────────────────┘
```

Decisions inside it:

- **Test before persisting.** Credentials are written to NVS only after a connection actually
  succeeds. pangcrd saves first and can leave a unit holding a password that never worked.
- **Report the failure inline, do not reboot.** Heftie reboots and lets a 20 s timeout re-enter
  setup; on a gift that is 30 silent seconds that look like a crash. Distinguish
  `WIFI_REASON_AUTH_FAIL` ("Wrong password") from not-found ("Network out of range") and show it
  under the password field, with the keyboard still up.
- **A real Wi-Fi join QR** — `WIFI:S:<ssid>;T:WPA;P:<pass>;;` rendered with `lv_qrcode` on the
  SETUP_AP screen. The recipient points a phone camera at the screen, joins the setup network, and
  the captive portal opens by itself. Neither reference does this; pangcrd's QR is a link to a web
  page *after* the device is already online. This is the single nicest thing in the whole flow and
  it costs one widget.
- **Forget network** in Settings, with Heftie's double-tap-to-arm so a stray touch cannot wipe it.
- **Scan off the UI task**, results handed back under the LVGL mutex. The keyboard sits in its own
  flex row beneath the fields (Heftie), never floating at hardcoded coordinates (pangcrd).

**Location is set the same way, and reuses the same keyboard.** Open-Meteo's geocoding API is
keyless: the recipient types a town name, taps a result, and the coordinates go to NVS. No IP
geolocation service, no account. Timezone comes back from Open-Meteo with `timezone=auto`; the
clock is NTP.

## 8. The interface

**Portrait: 240 wide × 320 tall**, long side vertical (operator direction, 2026-09-20). Designed
for *pressed taps on large targets*, because §4.2 says the touch is resistive.

Portrait is the better orientation for this product and not only a preference. The Now screen is a
vertical hierarchy — one verdict, then its supporting figures, then climate — and portrait lets
that stack read top to bottom at full width instead of competing for a short 240 px column. It
also puts the tab bar at the bottom of a tall object, where a thumb naturally lands, and it makes
the device a narrow upright thing on a shelf rather than a wide one. Practical consequences:

- LVGL is configured `hor_res = 240`, `ver_res = 320`; the panel is driven at its native rotation
  rather than rotated in software, which avoids a rotation-dependent offset bug of exactly the
  kind §3 warns about.
- **Touch must be calibrated in the same orientation as the display, and the XPT2046's axes do
  not follow the panel rotation automatically** — mismatched or swapped touch axes is the single
  most common CYD bring-up complaint, and it is a portrait-specific trap. AU-01 verifies rotation
  and touch mapping together, not separately.
- nicholaswilde's reference UI already builds both portrait and landscape variants with
  per-orientation label strings, so the responsive pattern is there to copy.

**Navigation:** a bottom tab bar — **Now · Trend · Outside · Settings**. Tabs are a big, forgiving
target; swipe works where touch allows but is never the only way to reach anything.

**Now** is the screen the device shows 99 % of the time and it must read from across a room:

```text
        240 px wide
┌──────────────────────────┐ ─┐
│ Living room   21:04  ((•))│  │  header: room, clock, link
├──────────────────────────┤  │
│                          │  │
│      ╭────────────╮      │  │
│     ╱              ╲     │  │
│    │     Stuffy     │    │  │  the verdict, colour-banded arc,
│     ╲              ╱     │  │  readable across a room
│      ╰────────────╯      │  │
│       Air quality        │  │
│                          │  │
│   Open a window          │  │  the action, in plain words
│                          │  │  320 px tall
├──────────────────────────┤  │
│  eCO₂     ~1240  est.    │  │  supporting figures, small type,
│  TVOC       340  ppb     │  │  never the headline
├──────────────────────────┤  │
│  21.4 °C        47 % RH  │  │  climate, from the BME280
├──────────────────────────┤  │
│ Now │ Trend │ Out │ Set  │  │  tab bar, thumb height
└──────────────────────────┘ ─┘
```

- **The word is the headline, the numbers are the supporting detail.** The brief is useful
  information, not laboratory accuracy, so the arc carries a band word — *Clean · Normal · Stuffy ·
  Poor* — driven by the ENS160's air-quality index and `eCO₂`. A gift should be readable by someone
  who has never heard of a VOC, and a band is the honest presentation of a self-baselining
  metal-oxide sensor. The figures sit beside it in small type for anyone who wants them.
- **The ventilation verdict is in, and `eCO₂` drives it** — *"open a window"* is exactly what
  ScioSense built this output for (§5). What the UI does not do is print a bare `CO₂` label: the
  detail row reads **`eCO₂ ~1240 est.`**, and a Settings → About line says it is estimated from
  VOC. The value is surfaced; the unearned word is not. That keeps `inventory.md:415` intact
  ("only the SCD41 may be labelled CO₂") while giving the recipient the prompt that makes the
  object useful.
- **Approximate by design, and it should look approximate.** The `~` and the `est.` are not
  hedging for its own sake — they stop a recipient from comparing 1240 against a number they
  googled, when a squirt of window cleaner can move this reading by a thousand.
- **Temperature and humidity come from the BME280, never the AHT20** (§5). The AHT20's numbers are
  the ENS160's compensation inputs and are not displayed.
- **The RGB LED is the ambient layer** — a slow, gamma-corrected glow in the current air-quality
  colour, so the room state is legible without looking at the screen at all. This is the detail
  that makes the object feel designed rather than assembled, and the board already has the LED.
- **Trend:** a themed `lv_chart` of the last 6–24 h with horizontal band lines at the thresholds.
- **Outside:** Open-Meteo — conditions, a short forecast, and outdoor PM2.5 / European AQI, which
  is what stands in for the PM sensor that G3 excluded. *Indoor measured vs outdoor fetched is a
  clear, honest split, and the UI must label which is which.*
- **Settings:** Wi-Fi, location, units (°C/°F), theme, brightness, night dimming, Home Assistant.

**Look:** a palette *token table* read at draw time — `base / surface / text / muted` plus one
accent per quantity (air quality, temperature, humidity, outdoor) — flavour switchable from Settings,
exactly as nicholaswilde does. Icons compiled as an **LVGL bitmap font** rather than images, so one
glyph recolours per air-quality band and the whole icon set costs kilobytes instead of megabytes.
Backlight fades rather than steps, dims on idle, and follows the onboard LDR at night.

**States are part of the design, not an afterthought (G5):**

| State | What is drawn |
| --- | --- |
| `WARMING_UP` | `"--"` in the number's place, "Warming up…" banner. Never a zero |
| `OK` | The value |
| `STALE` | Last value dimmed, with its age ("2 min ago") and an amber banner |
| `FAULT` | `"--"`, a red banner naming the sensor, and the rest of the screen still working |

This is the failure HenrysCat's sketch has — a dead sensor leaves stale numbers on screen forever
with no indication — and the four states above are host-tested in `reading_state`, so the
behaviour is verified without a bench.

## 9. Open decisions for the operator

These belong in `agent-context/INBOX.md` as decisions before any part is bought or any story opened.

1. ~~**Product name.**~~ **Answered 2026-09-20 → D-037: AtmosMesh Aura.** The working title was
   `Gift`; the operator vetoed it as naming the occasion rather than the object. See D-037.
2. ~~**Sensor tier.**~~ **Answered 2026-09-20 → D-036: ENS160 + AHT20 for gas, BME280 for
   climate, SHT41 kept as the fleet spare, no CO₂ sensor.** See §5. The MH-Z19C upgrade path stays
   open and GPIO35 is reserved for it.
3. **Which CYD.** Use an on-hand `R` (resistive, stylus) unit, or buy a `C` (capacitive) unit for
   the gift and keep the `R` pair for bench work. *Recommended: buy a `C` — see §4.2.*
4. **Graphics stack.** LVGL 9 + LovyanGFX with runtime panel detection, or LVGL 8 + TFT_eSPI to
   stay closer to nicholaswilde's copyable UI code. *Recommended: LVGL 9 + LovyanGFX — see §6.*
5. **Does MQTT ship enabled?** *Recommended: no — off by default, enabled in Settings.*
6. **Enclosure.** Not cosmetic: §4.1 makes sensor placement a measurement decision. A printed case
   that holds the sensors on the pigtail away from the board, with its own vents, or no case.
7. **OTA.** A dual-OTA partition layout on 4 MB must be chosen before the first flash. Worth it for
   a device that leaves the house; costs the option of a single large app partition.

## 10. Proposed story sequence

Nothing starts until §9 is answered. When it is, and per the roadmap's rule that optional work does
not displace a ready MVP story:

| Story | Outcome | Gate |
| --- | --- | --- |
| AU-01 | CYD unit identified from its own silicon: display controller, touch controller, board suffix, RGB LED and LDR exercised | A self-test image reports every device over serial and on screen |
| AU-02 | CN1 I²C bus proven with one sensor, pull-ups measured, 3.3 V rail watched under load | Meter (or scope) evidence on the rail; `inventory.md` updated with measured facts, replacing the community pin map with probed ones |
| AU-03 | Host-tested domain: `air_band`, `reading_state`, `wifi_credentials`, `provisioning`, `aura_view_model` | `task test` green, tests written first |
| AU-04 | LVGL 9 + LovyanGFX bring-up with runtime panel detection; the Now screen renders from fake readings | Both on-hand units render correctly from one image |
| AU-05 | Provisioning end to end: wizard, keyboard, test-before-save, inline failure, SetupAP + join QR, forget | A factory-reset unit joins a network with no serial cable touched |
| AU-06 | ENS160+AHT20 and BME280 live, four reading states visible on demand, `eCO2` shown as an estimate | Sensor unplugged mid-run shows FAULT not a frozen number; a host test proves every `eCO2` path carries the estimated label and none emits a bare `co2` name or entity id |
| AU-07 | Open-Meteo: geocoding search, forecast, outdoor AQI, NTP clock | Works with no account and no key |
| AU-08 | Optional MQTT + Home Assistant discovery, off by default | Existing D-007 contract unchanged |
| AU-09 | Enclosure, self-heating measured against a reference thermometer, offset documented or designed out | §4.1 closed with numbers |
| AU-10 | 48-hour unattended run, then hand it over | Power-loss, sensor-loss and network-loss behaviour all observed |

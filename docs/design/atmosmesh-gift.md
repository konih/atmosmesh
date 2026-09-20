# AtmosMesh Gift — design and architecture proposal

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
| G5 | It never lies about a number | D-002 applies unchanged: only a real NDIR-class part may be labelled CO₂; a stale reading is never a zero |
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
so the sensor choice in §5 is partly a power decision. **With the recommended set — SGP41 and
SHT41 — this stops being a problem:** both are single-digit-milliamp parts and the pigtail carries
a load the rail will not notice. Confirm the SGP41's heater peak from its own datasheet before
wiring (the repo does not take heater currents from memory), but nothing in that set approaches the
rail's limit.

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

- **Display controller unknown.** `2432S028` ships with ILI9341 *or* ST7789, differing in colour
  inversion and RGB/BGR order. The community heuristic is that **dual-USB boards (Micro-B + Type-C)
  are the ST7789 "CYD2USB" variant with inverted colours** — and the inventory records the operator's
  units as having *both* connectors (`inventory.md:567`). **Working hypothesis: these are ST7789,
  colour-inverted.** Unverified. §6 makes this a runtime concern rather than a rebuild.
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

The SCD41 is the obvious sensor for this and it is also the expensive one (~€30–45). It is not
required. **The recommended build below costs nothing — both parts are already free in the
drawer** (`inventory.md` "Unreserved and looking for a project": *SGP40 and SGP41 … 1 SHT41 …
the second ENS160+AHT20*).

### Recommended build — €0, everything already in stock

| Part | Measures | I²C addr | Stock | Why this one |
| --- | --- | --- | --- | --- |
| **Sensirion SGP41** | **VOC Index and NOx Index**, 1–500 | `0x59` | 1 free | The headline gas sensor, and a genuinely good one. Sensirion's gas-index algorithm **self-baselines to the room**, so it needs no calibration, no user action and no warm-up ritual — it just reports how the air compares to this room's own normal. Catches cooking, solvents, cleaning products, a full bin, a stuffy unaired room. Honest by construction: an index, not a fabricated ppm |
| **Sensirion SHT41** | T, RH | `0x44` | 1 free | Accurate (±1.8 % RH), sub-µA, and — critically for §4.1 — small enough to sit at the far end of the pigtail away from the board's heat |

Two parts, two wires, one bus, no address clash, no fan, no heater the rail will notice, and
nothing needing a pin this board does not have. Cost: a 4-pin pigtail and two pull-up resistors.

### The trade-off this makes, stated plainly

**A VOC index is not a substitute for CO₂, and it is important not to pretend otherwise.** They
answer different questions:

- **CO₂** answers *"have too many people been breathing in here too long?"* — it is the number that
  justifies the instruction **open a window**.
- **VOC** answers *"has something been released into the air?"* — cooking, spray, solvent, damp.

A closed bedroom overnight can reach 1500 ppm CO₂ while the VOC index sits at a contented 100.
Dropping the SCD41 means the gift **loses the ventilation prompt** and becomes an
air-*cleanliness* monitor rather than an air-*freshness* one. That is a perfectly good gift — it
is what most commercial sub-€50 "air quality" gadgets actually are — but the UI must then stop
promising what it cannot measure, so the verdict line in §8 changes from *"Open a window"* to
statements about what the VOC index genuinely supports.

### Also free in stock, as an alternative or an addition

| Part | Measures | I²C addr | Verdict |
| --- | --- | --- | --- |
| **ENS160 + AHT20** combo | TVOC, an air-quality index, (eCO₂), plus T/RH from the AHT20 | `0x52`/`0x53` + `0x38` | 1 free. **One module, one cable, gas *and* climate** — the cheapest possible path if the SGP41 or SHT41 get claimed elsewhere. Two caveats: the AHT20 is a clear step down from the SHT41 on accuracy, and **the ENS160's "eCO₂" is derived from VOC, not measured — under D-002 it must never be drawn, labelled or published as CO₂.** Use its TVOC and index, ignore the eCO₂ register entirely |
| **SGP40** | VOC Index only (no NOx) | `0x59` | 1 free. The SGP41's predecessor and a drop-in fallback if the SGP41 is wanted elsewhere. Same address, so the two cannot share a bus |
| **BME280** | T, RH, pressure | `0x76` | 6 free. Substitute for the SHT41 if that gets claimed; ±3 % RH instead of ±1.8 %, and adds a pressure reading a gift recipient will not use |

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

*Recommendation: ship the €0 SGP41 + SHT41 build, and treat the MH-Z19C as a later addition if the
ventilation prompt turns out to be missed.*

### Deliberately excluded, with reasons

| Part | Why not |
| --- | --- |
| **Any fan-based PM sensor** (SDS011, SPS30, PMS5003) | Audible, 5 V at 70–200 mA, needs a duct, and the SDS011 needs a UART this board does not have. Fails G3 outright. **Instead: show outdoor PM2.5/PM10 and European AQI from Open-Meteo's free air-quality API** — no hardware, no noise, no cost, and it is the number that actually varies day to day |
| **MQ135** | Not a CO₂ sensor (D-002), 5 V heater at ~190 mA the CYD's LDO cannot serve, and it needs an ADC pin. Every reason to reject it applies at once. It is also the *cheap gas sensor* this project has already decided against, on purpose — the SGP41 is the honest version of the same idea |
| **MQ-7, MQ-3** | Same family, same objections, and neither measures anything a home gift should claim |
| **VEML7700** | Redundant — the board has an LDR on GPIO34, which is enough to auto-dim a backlight |

### If particulates are non-negotiable

Buy a **Sensirion SEN55** — PM1/2.5/4/10 plus VOC, NOx, T and RH in one I²C part, replacing both
recommended sensors. It is *more* expensive than the SCD41 this section just dropped, so it only
makes sense if particulates are the point. Two caveats change the build: it **needs 5 V** (CN1 supplies only
3.3 V, so it must be tapped off the USB input separately), and it **has a fan**, so G3 is being
traded away deliberately rather than by accident.

## 6. Firmware architecture

AtmosMesh Gift becomes a fourth product under ADR-0001's existing four-layer scheme — one
PlatformIO project, one explicit composition root, shared host-tested domain code.

```text
firmware/
  include/atmosmesh/
    gift_pins.hpp            CYD pin map + CN1 I2C assignment, one place
    gift_profile.hpp         ProductProfile entry: atmosmesh-gift-v1
    air_band.hpp             VOC / NOx index -> named band + verdict    [host-tested]
    reading_state.hpp        WARMING_UP | OK | STALE | FAULT            [host-tested]
    wifi_credentials.hpp     SSID/PSK validation, NVS record shape      [host-tested]
    provisioning.hpp         the provisioning state machine             [host-tested]
    gift_view_model.hpp      every string the UI draws, incl. "--"      [host-tested]
    gift_openmeteo.hpp       JSON -> outdoor reading struct             [host-tested]
    gift_mqtt_contract.hpp   GiftMqttState, extends the D-007 contract  [host-tested]
  src/
    air_band.cpp  reading_state.cpp  wifi_credentials.cpp
    provisioning.cpp  gift_view_model.cpp  gift_openmeteo.cpp     <- native env
    gift_display.cpp      LovyanGFX panel + LVGL bind      \
    gift_touch.cpp        XPT2046, bit-banged               |
    gift_sensors.cpp      SGP41 + SHT41 on one I2C bus     |  device only,
    gift_nvs_store.cpp    Preferences/NVS                   |  excluded from
    gift_net.cpp          Wi-Fi + NTP + Open-Meteo client   |  the native env
    gift_ui_*.cpp         one file per screen              /
    products/atmosmesh_gift_v1.cpp   the composition root
```

**The rule that makes this testable:** anything that decides *what* to show is a pure function in
the native env with Unity tests written first (AGENTS.md workflow step 3, GUIDELINES §A3). Anything
that touches a pin or a socket is a thin device-only file with no logic in it. `gift_view_model`
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
that, when filled in, brings up the existing `mqtt_session` machinery with a `GiftMqttState` built
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

240×320 panel, **landscape 320×240** on a stand. Designed for *pressed taps on large targets*,
because §4.2 says the touch is resistive.

**Navigation:** a bottom tab bar — **Now · Trend · Outside · Settings**. Tabs are a big, forgiving
target; swipe works where touch allows but is never the only way to reach anything.

**Now** is the screen the device shows 99 % of the time and it must read from across a room:

```text
┌────────────────────────────────────────────┐
│  Living room            ⌂ 21:04    [ wifi ]│
│                                            │
│        ╭──────────╮                        │
│        │    34    │      21.4 °C           │
│        │   VOC    │      47 % RH           │
│        ╰──────────╯                        │
│        Air · Clean       NOx  1 · none     │
│                                            │
│      Nothing unusual in the air            │
├────────────────────────────────────────────┤
│   Now  │  Trend  │  Outside  │  Settings   │
└────────────────────────────────────────────┘
```

- One dominant number in a colour-banded arc, one plain-language verdict underneath
  (*Clean · Normal · Something in the air · Ventilate*). A gift should be readable by someone who
  has never heard of a VOC index — the index itself is secondary to the word next to it.
- **The wording must not overclaim (G5).** With no CO₂ sensor fitted, the verdict describes what
  the VOC index actually supports — *"nothing unusual in the air"*, *"cooking or cleaning
  detected"* — and never *"stuffy"* or *"open a window"*, which are ventilation claims only a CO₂
  measurement earns. If the MH-Z19C option of §5 is ever added, the ventilation verdict comes with
  it and this line changes.
- **The RGB LED is the ambient layer** — a slow, gamma-corrected glow in the current air-quality
  colour, so the room state is legible without looking at the screen at all. This is the detail
  that makes the object feel designed rather than assembled, and the board already has the LED.
- **Trend:** a themed `lv_chart` of the last 6–24 h with horizontal band lines at the thresholds.
- **Outside:** Open-Meteo — conditions, a short forecast, and outdoor PM2.5 / European AQI, which
  is what stands in for the PM sensor that G3 excluded. *Indoor measured vs outdoor fetched is a
  clear, honest split, and the UI must label which is which.*
- **Settings:** Wi-Fi, location, units (°C/°F), theme, brightness, night dimming, Home Assistant.

**Look:** a palette *token table* read at draw time — `base / surface / text / muted` plus one
accent per quantity (VOC, NOx, temperature, humidity) — with the flavour switchable from Settings,
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

1. **Product name.** `Gift` fits the existing one-word-noun family (Room, Spot, Aqua, Grove) and
   states the intent. Alternatives: `Glass`, `Desk`, `Cube`. *Recommended: Gift.*
2. **Sensor tier.** (A) **SGP41 + SHT41 — €0, both already in stock**, VOC/NOx + accurate
   climate, no ventilation prompt. (B) ENS160+AHT20 alone — also €0, one module and one cable,
   weaker climate accuracy, eCO₂ register must stay unused. (C) A + **MH-Z19C** (~€15–25) to buy
   back the CO₂ ventilation prompt, at the cost of a 5 V tap and a second wire. (D) SEN55 alone,
   particulates included, fan noise accepted — the most expensive option. *Recommended: A, with C
   as the upgrade if the ventilation prompt is missed.*
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
| GF-01 | CYD unit identified from its own silicon: display controller, touch controller, board suffix, RGB LED and LDR exercised | A self-test image reports every device over serial and on screen |
| GF-02 | CN1 I²C bus proven with one sensor, pull-ups measured, 3.3 V rail watched under load | Meter (or scope) evidence on the rail; `inventory.md` updated with measured facts, replacing the community pin map with probed ones |
| GF-03 | Host-tested domain: `air_band`, `reading_state`, `wifi_credentials`, `provisioning`, `gift_view_model` | `task test` green, tests written first |
| GF-04 | LVGL 9 + LovyanGFX bring-up with runtime panel detection; the Now screen renders from fake readings | Both on-hand units render correctly from one image |
| GF-05 | Provisioning end to end: wizard, keyboard, test-before-save, inline failure, SetupAP + join QR, forget | A factory-reset unit joins a network with no serial cable touched |
| GF-06 | SGP41 and SHT41 live, with the four reading states visible on demand | Sensor unplugged mid-run shows FAULT, not a frozen number |
| GF-07 | Open-Meteo: geocoding search, forecast, outdoor AQI, NTP clock | Works with no account and no key |
| GF-08 | Optional MQTT + Home Assistant discovery, off by default | Existing D-007 contract unchanged |
| GF-09 | Enclosure, self-heating measured against a reference thermometer, offset documented or designed out | §4.1 closed with numbers |
| GF-10 | 48-hour unattended run, then hand it over | Power-loss, sensor-loss and network-loss behaviour all observed |

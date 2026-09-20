# Adversarial review — AtmosMesh Aura design

- **Date:** 2026-09-20
- **Target:** `docs/design/atmosmesh-aura.md`, D-036, D-037, `agent-context/INBOX.md`
- **Method:** four hostile personas (Saboteur, Budget Holder, Security Auditor, New Hire), each run
  on a fresh model with no memory of the design discussion. Load-bearing claims were re-verified
  against the repository by the coordinator; those are marked *(verified)*.

## Verdict: BLOCK

Four CRITICAL findings, all four verified against the repository rather than merely argued. The
design's shape is not wrong, but it cannot be implemented as written.

---

## CRITICAL

### C1 — The MQTT key `eco2_estimated` is rejected by the repo's own guard *(verified)*

**Found independently by two personas.** `firmware/src/mqtt_contract.cpp:438-442` defines
`mqtt_payload_mentions_forbidden_gas_label()`, returning true for **any** payload containing
`co2`, `CO2` or `ppm`. `firmware/test/test_mqtt/test_mqtt_contract.cpp:13,470` asserts every
product's state JSON, discovery payload and topics are clean of it.

`eco2_estimated` contains the substring `co2`. The Home Assistant discovery config for that entity
needs `unit_of_measurement: ppm`. **Both are forbidden by a test that passes today for every
product.** D-036 claims consistency with D-002 and does not have it: the first AU-08 contract test
fails, and the maintainer must either weaken a fleet-wide guard that exists to protect the MQ135
rule, or special-case Aura. The design decides neither. D-036's "host test" is also unwritable as
specified — it names no function, no output and no forbidden-string set, while the repo's natural
oracle contradicts the design's own key.

**Fix:** choose explicitly. (a) Publish no gas-estimate entity over MQTT at all; (b) rename to a
key with no forbidden substring (`voc_index`, `air_quality_estimate`) and keep the eCO₂ figure on
the local screen only; or (c) amend D-002 and convert the guard to an allow-list that still forbids
a bare `co2`. **(b) is the smallest change and leaves D-002 untouched.**

### C2 — The gift is handed over carrying the operator's secrets; no factory reset exists *(verified)*

§10 `AU-10` is "48-hour unattended run, **then hand it over**". That run uses the operator's Wi-Fi
and, if `AU-08` is exercised, the operator's broker credentials — which `secrets.hpp.example:8-10`
documents as the **fleet-wide kumulus Mosquitto `homeassistant` password from sops**, not a
throwaway. All of it lands in the same NVS namespace the recipient will use.

Verified: `Taskfile.yml` has **no erase task**, and no story defines factory reset — although
`AU-05` presupposes "a factory-reset unit". §7's "Forget network" is scoped by name to
`wifi_ssid`/`wifi_pass` and says nothing about `mqtt_*`, location or trend data. NVS is
unencrypted, so anyone who later holds the device recovers all of it with `esptool read_flash` over
the same USB port the gift ships with.

**Fix:** a Settings → "Factory reset (erase everything)"; a `task erase-aura`; erase-then-
recipient-provisions as a **gate on AU-10**; and never exercise AU-08 against the fleet
`homeassistant` account — use a throwaway broker user with an ACL limited to this device's topics.

### C3 — ESPHome was never evaluated, and D-019 does not transfer *(verified)*

`grep -i esphome docs/design/atmosmesh-aura.md` returns nothing. §2 surveys nine hobby repos and
omits the platform that ships most of the ten-story plan as YAML. Verified to exist upstream:
`lvgl` (arc, tabview, keyboard, qrcode, textarea), `xpt2046` touchscreen, `ens160`, `aht10` (AHT20
variant), `bme280_i2c`, `ili9xxx`, captive-portal + AP-fallback provisioning, OTA, and Home
Assistant integration needing no broker at all.

D-019 / ADR-0002 did consider "use ESPHome YAML directly" (option b′) and rejected it — but for the
**Aqua ESP8266 variant**, because it "forks the fleet's tooling into two build systems with no
shared host-tested core". By §6's own account Aura shares almost none of that core: no
`secrets.hpp`, a new NVS store, a new provisioning machine, a UI stack no other product uses, and
the only genuinely shared code (`mqtt_session`/`mqtt_contract`) serves a feature that ships
**disabled**.

Where ESPHome genuinely falls short: no host-Unity tests for lambdas (AGENTS.md step 3), no
`lv_chart` for the Trend tab, compile-time panel selection. Roughly one story of shortfall against
perhaps seven of rebuild.

**Fix:** the design owes an explicit ESPHome row in §2 and a stated, priced reason the bespoke
build wins. "UI polish" may well be that reason — but it must be named and defended, not implied.

### C4 — The design cites facts its own branch does not contain *(verified; root cause found)*

§3 cites `inventory.md:594` for "ESP32-D0WD-V3 rev 3.1, 4 MB flash, CH340". In this branch line 594
reads *"The seller's article description adds nothing and is not evidence."*, and `grep D0WD-V3`
over `inventory.md` returns **nothing**.

Root cause: this worktree was branched from `origin/main` (`c2019e2`), one commit behind the
operator's local `main`. The missing commit is `1306f19 ":memo: docs(hardware): record the first
CYD's USB probe"` — exactly the commit holding those facts. The design was written from the main
checkout before the worktree existed, so it cites evidence that is real but absent here.

Consequence: the "4 MB flash" figure feeding the dual-OTA decision (§3, §9.7) is unverifiable in
this branch, and the new CYD sections were appended to an `inventory.md` missing the section they
build on — a merge will interleave them confusingly.

**Fix:** rebase this branch onto local `main` (or push `1306f19`) before further work, then
re-verify every `inventory.md:NNN` citation. Replace bare line numbers with heading anchors;
`inventory.md` is append-heavy and numeric citations rot within weeks.

---

## WARNING

**Resource budgets are asserted, never computed** *(two personas)* — verified: no
`board_build.partitions` anywhere in `firmware/platformio.ini`, so every ESP32 env inherits
`default.csv` (2 × 1.28 MB app). A realistic Aura image — Arduino + WiFi + mbedTLS + HTTPClient
≈ 1.1–1.2 MB, LVGL 9 with tabview/keyboard/chart/arc/qrcode ≈ +250–400 KB, LovyanGFX ≈ +100 KB,
fonts ≈ +100–200 KB — is **1.6–1.9 MB, over the slot**. §3's "comfortably under 2 MB … but only
just" contradicts itself. No RAM budget exists at all: ~290 KB free at boot, less WiFi (−45 KB),
LVGL pool and draw buffers (≈100 KB), one mbedTLS handshake (≈45–50 KB transient), then an
Open-Meteo JSON body. Three Open-Meteo hostnames means three handshakes. If `lv_malloc` uses the
system heap, days of UI churn fragment it until no contiguous 16 KB remains and the Outside tab
goes permanently STALE — invisible to a 48 h soak. Needs: static LVGL pool and draw buffers,
`forecast_days=1–2`, streaming/filtered JSON parse, one reused TLS client, a measured `.bin` before
§9.7 is decided, and pinned `platform`/LVGL/LovyanGFX versions (ESP32 envs are unpinned today, so a
silent core bump adds ~150 KB).

**Provisioning strands a working device** *(two personas)* — `RETRYING` has no outgoing edge and
`SETUP_AP` no exit but successful re-provisioning. A router rebooting at 03:00 burns three strikes
in two minutes and parks the device in SETUP_AP until a human re-types the password. Only
`AUTH_FAIL` should count as a strike — and on ESP32 a wrong PSK usually reports as
`4WAY_HANDSHAKE_TIMEOUT` (15) or `HANDSHAKE_TIMEOUT` (204), not `AUTH_FAIL` (202), so §7's inline
"Wrong password" fires on the wrong code in both directions. Enumerate and host-test the whole
reason-code → message table.

**SoftAP security unspecified** — where the AP's PSK comes from is not stated; a compile-time
constant in this public MIT repo is a public password, and WPA2-PSK has no client isolation, so a
listener holding it decrypts the recipient's home password as it is POSTed. Portal POST
authentication, teardown on `ONLINE`, and bounds-checking the POST body before `aura_nvs_store` are
all unaddressed. Use a per-device random PSK generated at first boot. Separately, ESP32 AP+STA
share one channel, so test-before-save drops the phone mid-test — the result must appear on the
device screen.

**The "open a window" thresholds do not exist** — §5, §6 and §8 name three different input sets
(eCO₂; TVOC/AQI; "index and eCO₂") and no numeric bands appear in the design, decisions or
inventory. ENS160's AQI-UBA is 1–5; the design has four named bands with no mapping. AU-03 says
"tests written first" — against what?

**No story, no roadmap entry, yet `inventory.md` already routes work to AU-01** — no `AU-*.md`
exists, `roadmap.md` has no Aura section, and the INBOX answer field is still the placeholder.
AGENTS.md requires a claimed story before implementation. §10 also claims compliance with the
roadmap's "optional work does not displace a ready MVP story" rule while `RLS-01` is still
`Ready, P0`. (AGENTS.md:75 names RLS-09/RLS-10 specifically, so the violation is narrower than the
persona claimed — but claiming compliance was sloppy, and the operator should re-baseline the
roadmap explicitly rather than let a design paper over it.)

**Glyph coverage** *(two personas)* — `eCO₂` uses U+2082 and "Warming up…" uses U+2026; LVGL's
built-in Montserrat fonts are ASCII plus a few symbols, so the host-tested string renders as `eCO▯`
on the device. Worse for provisioning: German/Polish SSIDs (`Müllers WLAN`) render as boxes in the
scan list and cannot be typed on an ASCII-only `lv_keyboard`. Needs fonts built with 0xA0–0xFF and
a Latin-1 keyboard map.

**Wizard cannot onboard common networks** — no manual-SSID path (hidden SSIDs), no "2.4 GHz only"
message anywhere a recipient can read it (ESP32 has no 5 GHz radio), and a 40-network list needs
scrolling, which §4.2 rules out as a primary resistive-touch interaction.

**LDR polling glitches the touch IRQ** — ESP32 errata 3.11: enabling SAR ADC1 pulls GPIO36/39 low
for ~80 ns. The LDR is GPIO34 (ADC1); touch IRQ is GPIO36 and touch MISO GPIO39. Night dimming
therefore injects phantom touch interrupts exactly when the screen dims. Poll XPT2046 by pressure
rather than edge-triggering, and never sample the LDR inside a touch read window.

**Fixed station id** — D-037 hardcodes `atmosmesh-aura-0001`. Two units exist; if both ever meet
one broker their last-will messages mark each other offline, the bug `platformio.ini` already
records for Room. Needs a MAC-derived default id.

**Further concrete items** — a torn NVS record (two keys for one credential; power loss between
writes yields an SSID with an empty password, then "wrong password" the user never mistyped); the
shared sensor snapshot has no stated protection while four contexts write to it (LVGL task, net
task, esp-mqtt task, Wi-Fi event loop), so the UI can read `state=OK` with `value=0.0`; ENS160
warm-up must come from the chip's `DATA_STATUS` validity flag, not a 3-minute timer, and a rail dip
resetting only the sensor leaves stale numbers forever — the exact HenrysCat failure G5 exists to
prevent; `i2c_bus.cpp` has no 9-clock bus recovery; Open-Meteo needs backoff with jitter or a
shared NAT gets rate-limited for the day; the trend chart has no ring size, sample period,
persistence or `LV_CHART_POINT_NONE` handling, so `--` periods plot as zero; the clock shows 1970
until NTP and drifts an hour at DST because `utc_offset_seconds` is a snapshot, not a rule;
`millis()` comparisons must use unsigned subtraction or a 49.7-day wrap flips every reading; MQTT
carries the recipient's broker password in cleartext on their LAN (D-007, port 1883, no TLS) and
the design never restates that for a foreign network; OTA has no delivery path to a device in
someone else's house and no signing story, so an unsigned pull on a hijacked network is remote code
execution; existing runtimes log `ssid=%s` and `user=%s` to serial and committed story evidence
already contains a real LAN address (`stories/RLS-05.md:56`), so Aura's evidence needs a throwaway
SSID and a fictitious town; every new device-only `.cpp` compiles into `task test` by default until
added to `platformio.ini`'s native exclusion list, which the design never mentions; Aura needs a
seventh `*_mqtt_runtime.cpp` because the only ESP32 transport hard-includes `secrets.hpp`; and the
repo now has three product-identity conventions (`product_profile.hpp`, hardcoded in
`mqtt_contract.cpp`, and the proposed `aura_profile.hpp`) with nothing saying which to copy.

**Scope and sequencing** — §8 states the Now screen shows "99 % of the time"; everything else
(Trend, Outside, geocoding, themes, units, night dimming, QR, captive portal, MQTT panel, OTA) is
the remaining 1 %. AU-08 in particular needs broker credentials typed on a resistive keyboard and
presumes a broker — a bench feature for the unit the operator keeps, costing one of ten stories.
The plan is sequenced to build rather than to learn: the riskiest assumptions (resistive-touch
typing is tolerable; self-heating is manageable) are first tested at stories 5 and 9.

---

## NOTE

G1–G6 carry no attribution — G3 ("it is silent") is an author-invented rule that then generates
story AU-07 to compensate for it, and G6 generates a second provisioning path alongside the one G1
already satisfies. Ask the operator which of the six are theirs. §9 lists the graphics stack and
the MQTT default as open while §6 states both as settled. §9 and the INBOX disagree on the option
set — the enclosure, which §4.1 calls a *measurement* decision, is in §9 but was never put to the
operator. §2's "nothing from these repos gets imported" rewrites a maintained MIT library
(WiFiManager) as policy. And "we already own two CYDs" stops being an argument the moment §9.3
recommends buying a capacitive unit — at which point ESP32-S3 display boards in the same bracket,
with PSRAM, free GPIOs and capacitive touch, would delete the pin famine, the LDO worry, the
controller lottery and the touch objection at once.

---

## What was not checked

No firmware exists, so nothing was executed against hardware. Flash and RAM figures are estimates
from component sizes, **not** a measured `.bin` — closing that is AU-04's job and it must happen
before §9.7. ESPHome's LVGL component was verified to exist with the needed widgets, but no Aura
YAML was written or compiled, so "one story of shortfall" is an argument, not a measurement. The
ESP32 reason-code behaviour, errata 3.11 and the ENS160 validity-flag behaviour come from
documentation, not from this bench. The host suite was run and passes (182 cases) but covers none
of the proposed Aura modules.

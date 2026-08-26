# ADR-0002: MQTT remains AtmosMesh's sole transport; ESPHome native API is declined for now

- **Status:** Accepted
- **Date:** 2026-08-26
- **Owners:** AtmosMesh maintainers

## Context

- **D-006** set the toolchain and explicitly parked this exact idea: "ESPHome remains a later
  option for station YAML if we want it; **we do not maintain two stacks now**."
- **OQ-001** recorded the precise trigger for revisiting that: "Revisit if: ESPHome would
  materially simplify the full station (MQTT, HA discovery) after the bench sensors work."
- **D-007/D-013** already deliver a working, host-tested, ID-based MQTT contract with Home
  Assistant MQTT discovery, retained availability/LWT and reconnect-safe discovery replay, for
  both AtmosMesh v1 and Grove v1.5, against an already-running broker.
- **ADR-0001** already permits product-specific transports to differ at a small compile-time
  composition boundary, and is explicit that product profiles are compile-time declarations, not
  runtime selection branches.
- The operator asked for ESPHome's native API (protobuf-over-TCP on port 6053, optionally
  Noise/ChaCha20-Poly1305-encrypted — the protocol `aioesphomeapi`/Home Assistant's `esphome`
  integration speaks directly) as a second, switchable transport next to MQTT.

This ADR is OQ-001's revisit trigger firing. It answers the trigger's question directly: does
ESPHome materially simplify the station now? The answer below is no, and D-006's "we do not
maintain two stacks now" stays in force. OQ-001 remains resolved, not reopened.

### What was checked (not assumed)

- PlatformIO registry search for "esphome api" returned no maintained, general-purpose
  device-side (server) ESPHome-native-API library.
- One real candidate surfaced: `knkaiser/BentuinoESPHomeAPI` — a from-scratch, PlatformIO-
  buildable, Arduino C++ implementation of the device/server side of the protocol for
  ESP32/ESP32-C3/ESP8266, including a self-contained Noise handshake (vendored TweetNaCl X25519 +
  in-tree SHA-256/HKDF + ChaCha20-Poly1305). Three facts disqualify it for now:
  - **License: AGPL-3.0**, against this project's MIT posture — a real conflict, not a nitpick.
  - **Maturity:** created 2026-06-09, last touched 2026-06-19, 0 stars, single author, `0.4.1`,
    self-reported open items (no TLS/alt transport, soak-untested heap use, partial persistence).
    Vendored, unaudited, from-scratch crypto is a materially higher risk profile than an
    established, audited crypto library.
  - **ESP8266 footprint:** the library's own minimal demo reports RAM 38.4% / flash 29.3% with
    Noise enabled — before any AtmosMesh sensors, OLED, or the existing MQTT stack are added.
    Grove v1.5's entire current firmware already runs at RAM 42.2% / flash 29.3%. Two resident
    stacks do not fit on one ESP8266.
- **Protocol stability:** `esphome/components/api/api.proto` changed in 6 of the last 8 tracked
  commits touching that file (2026-05 through 2026-08-24); `aioesphomeapi` cut three releases in
  the week before this ADR. No published compatibility guarantee to third-party server
  implementers is known to exist. Any implementation this project owns is a permanent,
  self-funded maintenance liability tracking someone else's release train.

## Options considered

| Option | Pros | Cons |
| --- | --- | --- |
| **(a) Hand-roll the native API protocol in this codebase** | Full control; MIT-compatible; fits ADR-0001's transport-boundary pattern | Means implementing protobuf framing *and* a Noise/ChaCha20-Poly1305 handshake ourselves with no audit trail; tracks an unversioned, actively-changing upstream protocol forever; disproportionate to a goal already met by MQTT |
| **(b) Adopt `BentuinoESPHomeAPI`** | Real, working, PlatformIO/Arduino, ESP8266-supported | AGPL-3.0 vs. this project's MIT license; ~2.5 months old, 0 stars, single maintainer; unaudited vendored crypto; same unversioned upstream dependency; RAM cost leaves little/no room for Aqua's own sensors+OLED+MQTT together |
| **(b′) Drop this codebase for this variant; use ESPHome YAML directly** | Gets native encryption, OTA, HA device card "for free" | Abandons PlatformIO/Arduino/ADR-0001 for this variant; reverses D-006 for real; forks the fleet's tooling into two build systems with no shared host-tested core |
| **(c1) MQTT only (status quo)** | Zero new work; already accepted, tested, host-covered contract; broker already running | Doesn't give a native HA device card, OTA-via-HA, or a broker-free path |
| **(c2) HTTP push to Home Assistant's REST API** | Brokerless HA integration at a fraction of the cost of (a)/(b): no protobuf, no Noise handshake, no vendored crypto, trivial on ESP8266's existing WiFiClient/HTTPClient | Not "ESPHome" — no native device card/OTA; swaps broker dependency for an HA-REST-endpoint dependency rather than eliminating infrastructure |

## Decision

We keep MQTT as AtmosMesh's sole accepted transport for this decision cycle, declining both
(a) hand-rolling and (b) adopting `BentuinoESPHomeAPI`, because the existing MQTT + HA-discovery
contract already delivers the integration outcome against infrastructure that already runs, while
both routes to the ESPHome native API mean owning a security-critical, actively-churning,
externally-unversioned protocol — and (b) additionally carries an AGPL-3.0/MIT license conflict
and unaudited from-scratch crypto. This is chosen deliberately over (b′) full ESPHome YAML, which
would require abandoning this repository's PlatformIO/Arduino/ADR-0001 architecture, and over a
silent status-quo default — (c1) is evaluated and chosen, not defaulted into by omission.

If the operator's actual underlying goal is "Home Assistant without running a broker" rather than
"ESPHome specifically," (c2) HTTP push to HA's REST API is the right-sized alternative. This is
**not** authorized by this ADR — it is named as a candidate follow-up story only if the operator
confirms that is the actual goal, decoupled from this decision.

### Runtime-switchable vs. compile-time-switchable (sub-decision)

Settled by precedent and headroom: ADR-0001 already establishes product profiles as compile-time
declarations and transports as composed at a compile-time boundary, one canonical environment per
product/variant. The RAM evidence above makes this non-negotiable if ever adopted: a runtime
toggle would keep both stacks resident and would not fit in ESP8266's heap. Any future
implementation must be a second canonical PlatformIO environment, never a runtime flag.

## Consequences

- AtmosMesh v1, Grove v1.5, and the new Aqua variant (see story AQ-01) ship MQTT-only. No new
  PlatformIO environment, library dependency, or transport code is added by this ADR.
- D-006 and OQ-001 remain as recorded; OQ-001's revisit trigger fired and was evaluated here —
  outcome: still resolved, not reopened.
- This forecloses (a)/(b)/(b′) implementation work for the current milestone, not forever.
- **Revisit triggers** (any one reopens this ADR):
  1. `BentuinoESPHomeAPI` (or an equivalent) relicenses to an MIT/Apache/BSD-compatible license,
     accumulates independent maturity/audit signal, and a measured ESP8266 RAM budget on a real
     Aqua-class build proves it fits — compile-time-only, never alongside MQTT in the same image.
  2. The operator confirms the real goal is broker-independence, not ESPHome specifically — pursue
     (c2) as its own story instead.
  3. ESPHome publishes an explicit compatibility/versioning guarantee for third-party server
     implementations of `api.proto`.

## Migration

None. This ADR authorizes no code change. A future ADR that supersedes this one must add the
chosen library as a normal PlatformIO `lib_deps` entry behind its own canonical environment, never
a build flag inside a shared environment, per ADR-0001's composition-root pattern.

## Validation

N/A for this decision-only ADR — no build, test, or flash changes result from it. If a future ADR
reverses this decision, validation must add: (1) a licensing review gate before any AGPL-adjacent
or copyleft dependency is vendored, (2) an explicit security review of any vendored crypto before
merge, and (3) the existing ADR-0001 gate matrix (native tests, both/all canonical builds,
independent review before flash).

# Architecture

## Product variants

AtmosMesh keeps the existing **AtmosMesh v1** ESP32 station entrypoint and full sensor/MQTT stack.
**AtmosMesh Grove v1.5** is the smaller ESP8266 variant (`atmosmesh-grove-0001` by default) with a
thin profiled entrypoint for its 128×32 OLED, BMP180, DHT11 and uncalibrated D7 RC light response.
PlatformIO source filters keep target transports/runtimes separate, while measurement validity,
display decisions, MQTT identity/discovery/state formatting and reconnect sequencing are shared,
host-testable utilities. This does not claim the legacy ESP32 composition root is thin/profile-driven.

Both composition roots are explicitly named under `firmware/src/products/`; the ESP32 source was
moved there without behavior changes. Compile-time identity metadata now exists for both products,
while incremental ESP32 adoption remains migration work. The full proposal, alternatives and
version rules are in [ADR-0001](adr/0001-multi-product-firmware-composition.md).

Grove v1.5 has its own ID-based MQTT product contract and a thin ESP8266WiFi/PubSubClient transport;
hardware network validation remains pending. It does not inherit claims for ESP32-only SDS011,
MQ135, PIR, VEML7700 or beeper devices. YL-69/YL-38 remains separately gated; MAX4466 is dropped.

## Context

The system has three independently testable boundaries:

1. **Device:** sensors, local display, validation, health, Wi-Fi, and MQTT publication.
2. **Messaging/platform:** broker, persistence, metrics ingestion, and service health.
3. **User experience:** Home Assistant entities, Grafana history, and alerting.

## Intended data flow

```text
Sensors -> validation -> device state -> OLED
                                  └-> MQTT state + availability
                                                    ├-> Home Assistant
                                                    └-> metrics bridge -> Prometheus -> Grafana
```

## Device power (station target)

See `docs/hardware/power.md` and decision D-005. One enclosed isolated 5 V rail feeds ESP32 `VIN`/`5V`
and 5 V sensors. The DevBoard LDO feeds 3.3 V logic and small 3.3 V loads. GPIOs are 3.3 V; 5 V
signals are not connected directly. Bench flashing stays on USB until that rail is enclosed and
measured.

## Device responsibilities

- Sample sensors independently; one failed sensor must not block the others.
- Carry value, unit, validity, and age as separate concepts.
- Never convert a missing value into numeric zero.
- Keep the local display useful while Wi-Fi or MQTT is unavailable.
- Reconnect with bounded backoff and publish MQTT Last Will availability.
- Avoid credentials in source-controlled files.

## MQTT contract (RLS-05)

Frozen identity (no room name in any topic — Home Assistant areas are assigned later):

| Piece | Value |
| --- | --- |
| Device id | `atmosmesh-v1` |
| Station id | `atmosmesh-0001` |
| State topic | `home/air/atmosmesh-0001/state` (JSON, not retained) |
| Availability | `home/air/atmosmesh-0001/availability` (`online` / `offline`, retained + LWT) |
| Discovery | `homeassistant/{sensor\|binary_sensor}/atmosmesh_0001/<object_id>/config` (retained) |
| Broker | LAN plain MQTT `:1883`, username `homeassistant`, no TLS (kumulus Mosquitto) |

State JSON includes `"id":"atmosmesh-0001"` and `"device":"atmosmesh-v1"`. Each reading is
`{value, unit, valid, age_ms}`; invalid readings omit `value` entirely. MQ135 publishes
`gas_index` (unit `index`) and optional `mq135_raw` — never `co2` / `ppm`. Lux is omitted until
a light sensor is fitted. Discovery configs are re-published on every MQTT connect because the
kumulus broker runs with persistence off.

Credentials live in a gitignored `.envrc` (see `.envrc.example`). Both canonical product build and
flash tasks run `scripts/gen-secrets-from-env` to materialise
`firmware/include/atmosmesh/secrets.hpp`. Without those credentials, sensors and OLED
still run; Wi-Fi/MQTT stay off.

### Grove v1.5 MQTT contract (D-013)

| Piece | Value |
| --- | --- |
| Product id | `atmosmesh-grove-v1.5` |
| Station id | `atmosmesh-grove-0001` |
| State topic | `home/air/atmosmesh-grove-0001/state` (JSON, not retained) |
| Availability | `home/air/atmosmesh-grove-0001/availability` (`online` / `offline`, retained + LWT) |
| Discovery | `homeassistant/sensor/atmosmesh_grove_0001/<object_id>/config` (retained) |

Grove state uses optional scalar keys `temperature_c`, `humidity_pct`, `pressure_hpa` and
`light_charge_us`; invalid readings are omitted while valid numeric zero remains publishable. Home
Assistant discovery covers exactly those four entities and is replayed on every connect. Light is
an **uncalibrated RC charge time** in `µs`, lower meaning brighter; it is never lux/illuminance or
percent. Both product builds use the same generated, gitignored secrets. Missing credentials or
network/broker loss leaves local sensing and OLED work active.

## Deployment responsibilities

- Declarative, reproducible configuration.
- Persistent configuration/history where required.
- Secrets provided outside committed manifests.
- Health checks appropriate to each workload.
- A stable broker address reachable from the home network without exposing it publicly by default.

## Out of scope for the MVP

- Mains control, open mains on the bench, or safety automation.
- Certified environmental or health monitoring.
- Battery optimization.
- Custom PCB.
- 480×320 local graphical UI.
- True CO₂ until a confirmed NDIR sensor is available.

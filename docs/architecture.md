# Architecture

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

Credentials live only in the gitignored `firmware/include/atmosmesh/secrets.hpp` (copy from
`secrets.hpp.example`). Without that file, sensors and OLED still run; Wi-Fi/MQTT stay off.

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

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

## Device responsibilities

- Sample sensors independently; one failed sensor must not block the others.
- Carry value, unit, validity, and age as separate concepts.
- Never convert a missing value into numeric zero.
- Keep the local display useful while Wi-Fi or MQTT is unavailable.
- Reconnect with bounded backoff and publish MQTT Last Will availability.
- Avoid credentials in source-controlled files.

## Initial MQTT contract

The final payload is frozen in RLS-05. Initial topic candidates are:

```text
home/air/wohnzimmer/state
home/air/wohnzimmer/status
home/air/wohnzimmer/availability
```

Names must be stable and units explicit. A gas trend from MQ135 must remain semantically separate
from any future NDIR CO₂ measurement.

## Deployment responsibilities

- Declarative, reproducible configuration.
- Persistent configuration/history where required.
- Secrets provided outside committed manifests.
- Health checks appropriate to each workload.
- A stable broker address reachable from the home network without exposing it publicly by default.

## Out of scope for the MVP

- Mains control or safety automation.
- Certified environmental or health monitoring.
- Battery optimization.
- Custom PCB.
- 480×320 local graphical UI.
- True CO₂ until a confirmed NDIR sensor is available.

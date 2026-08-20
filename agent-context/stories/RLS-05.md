# RLS-05 — Publish a reliable MQTT contract

- **Status:** In flight
- **Priority:** P0
- **Milestone:** M3 — Connected station
- **Depends on:** RLS-03 (operator overrode sequencing — implement MQTT without claiming RLS-03 done)

## User story

As the home-automation operator, I want stable, well-labelled MQTT messages so that Home Assistant
and metrics consumers can use the same trustworthy device state.

## Outcome

The station publishes state, health, and availability with explicit units and reconnects without
blocking local measurement or display.

## In scope

- Topic and payload contract, device ID, units, validity, timestamps/age, and availability.
- MQTT Last Will, bounded reconnect backoff, and non-secret configuration.
- Contract tests and example messages.
- Home Assistant MQTT discovery for sensors present without lux.

## Out of scope

- Production dashboards, alerts, public broker exposure, or remote actuation.
- Lux / VEML entity until the part is fitted.
- OLED Wi-Fi/MQTT health chrome (RLS-04).

## Acceptance criteria

- [x] Topic and payload schema are documented and machine-tested.
- [x] State includes stable names, units, validity, and device identity.
- [x] MQ135 trend and future NDIR CO₂ are distinct concepts.
- [x] Last Will marks the station unavailable after an ungraceful disconnect.
- [x] Wi-Fi and broker recovery use bounded backoff and recover without reboot.
- [x] Network loss does not block sampling or the local UI.
- [x] No real credential is stored in Git or emitted in normal logs.

## Validation

- Automated: `pio test -e native` suites `test_mqtt` (schema, discovery, backoff/session) and
  `test_native`.
- Manual (owed after flash on LAN with `secrets.hpp`):
  1. `mosquitto_sub -h <kum3-lan> -p 1883 -u homeassistant -P '<from sops>' -t 'home/air/atmosmesh-0001/#' -v`
  2. Also subscribe `homeassistant/+/atmosmesh_0001/#`
  3. Interrupt broker and Wi-Fi; confirm OLED/serial keep updating; confirm LWT `offline` then
     rediscovery + `online` without reboot.

## Evidence

- Host Unity: MQTT contract + session tests in `firmware/test/test_mqtt/`.
- Contract frozen in `docs/architecture.md` and D-007.
- Manual (2026-08-20): flashed with `.envrc` → `secrets.hpp`; serial showed Wi-Fi connect and
  `mqtt: connected` to kum3 LAN `192.168.178.82:1883`; subscriber received
  `home/air/atmosmesh-0001/availability` (`online`), eight HA discovery `/config` topics, and
  `home/air/atmosmesh-0001/state` JSON.

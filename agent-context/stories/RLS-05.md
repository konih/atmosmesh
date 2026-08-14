# RLS-05 — Publish a reliable MQTT contract

- **Status:** Blocked
- **Priority:** P0
- **Milestone:** M3 — Connected station
- **Depends on:** RLS-03

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

## Out of scope

- Production dashboards, alerts, public broker exposure, or remote actuation.

## Acceptance criteria

- [ ] Topic and payload schema are documented and machine-tested.
- [ ] State includes stable names, units, validity, and device identity.
- [ ] MQ135 trend and future NDIR CO₂ are distinct concepts.
- [ ] Last Will marks the station unavailable after an ungraceful disconnect.
- [ ] Wi-Fi and broker recovery use bounded backoff and recover without reboot.
- [ ] Network loss does not block sampling or the local UI.
- [ ] No real credential is stored in Git or emitted in normal logs.

## Validation

- Automated: schema/contract tests and reconnect state-machine tests where possible.
- Manual: subscribe to all topics; interrupt broker and Wi-Fi; verify offline/online transitions.

## Evidence

Blocked by RLS-03.

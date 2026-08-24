# AtmosMesh Grove v1.5 software handoff — 2026-08-24

## Completed

- V15-01 hardware/identity contract and V15-02 shared variant architecture.
- Proposed ADR-0001, two-product matrix, named composition roots and canonical/compatibility builds.
- Native-tested profile and explicit health/display formatting.
- Compiling ESP8266 Grove image plus preserved ESP32 build and task commands.
- Product, wiring, GPIO0 boot caveat and deferred analog constraints reconciled across docs.

## Hardware boundary

Nothing was flashed. The ESP8266 still has its working AT firmware. The operator authorized
replacement on 2026-08-24; V15-03's physical criterion now awaits fresh independent review and the
coordinator's controlled flash/monitor session. DHT11 DATA=D5/GPIO14 still needs physical proof.

## Next validation

After review, use `task flash-v1-5` once on a known-good USB path, then `task monitor-v1-5`.
Capture startup and a sensor cycle, confirm the 128×32 four-line layout, and disconnect one sensor
at a time to prove `ERR` behavior. Do not begin V15-04 analog work until the board's A0 divider and
external ADC/channel-sharing design are confirmed.

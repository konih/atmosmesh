# AtmosMesh Grove v1.5 software handoff — 2026-08-24

## Completed

- V15-01 hardware/identity contract and V15-02 shared variant architecture.
- Native-tested profile and explicit health/display formatting.
- Compiling ESP8266 Grove image plus preserved ESP32 build and task commands.
- Product, wiring, GPIO0 boot caveat and deferred analog constraints reconciled across docs.

## Hardware boundary

Nothing was flashed. The ESP8266 still has its working AT firmware. V15-03's physical criterion is
blocked until the operator explicitly authorizes replacement, verifies DHT11 DATA=D5/GPIO14, and
accepts the GPIO0/D3 shared-I²C boot risk.

## Next validation

After authorization, use `task flash-grove` once on a known-good USB path, then `task monitor-grove`.
Capture startup and a sensor cycle, confirm the 128×32 four-line layout, and disconnect one sensor
at a time to prove `ERR` behavior. Do not begin V15-04 analog work until the board's A0 divider and
external ADC/channel-sharing design are confirmed.

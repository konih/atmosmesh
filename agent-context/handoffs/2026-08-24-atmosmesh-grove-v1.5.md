# AtmosMesh Grove v1.5 software handoff — 2026-08-24

## Completed

- V15-01 hardware/identity contract and V15-02 shared variant architecture.
- Accepted ADR-0001, two-product matrix, named composition roots and canonical/compatibility builds.
- Native-tested profile and explicit health/display formatting.
- Compiling ESP8266 Grove image plus preserved ESP32 build and task commands.
- Product, wiring, GPIO0 boot caveat and deferred analog constraints reconciled across docs.

## Hardware result

The coordinator flashed reviewed head `a681990` under explicit authorization. Boot identity and
the D2/D3 bus matched the Grove profile. BMP180 passed five stable samples. OLED initialization at
0x3C passed, but pixels are not visually confirmed. DHT11 on profile D5/GPIO14 failed every read.
The captured banner reported product name, variant and station ID but not a separate stable product
ID. After independent approval of final head `50ca2f3`, a second authorized flash wrote 287,952
verified bytes and captured the corrected identity contract exactly:

```text
product=AtmosMesh Grove product_id=atmosmesh-grove-v1.5 variant=atmosmesh-v1.5 station_id=atmosmesh-grove-0001
```

OLED initialization again passed but pixels remain visually unconfirmed. BMP180 passed four more
samples at 24.6–24.7 °C / 984.3 hPa. DHT11 remained unavailable for all four cycles. AtmosMesh v1
and the ESP32 were untouched.

## Next validation

Do not flash again just to diagnose DHT11. Verify its actual DATA joint/pin, 3.3 V/GND orientation
and 4.7–10 kΩ pull-up/module resistor; then capture valid samples and visually confirm the 128×32
four-line layout. Do not begin V15-04 until the A0 divider and ADC/channel design are confirmed.

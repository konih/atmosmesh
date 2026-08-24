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
samples at 24.6–24.7 °C / 984.3 hPa. DHT11 was initially unavailable, then a later run produced
32.0→31.0 °C / 32% RH on D5/GPIO14 while BMP180 reported 25.6 °C / 983.9–984.0 hPa. Treat this as
communication evidence, not DHT accuracy/calibration. AtmosMesh v1 and the ESP32 were untouched.

## Next validation

V15-04/V15-05 add the installed D7/GPIO13 bare-LDR RC response and Grove MQTT contract. The product
diff was independently approved at `e5d83e1` and CI is green. During the authorized follow-up,
secret generation and the firmware build succeeded, but `/dev/cu.usbserial-0001` disappeared before
esptool opened it. Upload failed with `FileNotFoundError` before connecting or writing, so the Grove
still runs its prior firmware and no bytes from `e5d83e1` reached the device. `ls /dev/cu.*` showed
no USB serial device and ioreg showed hubs only; do not infer a firmware or board failure from this.

Reconnect/power the USB serial device and confirm its port, then validate raw light timing across
bright/dark conditions, timeout behavior, the four-line OLED page, Wi-Fi/broker-loss recovery,
retained availability and the Grove Home Assistant entities. Prior DHT11 communication evidence on
D5/GPIO14 remains valid, but is not an accuracy claim. MAX4466 is dropped.

The USB serial device later reappeared. V15-06 adds the already-installed bi-color LED on
red=D6/GPIO12 and green=D0/GPIO16, plus a cooperative YL-38 raw ADC policy using an active-low
D1/GPIO5 high-side control. The operator then confirmed and physically wired a 2N3906 PNP:
emitter=3V3, collector=YL-38 VCC, base through 2.2 kΩ to D1, and 100 kΩ base-emitter pull-up. AO is
wired through 47 kΩ / 15 kΩ to A0 with `104` decoupling; DO is unused. Software must receive fresh
independent review before the coordinator flashes it.

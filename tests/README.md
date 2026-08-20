# Tests

Host-side firmware tests live next to the code they protect:

```bash
pio test -e native
```

Run that from `firmware/` (or `task test` from the repo root). PlatformIO Unity covers:

- `test_native` — OLED banner clipping, D5/D4 pin constants, SSD1306 address selection
  (0x3C then 0x3D), OLED controller/geometry, SDS011 parse/listen logs, MQ135 ADC millivolt
  scale (never CO₂), PIR debounce, VEML7700 formatters.
- `test_mqtt` — MQTT topics (`atmosmesh-0001`, no room), state JSON units/validity, Home
  Assistant discovery configs (no lux), LWT/availability strings, reconnect backoff, and the
  session rule that disconnected ticks publish nothing.

These tests must fail if that behaviour is removed.

Still required later (not yet automated):

- broader sensor stale-state matrix beyond the MQTT contract;
- rendered Kubernetes manifests;
- an end-to-end synthetic measurement from device boundary to dashboard/alert;
- documented manual electrical checks and the 30-minute/48-hour run evidence;
- manual Wi-Fi/broker interrupt after flash (see RLS-05).

Hardware measurements are evidence, not a replacement for host-side automated tests.

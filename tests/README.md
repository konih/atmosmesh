# Tests

Host-side firmware tests live next to the code they protect:

```bash
pio test -e native
```

Run that from `firmware/`. PlatformIO Unity covers OLED banner clipping, D5/D4 pin constants,
SSD1306 address selection (0x3C then 0x3D), OLED controller/geometry (default SH1106 128×64 with
2-pixel offset; SSD1306 sequential COM as compile fallback ID=0), U8g2 constructor names and
prove-life serial lines, SDS011 listen-on-GPIO16 log, and MQ135 ADC millivolt scale
(10 kΩ/20 kΩ divider inverse; never CO₂). These tests must fail if that behaviour is removed.

Still required later (not yet automated):

- sensor parsing and invalid/stale states;
- MQTT payload schema and units;
- reconnect and Last Will behavior;
- rendered Kubernetes manifests;
- an end-to-end synthetic measurement from device boundary to dashboard/alert;
- documented manual electrical checks and the 30-minute/48-hour run evidence.

Hardware measurements are evidence, not a replacement for host-side automated tests.

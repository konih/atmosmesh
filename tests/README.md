# Tests

Host-side firmware tests live next to the code they protect:

```bash
pio test -e native
```

Run that from `firmware/`. PlatformIO Unity covers OLED banner clipping, D5/D4 pin constants,
and SSD1306 address selection (0x3C then 0x3D). These tests must fail if that behaviour is removed.

Still required later (not yet automated):

- sensor parsing and invalid/stale states;
- MQTT payload schema and units;
- reconnect and Last Will behavior;
- rendered Kubernetes manifests;
- an end-to-end synthetic measurement from device boundary to dashboard/alert;
- documented manual electrical checks and the 30-minute/48-hour run evidence.

Hardware measurements are evidence, not a replacement for host-side automated tests.

# Bluetooth mouse jiggler

A standalone image for the same ESP32-C3 SuperMini with built-in 0.42-inch 72×40 OLED used by
AtmosMesh Spot. It does not run the Spot sensors. USB supplies power and programming; the C3's
fixed-function USB Serial/JTAG peripheral cannot present a USB mouse.

## Use with a Mac

1. Build and flash the `atmosmesh-jiggler` environment (commands below).
2. Open macOS System Settings → Bluetooth, select **AtmosMesh Jiggler**, and connect.
3. The OLED shows **BT connected / OFF** once the encrypted HID connection is subscribed.
4. Tap the board's **BOOT** button after boot, or the external button, to toggle **ON / OFF**.
   Do not hold BOOT during power-on: that selects the chip's download mode.
5. When ON, every 15 seconds the mouse sends +1 horizontal relative count, then −1 after 80 ms.
   Pointer acceleration and screen edges mean exact visual return is not guaranteed.

The device boots OFF. Disconnecting, unsubscribing, or a failed motion notification disables it;
reconnecting requires another button press. A button held when firmware starts is ignored until
released and pressed again. Pairing uses BLE bonding with no passkey (Just Works). The HID
descriptor includes three unused mouse buttons because hosts such as macOS reject a buttonless
mouse and can bounce Bluetooth during pairing; firmware never sets those buttons. If a previous
attempt left a stale entry, forget **AtmosMesh Jiggler** on the Mac before pairing again. Pair
near the intended computer. To change hosts, disconnect the old host first.

No clicks, scrolling, keypresses, or attempts to conceal the device are implemented. Even tiny
movement can affect hover or an ongoing drag; toggle OFF while working. Whether mouse reports
prevent display sleep must be verified on the target Mac; this does not override lock or power
policies and is not intended to wake a sleeping computer.

## External momentary button

For the **confirmed Spot-type C3 board**, solder a normally-open momentary switch between the
pad marked **3 (GPIO3)** and **GND**. Internal pull-up and 30 ms debounce are enabled. No supply
voltage is connected to the switch. Do not wire it to RST/EN, which resets the chip rather than
toggling the application. Check the actual pad labels before soldering; a four-legged tactile
switch often has two permanently joined legs on each side.

GPIO4 is an alternative selected with `-DATMOSMESH_JIGGLER_BUTTON_GPIO=4` in the environment's
build flags. OLED is SDA GPIO5, SCL GPIO6, address 0x3C, powered at 3.3 V. The built-in BOOT
button on GPIO9 works without soldering. This image targets the built-in 72×40 display, not a
separate 128×64 module. Serial output reports a missing OLED; BLE remains usable.

## Build and flash

Install PlatformIO 6.1.19 on PATH. From the repo root:

```sh
task test
task build-jiggler
task flash-jiggler ESP_PORT=/dev/ttyACM0
task monitor-jiggler ESP_PORT=/dev/ttyACM0
```

On macOS replace the port with the board's `/dev/cu.usbmodem...` path. The firmware environment
pins espressif32 7.1.0, NimBLE-Arduino 2.3.6, and U8g2 2.36.15. No Wi-Fi secrets are needed.
Direct build: `pio run -d firmware -e atmosmesh-jiggler`.

## Scope and validation

This follows ADR-0001’s separate composition-root and explicit build-filter pattern. It is a
standalone HID utility, with no station measurements, MQTT identity, or sensor product profile.
Its environment and Task targets are separate from Spot so sensor firmware remains unchanged.

Host tests cover startup, debounce/hold, offline input, disconnect, disabling mid-movement,
periodicity, and timer wrap/late loops. An ESP32-C3 firmware build checks the actual BLE APIs.
On hardware check OLED text, pairing, toggling, reconnect remaining OFF, and display idle behavior
on the target Mac. Automated tests do not establish macOS pairing or physical button wiring.

MQTT/Home Assistant control, larger motion patterns, and keyboard emulation are deferred. BLE
keyboard is technically possible, but adds input risk with no benefit for this minimum version.
MQTT would add Wi-Fi configuration and command handling; it can be added after Mac validation,
with explicit state feedback and protection against retained commands enabling movement.

References: [Espressif C3 USB limitation](https://docs.espressif.com/projects/esp-idf/en/release-v5.0/esp32c3/api-guides/usb-serial-jtag-console.html),
[NimBLE HID API, pinned version](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEHIDDevice.h).

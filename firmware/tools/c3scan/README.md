# c3scan — I²C pin-pair scanner for the ESP32-C3 SuperMini OLED

A standalone PlatformIO project (its own `platformio.ini`, independent of `firmware/`) that
flashes the ESP32-C3 SuperMini OLED board and reports which SDA/SCL pin pair answers and at which
addresses. Written for SP-01's open question 1 ("which GPIOs carry the OLED?") and kept because
it is the quickest way to prove a fresh SuperMini and a fresh sensor before they go on a carrier.

It is also the first ESP32-C3 build in this repository: `platform = espressif32` 7.1.0 with the
Arduino core 3.20017 and the RISC-V toolchain built it on 2026-09-04 (the SP-02 toolchain spike,
in effect).

## Use

From this directory, with a PlatformIO venv on `PATH` (the shared agent venv does not carry
PlatformIO; `uv venv && uv pip install platformio pip` in a scratch directory is enough):

```sh
pio run                      # build only
pio run -t upload            # flash over the board's native USB (/dev/ttyACM0)
pio device monitor           # 115200 baud; a scan pass prints every 15 s
```

The board enumerates as `Espressif USB JTAG/serial debug unit` (303a:1001) on `/dev/ttyACM0`.
That device belongs to `root:dialout`; the user flashing needs to be in `dialout` (log out and
in after `sudo usermod -aG dialout $USER`) or the port opened for the session
(`sudo chmod a+rw /dev/ttyACM0`).

## What it prints

Chip model, revision and MAC, then for each of the pin pairs `(5,6) (6,5) (4,3) (3,4) (7,10)
(10,7) (2,1) (1,2) (0,1) (1,0) (20,21) (21,20) (8,9) (9,8)` the addresses that ACK between
`0x08` and `0x77`, with the usual suspects named (`0x3C` OLED, `0x44` SHT4x, `0x10` VEML7700,
`0x76` BME/BMP280). With nothing plugged into the module the only expected hit is the on-board
OLED at `0x3C`; the pair it appears on is the answer to SP-01's open question 1.

## PlatformIO gotchas seen on 2026-09-04

- A venv created with `uv` has no `pip`; PlatformIO 6.x needs it to install `tool-esptoolpy`
  2.41100.x (a pip package now) and otherwise fails with `MissingPackageManifestError`. Fix:
  `uv pip install --python <venv>/bin/python pip`, delete `~/.platformio/packages/tool-esptoolpy`,
  run again.
- The build flags `-DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1` are what make `Serial`
  talk over the board's native USB; without them the output goes to the unconnected UART0 pins.

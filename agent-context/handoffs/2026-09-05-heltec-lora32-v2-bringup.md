# Heltec WiFi LoRa 32 V2 bring-up handoff — 2026-09-05

- Status: Blocked (operator action)
- Done:
  - Board identified over USB: ESP32-D0WDQ6 rev 1.0, 8 MB flash, MAC `3c:61:05:0e:04:ec` → Heltec V2
    (classic ESP32, SX1276 class radio). Facts in `docs/hardware/inventory.md`.
  - Stock factory firmware captured for 30 s: `LoRa Initial success!` at boot, no packet log.
  - Full 8 MB flash backed up to
    `PlatformRelay/.tooling/firmware-backups/heltec-wifi-lora32-v2_3c61050e04ec_stock-factory_2026-09-05.bin`.
  - Host tooling repaired: the shared venv `PlatformRelay/.tooling/python/atmosmesh` was a dead
    symlink set from the old Mac; rebuilt with `uv` (Python 3.12, esptool 5.3.1, pyserial,
    meshtastic CLI 2.7.11).
  - Plan and idea list written: `docs/hardware/lora-remote-node.md`.
- Evidence: esptool output and serial capture quoted in the inventory row.
- Blocked by: the coverage/link test needs a listener firmware on the board. Meshtastic no longer
  ships a Heltec V2 image; the generic `meshtastic-diy-v1` 2.7.26 image (radio SPI + DIO0 pins
  match the V2) is downloaded but writing it is the operator's decision.
  The serial port needs the `dialout` group; the current login session predates the membership,
  so run through `sudo -g dialout` or log in again.
- Next (operator, ~15 min):
  1. `cd atmosmesh && sudo -u koni -g dialout ./scripts/esp-tool --port /dev/ttyUSB0 --baud 921600 erase-flash`
  2. `sudo -u koni -g dialout ./scripts/esp-tool --port /dev/ttyUSB0 --baud 921600 write-flash 0x0 <firmware-meshtastic-diy-v1-2.7.26.54e0d8d.bin> 0x300000 <littlefs-meshtastic-diy-v1-2.7.26.54e0d8d.bin>`
     (both files come from `gh release download v2.7.26.54e0d8d --repo meshtastic/firmware --pattern firmware-esp32-2.7.26.54e0d8d.zip`)
  3. Wait 30 s, then `sudo -u koni -g dialout ../.tooling/python/atmosmesh/bin/meshtastic --port /dev/ttyUSB0 --set lora.region EU_868`
  4. After 10–15 min: `… meshtastic --port /dev/ttyUSB0 --nodes`. Any node besides the local one =
     RX works and there is Meshtastic traffic in range. `… --info` shows the radio init state.
  5. If the radio fails to init on the diy-v1 pin map, restore the backup (command in
     `docs/hardware/lora-remote-node.md` §2) and go straight to the RadioLib P2P route (LORA-02).
- Do not:
  - Power the board with the antenna disconnected.
  - Connect an 18650 before the connector polarity is verified on the board (§4 of the plan).

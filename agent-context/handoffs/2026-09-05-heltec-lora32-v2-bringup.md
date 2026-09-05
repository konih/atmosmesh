# Heltec WiFi LoRa 32 V2 bring-up handoff — 2026-09-05

- Status: Done for the listen test; TX proof open (needs a second radio or a longer capture)
- Done:
  - Board identified over USB: ESP32-D0WDQ6 rev 1.0, 8 MB flash, MAC `3c:61:05:0e:04:ec` → Heltec V2
    (classic ESP32, SX127x class radio). Facts in `docs/hardware/inventory.md`.
  - Full 8 MB stock flash backed up to
    `PlatformRelay/.tooling/firmware-backups/heltec-wifi-lora32-v2_3c61050e04ec_stock-factory_2026-09-05.bin`.
  - Host tooling repaired: shared venv `PlatformRelay/.tooling/python/atmosmesh` rebuilt with `uv`
    (Python 3.12, esptool 5.3.1, pyserial, meshtastic CLI 2.7.11).
  - Operator gave the go-ahead; Meshtastic 2.7.26 `meshtastic-diy-v1` written at 115200 baud:
    `firmware-…factory.bin` @ `0x0`, `mt-esp32-ota.bin` @ `0x260000`, `littlefs-…bin` @ `0x300000`
    (hash-verified). Region set to `EU_868`. Node id `!050e04ec`.
  - Listen result: `RF95 init success`, 869.525 MHz LongFast, 20 dBm, noise floor −107…−110 dBm;
    ~10 min across two windows: `num_packets_rx=0`, `num_packets_tx=0`, `--nodes` lists only self.
  - Plan and idea list: `docs/hardware/lora-remote-node.md` (§2 has the coverage verdict).
- Evidence: serial captures in the job's tmp dir (not kept); the key lines are quoted in the
  inventory row and plan §2.
- Gotchas learned:
  - Writing the app-only `firmware-*.bin` at `0x0` boot-loops (`flash read err, 1000`); use the
    `*.factory.bin` merged image.
  - Every serial port open resets the board (CP2102 DTR/RTS), so a CLI command and a log capture
    cannot overlap; the node's first NodeInfo broadcast is skipped as "sent <600 s ago".
  - Stay at 115200 baud; 460800/921600 fail after the esptool stub loads.
  - Run serial tools via `sudo -n -u koni -g dialout …` until the login picks up the group.
- Next:
  1. TX proof: keep one capture open for >10 min without any CLI access
     (`meshtastic --port /dev/ttyUSB0 --noproto` works as the terminal) and wait for the node's own
     NodeInfo/telemetry broadcast (`Starting low level send` … `completeSending`), or shorten
     `telemetry.device_update_interval` first. Alternative: any second LoRa radio (LORA-02).
  2. Coverage: repeat the listen for a few hours and once outdoors/at a window with the real
     antenna screwed on; any node in `--nodes` besides `!050e04ec` = RX + neighbours.
  3. Restore the stock image when done testing Meshtastic (plan §2), or keep diy-v1 as the
     home-end listener once a second radio arrives.
- Do not:
  - Power the board with the antenna disconnected.
  - Connect an 18650 before the connector polarity is verified on the board (§4 of the plan).

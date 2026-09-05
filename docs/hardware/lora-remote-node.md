# LoRa remote sensor node — Heltec WiFi LoRa 32 V2 + 18650

Plan and idea list for a battery-powered outdoor/remote AtmosMesh node built on the Heltec board
that arrived on 2026-09-05. Status of the board itself is in
[inventory.md](inventory.md#heltec-wifi-lora-32-v2--usb-probe-2026-09-05). Nothing here is wiring
approval; every module still goes through the front/back photo rule in `AGENTS.md`.

## 1. What the board is, and what that decides

| Fact | Value | Source |
| --- | --- | --- |
| MCU | ESP32-D0WDQ6 rev 1.0, 8 MB flash, 40 MHz crystal, MAC `3c:61:05:0e:04:ec` | esptool probe 2026-09-05 |
| Board generation | **V2** (classic ESP32). The V3 is an ESP32-S3 with an SX1262; this is not that | chip identity |
| Radio | SX1276/SX1278 family, 868/915 MHz build (seller listing) | not yet read from the chip |
| USB-UART | CP2102 (`10c4:ea60`) | lsusb |
| Display | 0.96" 128×64 SSD1306, I²C on the board's own pins (V2: SDA GPIO4, SCL GPIO15, RST GPIO16) | Arduino core `heltec_wifi_lora_32_V2` variant; confirm on silkscreen |
| LoRa SPI | SCK 5, MISO 19, MOSI 27, NSS 18, RST 14, DIO0 26, DIO1 35, DIO2 34 | same variant file |
| Li-ion path | 2-pin 1.25 mm battery connector, onboard 500 mA-class linear charger, `Vext` 3.3 V switched rail (GPIO21, active low), battery sense on an ADC pin through the board's divider | Heltec docs; **verify polarity and divider on the board before connecting a cell** |
| Stock firmware | Heltec factory test: prints `LoRa Initial success!` and `ESP32ChipID` at boot, then reports nothing further on serial | 30 s capture 2026-09-05 |
| Backup | Full 8 MB image at `PlatformRelay/.tooling/firmware-backups/heltec-wifi-lora32-v2_3c61050e04ec_stock-factory_2026-09-05.bin` | read-flash 2026-09-05 |

Two consequences of "V2, not V3":

1. **Meshtastic dropped this board.** No 2.5/2.6/2.7 release ships a `heltec-v2.1` image, so the
   zero-firmware Meshtastic telemetry route is closed unless the generic `meshtastic-diy-v1` image
   proves to work (it shares SCK/MISO/MOSI/NSS/DIO0 with the V2 and differs only in RST/DIO1/DIO2,
   which the SX1276 receive path does not need). The firmware routes that definitely work are
   RadioLib point-to-point and LoRaWAN (MCCI LMIC / RadioLib LoRaWAN).
2. **Sleep current is the weak spot.** The V2 keeps its CP2102 and LDO powered in deep sleep; the
   community reports roughly the 1 mA class (the V3 is tens of µA). Treat this as a number to
   **measure with the INA226 in stock** before sizing the battery, not a datasheet value.

## 2. Coverage and link test (blocked on the operator)

With a single LoRa node there is nothing to talk to, so "coverage" can only mean hearing other
people's traffic. The candidate test is a Meshtastic listen on EU_868 for 10–15 minutes: any node in
the `--nodes` table proves both RX and a live neighbourhood; the log lines around the boot
broadcast prove TX at the SX1276 level. The steps are in the 2026-09-05 handoff. The write itself is
the operator's to run: it replaces the factory image (backed up above). Restore is one command:

```bash
./scripts/esp-tool --port /dev/ttyUSB0 --baud 921600 write-flash 0x0 \
  ../.tooling/firmware-backups/heltec-wifi-lora32-v2_3c61050e04ec_stock-factory_2026-09-05.bin
```

For **LoRaWAN** coverage (The Things Network), check `ttnmapper.org` for the address and, if a gateway
is within a few km, register a device on TTN and run an OTAA join; a join accept is the coverage
proof. That is a separate afternoon and needs a TTN account.

## 3. Recommended first build: the Garden node

Everything below is in the 2026-09-04 inventory count and runs from 3.3 V, so no level shifters, no
5 V rail, and no mains.

| Function | Part in stock | Notes |
| --- | --- | --- |
| Air T/RH/p | BME280 (6 available; one photo-verified, 3.3 V only, `0x76`) | I²C |
| Soil moisture ×2 | Capacitive "Soil Sensor v2.0.0" (5 available) | Analog out; ADC1 pin; power from `Vext` so it is off while sleeping; calibrate dry/wet as done for Grove |
| Soil temperature | DS18B20 encapsulated probe, 2.5 m cable (2 available) | 1-Wire, 4.7 kΩ pull-up; bury the probe |
| Light | VEML7700 `HW-900` (3 available, one photo-verified) | I²C; lux for "did the plant get sun" |
| Self-telemetry | Battery voltage through the board's divider; optionally INA226 (1) on the battery lead | Report volts, never a percentage without a curve |
| Status | On-board OLED, on only while the button is held or for the first 10 s after wake | OLED draws 5–10 mA; keep it off |

Firmware shape (PlatformIO, `heltec_wifi_lora_32_V2` board, Arduino core, RadioLib):

1. Wake from deep sleep on a timer (10 min) or on the PRG button.
2. Raise `Vext`, wait 100 ms, read all sensors (≤ 1 s), lower `Vext`.
3. Send one binary LoRa frame (≤ 32 bytes: station id, sequence, T, RH, p, 2× soil raw, soil T, lux,
   Vbat, CRC) at 868.1–868.5 MHz, SF9/BW125, +14 dBm. That is ~0.2 s airtime, far inside the 1 % duty
   cycle that band allows.
4. Put the SX1276 to sleep, then `esp_deep_sleep`.

Battery estimate, to be replaced by INA226 measurements:

| Term | Value |
| --- | --- |
| Sleep floor (V2, community figures) | ~1 mA |
| Wake burst | ~120 mA for ~3 s every 600 s → ~0.6 mA average |
| Average | ~1.6 mA |
| One 18650 at 2500 mAh usable, 30 % derated | **~30 days** |
| With a 1–2 W panel on the charger | indefinite in summer, see the cold-charging note |

If 30 days is not enough and the sleep floor really is 1 mA, the cheaper fix is a V3 board
(~€20), not a bigger battery.

## 4. 18650 cells — what has to be true before one is connected

- **Cell identity.** Salvaged or unbranded 18650s vary from 1500 to 3500 mAh and some are 4.35 V
  chemistry. Measure capacity with a hobby charger or accept unknown runtime. Discard any cell
  below 2.5 V at rest or with a torn wrap.
- **Protection.** Use a protected cell or add a 1S protection board (DW01 + 8205 class) between
  cell and board. The board's charger has no over-discharge cut-off worth trusting.
- **Charger rating.** A 500 mA-class linear charger is 0.2 C for a 2500 mAh cell — fine. The
  charger heats the board; do not put the cell against it.
- **Connector polarity.** Heltec's 1.25 mm battery connector is not wired the same way as every
  pigtail sold as "JST 1.25". Verify + and − against the board's silkscreen with a meter before
  plugging in; reversed polarity is the classic way to kill this board.
- **Cold charging.** Li-ion must not be charged below 0 °C; the onboard charger has no temperature
  input. An outdoor solar node needs either a charger with NTC cut-off or a rule "no charging in
  winter" implemented in firmware by switching the panel through a MOSFET.
- **Antenna first.** Never power the board with LoRa TX possible and no antenna attached; the
  SX1276 PA is unprotected.

### To order (the cheap list)

| Item | Why | Rough price |
| --- | --- | --- |
| 1×18650 holder with leads (×2) | Mount the cell; one spare for the bench | €2 |
| 1.25 mm 2-pin pigtails (×5) | Battery lead to the board's connector | €3 |
| 1S protection boards or protected 18650s | See above | €3 / €6 per cell |
| 868 MHz antenna with u.FL/IPEX pigtail if the board came with a bare spring | Real range | €4 |
| IP65 enclosure ~100×70 mm + PG7 glands (×2) | Outdoor | €8 |
| **Second LoRa radio for the home end** — either a Heltec V3 (also fixes sleep current, keep the V2 as the gateway) or an RFM95W / E22-900M22S module wired to one of the spare ESP32-C6 / classic DevKit boards | There is no LoRa receiver in the house today | €20 / €8 |
| Optional: 6 V 1–2 W panel + CN3791 MPPT board | Solar top-up with a proper charger | €12 |

## 5. Home end: how the data gets to MQTT

| Option | Effort | Comment |
| --- | --- | --- |
| **A. Second LoRa board as a bridge** (recommended) | Firmware for two boards | ESP32 + LoRa RX → Wi-Fi → MQTT using the existing `esp32_mqtt_runtime` and Home Assistant discovery. Same protocol both ends, no cloud |
| B. LoRaWAN via TTN | Account + gateway in range | Free if a public gateway is within reach; otherwise a TTN indoor gateway (~€90). Data arrives at TTN's MQTT broker |
| C. Meshtastic mesh | Only if `diy-v1` works on the V2 | Environment telemetry module reads BME280 etc. natively; Home Assistant has a Meshtastic integration. Elegant, but the V2 is outside support |

## 6. Brainstorm — other nodes the inventory already covers

1. **Garden node** (above). Soil ×2, soil temp, air, light. Irrigation dashboard.
2. **Cellar / attic / freezer logger.** Two DS18B20 probes on their 2.5 m cables, nothing else.
   Lowest power of all; a year on a cell if the V2 sleep floor is tamed or a V3 is used.
3. **Balcony climate.** BME280 + VEML7700; add SGP41 for a VOC/NOx *trend* (never "air quality
   index" without calibration).
4. **Remote CO₂ room** (SCD41 in stock). SCD41 single-shot mode is ~0.5 mA average at a 5 min
   cadence; periodic mode is not battery-friendly. True CO₂, unlike the MQ family.
5. **Mailbox / gate / shed sentinel.** Reed switch or HC-SR501 PIR on a wake-up pin; send one
   frame per event. PIR modules want 5 V on VCC; feeding 3.3 V after the module's regulator is a
   known modification but must be done on the photographed board, not from a picture.
6. **Asset / bike beacon.** BMI160 (4 in stock) for motion wake, GPS module to order; sends a
   position every few minutes while moving. Meshtastic-style, no infrastructure.
7. **Rain barrel / cistern level.** JSN-SR04T waterproof ultrasonic (order) or a float switch.
   One reading per hour is plenty.
8. **Beehive scale.** DS18B20 for brood temperature + HX711 and load cells (order). The one that
   pays for itself.
9. **Solar rig self-monitor.** INA226 on the panel and battery leads; the node reports its own
   energy budget so the sleep-current estimate above is replaced by data.
10. **Range mapper.** Before any of the above: put the node on a power bank, walk it around the
    neighbourhood while the home bridge logs RSSI/SNR per frame, and draw the coverage map. This is
    the real "do we have LoRa coverage here" answer for point-to-point.

## 7. Suggested story order

| Story | Goal |
| --- | --- |
| LORA-01 | Board facts: photos, antenna type, battery connector polarity, sleep current with INA226 |
| LORA-02 | RadioLib P2P: V2 sends, a second board receives, RSSI/SNR logged; range walk |
| LORA-03 | Garden node firmware: sensors, frame format, deep sleep, 7-day bench run on the 18650 |
| LORA-04 | Home bridge: LoRa → MQTT → Home Assistant discovery, stale-value handling |
| LORA-05 | Outdoor enclosure, solar option, cold-charging rule |

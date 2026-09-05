# Delivery roadmap

The MVP is complete only when RLS-01 through RLS-08 are done. Optional work does not substitute
for an unfinished MVP story.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| 1 | [RLS-01](stories/RLS-01.md) | Ready | P0 | — | Hardware and wiring approved from evidence |
| 2 | [RLS-02](stories/RLS-02.md) | Blocked | P0 | RLS-01 | OLED and BMP280 work on the shared I²C bus |
| 3 | [RLS-03](stories/RLS-03.md) | Blocked | P0 | RLS-01, RLS-02 | All MVP sensors run together on the bench |
| 4 | [RLS-04](stories/RLS-04.md) | Blocked | P1 | RLS-02, RLS-03 | Useful local display and explicit health states |
| 5 | [RLS-05](stories/RLS-05.md) | In flight | P0 | RLS-03 | Stable MQTT contract and reconnect behavior |
| 6 | [RLS-06](stories/RLS-06.md) | Blocked | P1 | RLS-05, cluster context | Declarative home-automation services in Kubernetes |
| 7 | [RLS-07](stories/RLS-07.md) | Blocked | P1 | RLS-05, RLS-06 | Dashboard, history, and first alert end to end |
| 8 | [RLS-08](stories/RLS-08.md) | Blocked | P0 | RLS-01–RLS-07 | Safe 48-hour unattended run |
| — | [RLS-09](stories/RLS-09.md) | Optional | P3 | Confirmed NDIR sensor, RLS-08 | True CO₂ measurement |
| — | [RLS-10](stories/RLS-10.md) | Optional | P3 | RLS-08 | Evaluate 480×320 TFT separately |
| — | [RLS-11](stories/RLS-11.md) | Ready | P1 | Physical `atmosmesh-0001` disassembled | Archive AtmosMesh v1 code and live mentions |

## AtmosMesh Grove v1.5 variant

This is a separate ESP8266 product variant. It reuses shared, host-tested domain and display
utilities; it does not copy the ESP32 station's complete hardware stack. Its work does not change
the order or completion state of the RLS MVP above.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| V1.5-1 | [V15-01](stories/V15-01.md) | Done | P0 | — | ESP8266 identity and safe wiring contract recorded |
| V1.5-2 | [V15-02](stories/V15-02.md) | Done | P0 | V15-01 | Shared multi-product firmware architecture builds both targets |
| V1.5-3 | [V15-03](stories/V15-03.md) | Blocked | P0 | V15-02 and OLED visual confirmation | Identity, OLED, BMP180 and DHT11 validated on hardware |
| V1.5-4 | [V15-04](stories/V15-04.md) | Blocked | P1 | Reviewed hardware validation | Bounded uncalibrated D7 RC light response |
| V1.5-5 | [V15-05](stories/V15-05.md) | Blocked | P1 | Reviewed hardware/network validation | Grove MQTT state and Home Assistant discovery |
| V1.5-6 | [V15-06](stories/V15-06.md) | Blocked | P1 | V15-04, V15-05 software contracts | Bi-color health LED and duty-cycled raw YL-38 ADC |
| V1.5-7 | [V15-07](stories/V15-07.md) | Blocked | P1 | Operator visual confirmation | Descriptive four-row Grove OLED page |
| V1.5-8 | [V15-08](stories/V15-08.md) | Blocked | P1 | Independent review + operator visual evidence | Reversible OLED/LED visual diagnostic image |
| V1.5-9 | [V15-09](stories/V15-09.md) | Blocked | P1 | Operator visual confirmation + controlled dry/wet evidence | Calibration-safe soil/system status LED |
| V1.5-10 | [V15-10](stories/V15-10.md) | Ready | P1 | Serial `soil-led-status` while stuck | Grove LED reaches green when healthy; red/amber only for real faults |

## AtmosMesh Aqua variant

A third, independent ESP8266 product variant (SHT41 + genuine 128×64 OLED + duty-cycled water
probe). Its work does not change the order or completion state of the RLS MVP or Grove above.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| Aqua-1 | [AQ-01](stories/AQ-01.md) | In flight | P2 | ADR-0001, D-019/ADR-0002 | Aqua product composition root, host-tested, build-verified, independently reviewed and merged to `main`; hardware wiring/flashing blocked on photo evidence |

## Field corrections (multi-product)

Logged 2026-08-31 from live hardware. These do not rewrite the MVP order above.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| F-1 | [ENV-01](stories/ENV-01.md) | Ready | P1 | Paired enclosed vs outside reference readings | Grove + Aqua temps stop reading enclosure heat as ambient (relocate/vent/replace; offset only if labeled) |
| F-2 | [ROOM-05](stories/ROOM-05.md) | Ready | P0 | Live Room MQTT `motion` | Room occupancy polarity matches real presence |
| F-3 | [V15-10](stories/V15-10.md) | Ready | P1 | (also listed under Grove) | Grove LED not stuck red/amber |
| F-4 | [RLS-11](stories/RLS-11.md) | Ready | P1 | (also listed under RLS) | Disassembled `atmosmesh-0001` archived |

## AtmosMesh Room variant

A compact ESP32 room-station carrier using the ideaspark ESP32 1.14 inch TFT LCD board
(ESP32-WROOM-32), VEML7700, SHT41, D-SUN PIR and buzzer. This design track does not change the RLS
MVP order.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| Room-1 | [ROOM-01](stories/ROOM-01.md) | Blocked | P1 | Row-spacing and 5 V measurements | Protected carrier and complete build plan; energising remains blocked until the row spacing and both 5 V modules are measured |
| Room-2 | [ROOM-02](stories/ROOM-02.md) | Blocked | P1 | ROOM-01 + VIN and 5 V budget measurements | SDS011 with crossed, series-protected UART and a diodeless default-open 5 V rail |
| Room-3 | [ROOM-03](stories/ROOM-03.md) | Ready to solder | P1 | ROOM-01, ROOM-02 | 31×27 perfboard build plan with its own mutation-proved gate; the KiCad PCB is parked |
| Room-4 | [ROOM-04](stories/ROOM-04.md) | Proposed | P2 | ROOM-02, ROOM-03 | Firmware duty-cycling via the SDS011's own hibernation command; the MOSFET switch was rejected on topology (D-030) |
| Room-5 | [ROOM-05](stories/ROOM-05.md) | Ready | P0 | Live Room MQTT occupancy inverted | Flip PIR polarity so MQTT matches presence |

## AtmosMesh Spot variant

A 3.3 V presence-and-climate node on the ESP32-C3 SuperMini OLED (SHT41, VEML7700, HLK-LD2410S
radar, DS18B20 probe). First unit soldered and running since 2026-09-05. Does not change the
order above.

| Order | Story | Status | Priority | Depends on | Outcome |
| ---: | --- | --- | --- | --- | --- |
| Spot-1 | [SP-01](stories/SP-01.md) | In flight | P3 | — | Carrier built; acceptance tests left: empty-room presence, Room comparison, 24 h Wi-Fi, probe unplug |
| Spot-2 | [SP-02](stories/SP-02.md) | In flight | P3 | SP-01 | C3 product image on the unit: all sensors, radar distance/state, OLED, MQTT; HA and OLED-orientation checks left |

## Milestones

### M1 — Hardware approved

RLS-01 is done. No electrical assumption remains unresolved in the build table.

### M2 — Bench station

RLS-02 and RLS-03 are done. All sensors run together for at least 30 minutes.

### M3 — Connected station

RLS-04 and RLS-05 are done. Local UI and MQTT survive controlled failures.

### M4 — Platform integration

RLS-06 and RLS-07 are done. One measurement is traceable from sensor to dashboard and alert.

### M5 — MVP

RLS-08 is done. The station completes a safe 48-hour unattended run with documented recovery.

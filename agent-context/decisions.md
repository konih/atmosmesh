# Decisions and open questions

## Accepted decisions

### D-001 — Mini I²C OLED is the MVP display

- **Status:** Accepted
- **Reason:** Low pin count, low memory use, sufficient local feedback.
- **Consequence:** The 480×320 Raspberry Pi TFT is excluded from the MVP.

### D-002 — MQ135 is not CO₂

- **Status:** Accepted
- **Rule:** Publish only a raw or normalized relative gas/air-quality trend.
- **Forbidden labels:** `co2`, `co2_ppm`, `ppm`, or any health/safety claim derived from MQ135.

### D-003 — Separate power domains during the bench phase

- **Status:** Accepted in principle; exact wiring awaits RLS-01.
- **Rule:** ESP32 is powered through its confirmed USB input. SDS011 and MQ135 use a regulated
  external 5-V supply. Grounds are common; positive rails are not joined.

### D-004 — No mains control

- **Status:** Accepted
- **Reason:** The project is a measurement station, not a mains automation device.

## Open decisions

### OQ-001 — Firmware framework

- **Options:** ESPHome; PlatformIO with Arduino framework; ESP-IDF.
- **Decision trigger:** Complete RLS-01 and confirm sensor/display support and desired test depth.
- **Current leaning:** ESPHome for speed unless custom display/recovery behavior or testing needs
  justify PlatformIO.

### OQ-002 — Kubernetes packaging

- **Options:** Helm values over upstream charts; Kustomize; a small umbrella chart.
- **Decision trigger:** Capture the target cluster's ingress, storage, secret, DNS, and GitOps
  conventions in RLS-06.

### OQ-003 — Metrics path

- **Options:** Home Assistant history only; MQTT exporter to Prometheus; dedicated time-series
  storage.
- **Decision trigger:** Agree retention and dashboard needs before RLS-06 implementation.

### OQ-004 — MQ135 in MVP

- **Options:** Include as an experimental trend; omit until burn-in and safe ADC measurement are
  proven.
- **Decision trigger:** RLS-03 bench evidence.

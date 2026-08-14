# RLS-04 — Build the local display and device health UI

- **Status:** Blocked
- **Priority:** P1
- **Milestone:** M3 — Connected station
- **Depends on:** RLS-02, RLS-03

## User story

As a person in the room, I want to see useful measurements and health without a phone so that the
station remains informative even when the network is unavailable.

## Outcome

The mini OLED cycles through readable measurement and status views and clearly distinguishes
valid, stale, missing, and disconnected data.

## In scope

- PM view, room-climate view, and system/network health view.
- Automatic page rotation and sensible refresh rates.
- Failure and stale-state representation.

## Out of scope

- Large TFT, touch UI, graphical dashboard, and final enclosure.

## Acceptance criteria

- [ ] PM2.5, PM10, temperature, humidity, and pressure are readable at normal viewing distance.
- [ ] Wi-Fi, MQTT, and sensor health are visible without entering a debug mode.
- [ ] Stale or failed readings never appear as numeric zero.
- [ ] Display activity does not disrupt sensor sampling.
- [ ] The UI remains useful during a controlled Wi-Fi outage.

## Validation

- Automated: formatting/page-state tests where framework permits.
- Manual: photos of each page, unplugged-sensor test, and Wi-Fi outage demonstration.

## Evidence

Blocked by RLS-02 and RLS-03.

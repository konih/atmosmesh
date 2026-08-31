# ROOM-05 — Flip Room PIR so MQTT occupancy matches reality

- **Status:** Ready
- **Priority:** P0
- **Milestone:** Room variant field correction
- **Depends on:** Live `atmosmesh-room-v1` publishing `motion` over MQTT (already on the bench)

## User story

As the operator watching Room in Home Assistant / MQTT, I want `motion` / occupancy to read
detected when a person is present and clear when the room is empty, so that the binary sensor is
usable instead of inverted.

## Outcome

Room's published occupancy polarity matches observed presence. Idle room → `motion` false /
clear; person present → `motion` true / detected. TFT motion cell and serial `pir:` lines agree
with MQTT.

## In scope

- Confirm whether the live wiring is bare-module active-high, carrier `Q_PIR` active-low, or a
  module whose idle level is the opposite of what `kPirActiveLow` assumes today
  (`firmware/include/atmosmesh/room_pins.hpp`).
- Flip the effective polarity (build flag and/or default) so MQTT `motion.value` matches reality.
- Keep host tests for `pir_reading_is_motion` / contract payloads green for the chosen default.
- Document the live polarity in `wiring.md` / firmware README so the next carrier build does not
  re-invert by surprise.

## Out of scope

- Redesigning the NPN protection network on the perfboard
- Changing HA discovery device class or topic names
- SDS011 / buzzer behavior

## Acceptance criteria

- [ ] Given an empty room after PIR warmup, when MQTT state is read, then `motion` is false /
      clear (and the TFT / serial idle labels agree).
- [ ] Given a person in view of the PIR, when MQTT state is read, then `motion` is true / detected
      (TFT / serial agree).
- [ ] Given the polarity fix, when native MQTT/room tests run, then they encode the chosen
      `kPirActiveLow` default and still pass.
- [ ] Given `room_pins.hpp` / wiring notes, when an agent reads them, then they state which
      hardware configuration the default matches (bare module vs carrier with `Q_PIR`).

## Validation

- Automated: native tests around PIR polarity helper and room MQTT state payload.
- Manual: empty-room and walk-test observation of MQTT + serial.
- Failure/edge case: warmup window still may chatter — do not call warmup noise a polarity failure;
  re-check after `kPirWarmupMs`.

## Evidence

Operator, 2026-08-31: PIR appears flipped — MQTT shows person detected with nobody present, and
the reverse when someone is there.

## Notes

- Bring-up default is active-high (`kPirActiveLow == false`) because the carrier's inverting
  `Q_PIR` is not on the unbuilt perfboard. Live evidence now says that default is wrong for the
  installed module/wiring — fix the default or ship `-DATMOSMESH_ROOM_PIR_ACTIVE_LOW` as the
  canonical Room env flag, with tests locking the choice.

# ROOM-05 — Flip Room PIR so MQTT occupancy matches reality

- **Status:** Fix applied, flash/manual verification pending
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
      clear (and the TFT / serial idle labels agree). **Needs a flash + bench re-check** — not yet
      done this session (see Evidence: PlatformIO unavailable in this environment).
- [ ] Given a person in view of the PIR, when MQTT state is read, then `motion` is true / detected
      (TFT / serial agree). Same caveat as above.
- [ ] Given the polarity fix, when native MQTT/room tests run, then they encode the chosen
      `kPirActiveLow` default and still pass. **Not done** — `pir_reading_is_motion()` lives in
      the ESP32-only composition root, has no native test today, and none was added this session
      (see D-035's Consequence note); this remains an open gap, not a regression.
- [x] Given `room_pins.hpp` / wiring notes, when an agent reads them, then they state which
      hardware configuration the default matches (bare module vs carrier with `Q_PIR`). Both
      updated 2026-09-11 to record the live active-low finding and why it overrides the original
      theoretical assumption.

## Validation

- Automated: native tests around PIR polarity helper and room MQTT state payload.
- Manual: empty-room and walk-test observation of MQTT + serial.
- Failure/edge case: warmup window still may chatter — do not call warmup noise a polarity failure;
  re-check after `kPirWarmupMs`.

## Evidence

- Operator, 2026-08-31: PIR appears flipped — MQTT shows person detected with nobody present, and
  the reverse when someone is there.
- 2026-09-11: operator confirmed the Room board is plugged in and asked for the fix. Applied
  `-DATMOSMESH_ROOM_PIR_ACTIVE_LOW` to the canonical `[env:atmosmesh-room-v1]` in
  `firmware/platformio.ini` (env-level `build_flags` replaces rather than merges with `[env]`'s
  defaults in PlatformIO, so the full default flag set was repeated with the new macro added, not
  just the macro alone — dropping the others would have broken the build). Updated
  `room_pins.hpp` and `hardware/kicad/atmosmesh-room/wiring.md` to record the live polarity
  finding. Recorded as [D-035](../decisions.md).
- **Not done this session:** PlatformIO is not installed in this environment (only the
  `pyserial`/`esptool` bootstrap via `task bootstrap-agent-python`), so `task build-room` /
  `task flash-room` could not be run here to build, flash, or bench-verify the fix. The change is
  a one-line, high-confidence flip of an existing tested polarity-mapping function's compile-time
  input (`pir_reading_is_motion()` itself is unchanged), but it has not been built or run against
  the live board yet. Flash `atmosmesh-room-v1` and re-check MQTT/TFT/serial motion state before
  closing this story.

## Notes

- Bring-up default is active-high (`kPirActiveLow == false`) because the carrier's inverting
  `Q_PIR` is not on the unbuilt perfboard. Live evidence says that theoretical default was wrong
  for the specific installed module — fixed via the canonical env's build flag (D-035) rather than
  flipping the header default, so a future bare-board build with a differently-wired module still
  gets the originally-reasoned active-high assumption unless it too needs correcting from evidence.

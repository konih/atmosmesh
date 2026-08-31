# RLS-11 — Archive AtmosMesh v1 (`atmosmesh-0001`) after disassembly

- **Status:** Ready
- **Priority:** P1
- **Milestone:** Post-MVP retirement (does not unblock RLS-01–RLS-08 completion language; that MVP track is
  superseded by Room / Grove / Aqua as the live fleet)
- **Depends on:** Operator confirmation that the physical `atmosmesh-0001` bench station stays
  disassembled and will not be reassembled as the active v1 product

## User story

As the operator, I want AtmosMesh v1 / station id `atmosmesh-0001` retired from the live product
surface after the hardware was disassembled, so that docs, CI, Task targets and MQTT identity stop
implying a station that no longer exists, while the code history remains recoverable.

## Outcome

AtmosMesh v1 is explicitly archived: build/flash/docs paths stop treating it as a canonical live
product; remaining mentions either point at the archive or are deleted; Grove, Aqua and Room keep
building and publishing under their own identities.

## In scope

- Record the disassembly fact and archive intent in roadmap / decisions (short D-nnn).
- Stop advertising `atmosmesh-v1` / `atmosmesh-0001` as an active fleet member in README, Taskfile
  canonical product lists, and firmware README product tables (or mark them Archived).
- Decide and apply one archive shape: keep the composition root behind an `archive/` / opt-in env,
  or drop the env from default `build-all` / CI while retaining the source for archaeology.
- Sweep product mentions that would mislead a new agent into flashing or discovering v1 as live.
- Leave shared libraries that Grove/Aqua/Room still need in place; only retire the v1 *product*
  surface.

## Out of scope

- Deleting git history.
- Rewriting Grove / Aqua / Room MQTT contracts except where they incorrectly hard-depend on
  `atmosmesh-0001` / `atmosmesh-v1` as the only station.
- Rebuilding or recommissioning the disassembled board.

## Acceptance criteria

- [ ] Given the operator's disassembly note, when the archive decision is recorded, then
      `agent-context/decisions.md` states that AtmosMesh v1 / `atmosmesh-0001` is archived and why.
- [ ] Given default Task / CI / README product lists, when a reader looks for live products, then
      AtmosMesh v1 is absent or clearly marked Archived — not listed as a flash target alongside
      Grove, Aqua and Room.
- [ ] Given `task build-all` (or the repo's equivalent default matrix), when it runs, then it does
      not require a successful `atmosmesh-v1` build unless an explicit archive/opt-in target is
      invoked.
- [ ] Given a grep for `atmosmesh-0001` / canonical "live station" language, when the sweep is done,
      then remaining hits are historical, test fixtures for shared contract code, or behind the
      archive path — none instruct an agent to commission v1 as current hardware.
- [ ] Given Grove, Aqua and Room, when the archive lands, then their builds, native tests and MQTT
      identities still pass unchanged.

## Validation

- Automated: native tests + the remaining live product builds; optional archive env build if kept.
- Manual: README / Taskfile / ADR-0001 product matrix read-through for stale "live v1" claims.
- Failure/edge case: shared `mqtt_contract` / profile tables still compile for live products when
  v1 literals move or are gated.

## Evidence

Operator, 2026-08-31: physical AtmosMesh v1 / `atmosmesh-0001` has been disassembled; archive the
code and mentions.

## Notes

- Station id in firmware/docs is `atmosmesh-0001` (not `atmosmesh001`); treat operator shorthand as
  that identity.
- Adversarial note: "delete everything v1" would break shared tests and ADR-0001's composition
  story. Prefer retire-the-product over delete-the-libraries.

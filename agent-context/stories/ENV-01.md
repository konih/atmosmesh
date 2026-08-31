# ENV-01 — Grove and Aqua enclosure heat bias (recalibrate vs relocate/replace)

- **Status:** Ready
- **Priority:** P1
- **Milestone:** Field correction (Grove v1.5 + Aqua)
- **Depends on:** Serial or MQTT captures of the elevated readings; access to open the housings

## User story

As the operator reading Grove and Aqua temperatures, I want the published values to reflect room or
tank conditions rather than trapped heat inside the enclosure, so that Home Assistant history and
any downstream automation are not systematically high.

## Outcome

A recorded root cause (sensor sitting in stagnant enclosure air vs faulty/wrong part vs missing
offset) and one chosen remedy: relocate/vent the sensor, replace the sensor, or — only as an
explicit interim — publish a documented enclosure offset without pretending the raw number is true
ambient.

## In scope

- Capture paired evidence: enclosed reading vs a reference just outside the housing (same moment)
  for Grove (DHT11 path) and Aqua (SHT41 path).
- Decide per product: move sensor outside / vent housing, replace the part, or apply a clearly
  labeled software offset with a decision entry.
- Update docs / MQTT entity naming or attributes if an offset is used so HA does not present the
  value as unbiased ambient.
- Keep Grove and Aqua product identities and contracts otherwise unchanged.

## Out of scope

- inventing a single magic offset without a reference measurement
- changing Room's SHT41 path in this story (separate product; open a ROOM story if it shows the
  same bias)
- soil / water-probe calibration work

## Acceptance criteria

- [ ] Given Grove inside its housing, when a simultaneous outside-housing reference is taken, then
      the delta and method are recorded under Evidence (with units and rough air conditions).
- [ ] Given Aqua inside its housing, when the same paired check is done, then its delta is recorded
      the same way.
- [ ] Given the deltas, when a remedy is chosen, then `decisions.md` states relocate/vent vs replace
      vs interim offset — and rejects silent "recalibration" that hides enclosure bias as if it were
      sensor trim.
- [ ] Given the chosen remedy is implemented, when MQTT/OLED are observed, then temperatures are no
      longer systematically high relative to the agreed reference (or, if offset remains, the entity
      documents that fact).
- [ ] Given failure to get a trustworthy reference, when the story cannot close, then it stays Ready
      with the blocker named rather than shipping a guessed offset.

## Validation

- Automated: any new offset/label helpers get native tests; product builds stay green.
- Manual: before/after MQTT or serial temperature vs reference.
- Failure/edge case: sensor still high after relocation → treat as part failure / wrong sensor, not
  another firmware fudge.

## Evidence

Operator, 2026-08-31: Grove and Aqua temperatures are rather high; likely cause is sensors inside
the housing. Remedy options named: recalibrate or switch the sensor. (Prefer relocate/vent before
"recalibrate".)

## Notes

- Challenging the ask: a housing-induced rise is not fixed by soil-style calibration constants. An
  offset is a known lie unless labeled; moving the sensor or opening airflow is the honest fix.
- Grove uses DHT11 (±2 °C class, slow); Aqua uses SHT41 (far better). If only Grove is wild after
  venting, prefer replacing DHT11 rather than teaching firmware to invent precision.

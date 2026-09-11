# AtmosMesh operator board

## In flight

| Lane | Owner | Branch | Scope |
| --- | --- | --- | --- |
| ROOM-01 protected room carrier | Codex implementer | `codex/atmosmesh-room-carrier` | Provisional 60×80 mm KiCad carrier, validation, and safety documentation; no fabrication approval before exact module photos |

## PR open

| Lane | PR | State | Hardware follow-up |
| --- | --- | --- | --- |
| Dependency update | #15 | Open, separate existing lane | None |
| Hantek guide | #14 | Existing draft, separate lane | None |

## Integrated

| Lane | PR | Main | Remaining hardware follow-up |
| --- | --- | --- | --- |
| JIG-01 Bluetooth mouse jiggler | [#16](https://github.com/konih/atmosmesh/pull/16) | `6ea2387` | Operator confirmed Mac pairing 2026-09-11 |
| Grove OLED/LED diagnostic + calibrated soil-status policy | [#4](https://github.com/konih/atmosmesh/pull/4) | `6643422` | Operator supplied dry/wet thresholds 2026-08-26 (D-021); operator visual confirmation of LED colour still owed |
| AtmosMesh multi-product + Grove v1.5 | [#3](https://github.com/konih/atmosmesh/pull/3) | `797aabd` | OLED pixels, visible LED colour, electrical behavior and broker/HA receipt |


## RESULTS — JIG-01 pairing-fix independent review

### REVIEW — atmosmesh/JIG-01 pairing fix @ 6990ed7 — APPROVE — 2026-09-11
**Verdict:** APPROVE (software; Mac pairing still unverified on hardware)

| Gate | Result | Notes |
| --- | --- | --- |
| `task test` | ✅ | 182/182 native (13 jiggler tests) |
| `task build-jiggler` | ✅ | C3: 514322 flash / 24772 RAM |
| `task check` | ✅ | Clean at `6990ed7` |
| Hardware acceptance | ⚠️ | Board not attached this session; Mac retest owed after flash |

| ID | Sev | Area | Finding | Evidence path:line | Blocks? |
| --- | --- | --- | --- | --- | --- |
| F1 | P2 | TDD | Map test is a byte-pair scan, not full packing; 3-byte send tests lock the layout | firmware/test/test_jiggler/test_jiggler.cpp:102 | No |
| F2 | P3 | Quality | `startSecurity` return value discarded; disconnect reason is logged | firmware/src/products/atmosmesh_jiggler.cpp:24 | No |
| F3 | P3 | TDD | Test name still says “only X/Y” after the leading button byte | firmware/test/test_jiggler/test_jiggler.cpp:120 | No |

**Functional correctness:** Standard 3-button relative mouse (buttons always 0), PnP ID, Just Works bonding without Secure Connections. Report bytes match the descriptor. No clicks/keyboard.

**Summary / next steps:** Clear to keep on PR #16. Operator: forget stale **AtmosMesh Jiggler**, flash `6990ed7`, retry pairing. OLED should show `BT connected`. This verdict does not accept hardware.

## RESULTS — JIG-01 final independent review

### REVIEW — atmosmesh/JIG-01 @ 65ff145 — APPROVE — 2026-09-10
**Verdict:** APPROVE (software review; hardware acceptance remains pending)

| Gate | Result | Notes |
| --- | --- | --- |
| `task test` | ✅ | Fresh run: 181/181; 12 jiggler tests |
| `task build-jiggler` | ✅ | Fresh C3 build: 513908 flash / 24804 RAM |
| `task check`, diff whitespace | ✅ | Clean |
| Readiness mutation | ✅ | Replacing `ready()` with `true` fails partial-connection test at test_jiggler.cpp:170 |
| Policy coverage | ✅ | gcov: 90% lines, 96.55% branches executed; no repo coverage floor |
| Secret/diff review | ✅ | No credentials, generated binaries, or personal network settings in change |
| Hardware acceptance | ⚠️ | Mac pairing, OLED, physical button and idle behavior pending |
| Go/workshop-specific gates | ⚠️ | Not applicable to this PlatformIO firmware repo |

| ID | Sev | Area | Finding | Evidence path:line | Blocks? |
| --- | --- | --- | --- | --- | --- |
| — | — | — | No remaining blocking findings. R1 transport tests and R2 readiness-call-site regression resolved. | firmware/test/test_jiggler/test_jiggler.cpp:160 | No |

**Functional correctness:** Delivers standalone C3 BLE HID X/Y motion with OFF boot, debounced toggle, encryption/subscription gating, disconnect/unsubscribe latch, send-failure disarming and paired bounded timing. Docs describe wiring, pairing and USB limitations. Actual NimBLE interfaces checked against pinned source; descriptor contains only relative X/Y. Physical acceptance remains unverified.

**Coverage delta:** New policy: absent → 90% measured lines (gcov template-instantiation accounting); baseline percentage unavailable. Existing native suites remain green. New assertions cover readiness, notification bytes/counts, failure disable and fast reconnect. Readiness bypass demonstrably turns the suite red.

**Summary / next steps:** No software changes required by this review. Complete documented hardware checks before marking the story done. This verdict does not authorize merge or perform a flash.

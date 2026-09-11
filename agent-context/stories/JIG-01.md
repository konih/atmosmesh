# JIG-01 — standalone Bluetooth mouse jiggler

Status: Done. Merged to main `6ea2387` 2026-09-11. Operator confirmed Mac pairing.

Goal: Spot-type C3 SuperMini OLED as togglable mouse jiggler for Mac. Minimum BLE mouse,
momentary button toggle and OLED state. Optional MQTT, keyboard and larger motions deferred
until this minimum is physically verified. No clicks: unwanted input risk.

Acceptance: starts disabled, debounced press toggles once, disconnect disables, minimal paired
relative motion only while connected/enabled, OLED distinguishes pairing/off/on, documentation
covers button wiring, pairing and USB limitation. No other AtmosMesh product behavior changes.

Validation: initial test failed missing jiggler header; seven policy cases pass; complete native
suite initially 176/176, now 180/180; C3 build succeeds (513848 flash, 24812 RAM). Review R1 requested BLE transport policy tests; added JigglerHid encryption/subscription gating, disconnect latch, failed-send disable and X/Y frame tests. Fresh review pending.
Chip identified on /dev/ttyACM0 as ESP32-C3 revision 0.4, embedded 4 MB flash. Pre-flash backup
requested to /tmp/atmosmesh-jiggler-before.bin. Mac/OLED/button acceptance still pending.

Worktree: /home/koni/Projects/PlatformRelay/worktrees/atmosmesh/ble-jiggler, feat/ble-jiggler.
Implementation worker hit a service usage limit after tests; coordinator salvaged implementation,
with separate reviewer dispatched. No self-approval.


Update: final independent APPROVE at 65ff145, 181/181 tests. Firmware flashed successfully;
startup serial `jiggler: ready oled=ok button=GPIO3 boot=GPIO9 mode=off`. BLE advertising verified
as AtmosMesh Jiggler, HID service 0x1812, appearance 0x03c2 input-mouse. PR #16 open, not merged.
Mac pairing/movement and visible OLED/button acceptance requested from operator.

Operator confirmed Mac connected (2026-09-10). BOOT toggle, visible motion and idle prevention still awaiting confirmation.

Correction: operator reports Mac could NOT properly pair. Previous connection confirmation withdrawn. Diagnose before accepting hardware.

Pairing-fix follow-up 2026-09-11 at 6990ed7: standard 3-button relative HID map (buttons always
released), PnP ID, Just Works bonding without Secure Connections, connect/auth/disconnect serial
logs. Native tests 182/182; C3 build 514322 flash / 24772 RAM.

Merged to main as `6ea2387` 2026-09-11. Flashed `/dev/ttyACM0`: `jiggler: ready oled=ok … mode=off`
and advertising AtmosMesh Jiggler. Operator confirmed Mac pairing and jiggler operation 2026-09-11.

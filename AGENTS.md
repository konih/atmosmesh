# AGENTS.md

Canonical instructions for humans and coding agents working on AtmosMesh.

> Read `agent-context/README.md` and the active story before changing files. Live status belongs
> there; keep this file stable.

## Mission

Build a safe, reproducible ESP32 indoor air-quality station that publishes trustworthy,
well-labelled measurements to a Kubernetes-hosted home-automation stack.

## Current boundary

- First active story: `RLS-01` hardware identification and wiring approval.
- Firmware bring-up for the bench I²C SSD1306 OLED on GPIO5/GPIO4 is in `firmware/` (D-001).
  Do not energise SDS011/MQ135/mains until RLS-01 wiring is approved.
- Do not deploy to a Kubernetes cluster without explicit user authorization and known context.

## Mandatory electrical safety rules

- ESP32 GPIOs and the `3V3` pin must never receive 5 V.
- A devboard may receive 5 V only through a confirmed `USB` or `VIN/5V` input.
- Do not infer connector orientation, voltage, or pin order from a generic product image.
- Confirm the exact front and back markings of every module before approving wiring.
- SDS011 (Nova PM; board often labelled d011v2) is **UART2 only**: sensor TX → GPIO16 (RX2), GPIO17 (TX2) → sensor RX. **Never GPIO1/TX0 or GPIO3/RX0** — those are the USB-UART (CP2102) for `task flash` / `task monitor`. Do not move SDS011 firmware to UART0.
- SDS011 and MQ135 take 5 V power; their *signals* must stay at 3.3 V (UART TTL, or a measured ADC divider). MQ135 is analog (GPIO34 via divider), not UART.
- Protect ESP32 ADC inputs with a verified divider and measure the node before connection.
- MQ135 is not a CO₂ sensor. Never label its output `co2`, `ppm`, or an equivalent claim.
- Open mains PCBs, breadboard-mains, mains switching, and life-safety automation are out of scope. An enclosed, isolated AC/DC module as the station 5 V source is allowed only after the DC output is measured at ~5 V (`docs/hardware/power.md`).

## Sources of truth

| Concern | Source |
| --- | --- |
| Live work and blockers | `agent-context/README.md` |
| Story order/status | `agent-context/roadmap.md` |
| Acceptance contract | `agent-context/stories/RLS-*.md` |
| Decisions and open questions | `agent-context/decisions.md` |
| Confirmed hardware facts | `docs/hardware/inventory.md` |
| Power architecture | `docs/hardware/power.md` |
| Manufacturer datasheets / spec comparison | `docs/hardware/datasheets/`, `docs/hardware/spec-comparison.md` |
| System boundaries | `docs/architecture.md` |

If documents disagree, stop and reconcile them. Electrical safety rules above always win.

## Repository map

- `firmware/`: ESP32 source and device configuration after the framework ADR.
- `deploy/`: declarative Kubernetes and application configuration.
- `tests/`: host-side tests, config validation, and end-to-end checks.
- `docs/`: product and operator documentation.
- `agent-context/`: committed planning and handoff material, not user-facing product docs.

## Workflow

1. Claim a ready story by updating its status in `agent-context/roadmap.md`.
2. Write or update the story validation plan before implementation.
3. Prefer a failing automated test first when software behavior is involved.
4. Make the smallest change that satisfies the acceptance criteria.
5. Run the applicable gates and record evidence in the story.
6. Add a dated handoff under `agent-context/handoffs/` when work spans sessions.
7. Mark a story done only when every required criterion has evidence.

Do not start optional RLS-09 or RLS-10 while an MVP story is ready unless the user explicitly
reprioritizes the roadmap.

## Commands

From the repository root (`Taskfile.yml`). Override the serial port with `ESP_PORT=/dev/…`.

**Host Python for flash/monitor:** never run `python -m esptool` or `pip install` with Homebrew
`python3` (currently 3.14), Xcode `/usr/bin/python3`, or a random `python`. Those interpreters
do not provide `pyserial` (import name `serial`) or `esptool` on PATH. Use `task flash` /
`task monitor` (they prepend the shared venv) or `scripts/esp-tool`.

Host venv resolution (`scripts/with-agent-python`): `ATMOSMESH_VENV` if it exists, else
`<clone>/.venv`, else a workspace `.tooling/python/atmosmesh` found by walking parents.
Prefer `python3.12` or `python3.11`, not Homebrew `python3`.

```bash
task bootstrap-agent-python
# or: python3.12 -m venv .venv && .venv/bin/pip install -r firmware/requirements-agent.txt
```

Direct esptool: `./scripts/esp-tool version` (or `./scripts/with-agent-python python -m esptool …`).
Keep using Homebrew `pio` for firmware builds; do not install a second PlatformIO into the venv.

| Task | Command |
| --- | --- |
| List tasks | `task` |
| Bootstrap host venv | `task bootstrap-agent-python` |
| Host unit tests | `task test` |
| Build ESP32 image | `task build` |
| Flash | `task flash` (OLED SDA is GPIO5 / SCL GPIO4) |
| Serial monitor | `task monitor` |
| Flash then monitor | `task run` / `task flash-monitor` |
| Direct esptool | `./scripts/esp-tool` (not system `python -m esptool`) |
| Check whitespace | `task check` |
| Clean ESP32 build | `task clean` |
| Inspect repository state | `git status --short` |

## Conventions

- Keep secrets out of the repository. Commit examples only as `.example` files.
- Use stable MQTT names with explicit units; distinguish missing/stale values from numeric zero.
- Prefer ADC1 pins for analog measurements while Wi-Fi is active.
- Configuration and manifests must be declarative and reproducible.
- Keep generated build output out of Git.
- Use Conventional Commits when the user asks for commits: `type(scope): summary`.
- Never modify Git configuration or add an AI tool as author/co-author/trailer.
- Preserve user changes and avoid unrelated refactors.

## Completion gates

- Story acceptance criteria and validation evidence are complete.
- Relevant automated checks pass.
- No secret, credential, personal network detail, or generated binary is staged.
- Documentation matches actual behavior and wiring.
- Power-loss, sensor-loss, and network-loss behavior is tested when affected.

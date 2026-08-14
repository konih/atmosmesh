# AGENTS.md

Canonical instructions for humans and coding agents working on AtmosMesh.

> Read `agent-context/README.md` and the active story before changing files. Live status belongs
> there; keep this file stable.

## Mission

Build a safe, reproducible ESP32 indoor air-quality station that publishes trustworthy,
well-labelled measurements to a Kubernetes-hosted home-automation stack.

## Current boundary

- First active story: `RLS-01` hardware identification and wiring approval.
- Do not implement firmware or energize a proposed circuit until RLS-01 is complete.
- Do not deploy to a Kubernetes cluster without explicit user authorization and known context.
- Firmware framework and deployment packaging are pending decisions, not assumptions.

## Mandatory electrical safety rules

- ESP32 GPIOs and the `3V3` pin must never receive 5 V.
- A devboard may receive 5 V only through a confirmed `USB` or `VIN/5V` input.
- Do not infer connector orientation, voltage, or pin order from a generic product image.
- Confirm the exact front and back markings of every module before approving wiring.
- SDS011 and MQ135 may use a separate regulated 5-V supply; signal grounds must be common.
- Protect ESP32 ADC inputs with a verified divider and measure the node before connection.
- MQ135 is not a CO₂ sensor. Never label its output `co2`, `ppm`, or an equivalent claim.
- Mains voltage, open mains relays, and life-safety automation are out of scope.

## Sources of truth

| Concern | Source |
| --- | --- |
| Live work and blockers | `agent-context/README.md` |
| Story order/status | `agent-context/roadmap.md` |
| Acceptance contract | `agent-context/stories/RLS-*.md` |
| Decisions and open questions | `agent-context/decisions.md` |
| Confirmed hardware facts | `docs/hardware/inventory.md` |
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

The implementation toolchain is not selected yet. Until that decision:

| Task | Command |
| --- | --- |
| Check whitespace and patch integrity | `git diff --check` |
| Inspect repository state | `git status --short` |
| Review planned work | `sed -n '1,240p' agent-context/README.md` |

When a firmware or deployment toolchain is chosen, add reproducible install, build, lint, test,
and validation commands here and in the root README in the same change.

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

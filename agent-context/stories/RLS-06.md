# RLS-06 — Deploy the home-automation services to Kubernetes

- **Status:** Blocked
- **Priority:** P1
- **Milestone:** M4 — Platform integration
- **Depends on:** RLS-05 and documented cluster context

## User story

As the platform operator, I want the required services deployed declaratively so that the station
does not depend on an undocumented, manually configured server.

## Outcome

Mosquitto, Home Assistant, Grafana, and the agreed metrics path are reproducibly deployed with
persistence, health checks, and secrets outside versioned manifests.

## In scope

- Record namespace, storage, ingress/DNS, secret, network, and GitOps constraints.
- Decide deployment packaging OQ-002 and metrics path OQ-003.
- Reproducible render/validation and documented install/rollback procedures.

## Out of scope

- Public internet exposure by default, unrelated cluster applications, and destructive cluster changes.

## Acceptance criteria

- [ ] Cluster constraints and deployment decisions are documented before manifests are added.
- [ ] All workloads are declarative and pass local render/schema validation.
- [ ] Configuration and required history survive a controlled pod restart.
- [ ] No secret appears in committed files or rendered test artifacts.
- [ ] Services expose meaningful readiness/liveness checks where supported.
- [ ] ESP32 can reach a stable broker address from the home network.
- [ ] Install, upgrade, rollback, and recovery steps are documented.

## Validation

- Automated: manifest render, schema/lint, and secret-pattern checks.
- Manual: controlled deployment with explicit authorization, pod restart, persistence, and broker reachability.

## Evidence

Blocked by RLS-05 and missing cluster context.

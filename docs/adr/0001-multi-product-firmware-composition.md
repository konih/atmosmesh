# ADR-0001: Multi-product firmware composition

- **Status:** Accepted after independent review
- **Date:** 2026-08-24
- **Owners:** AtmosMesh maintainers

## Context

AtmosMesh now has two first-class firmware products. AtmosMesh v1 is the existing full ESP32
station; AtmosMesh Grove v1.5 is an additional compact ESP8266 node. They share measurement and
presentation concepts but have different controllers, sensors, display geometry and lifecycle.
Keeping Grove as a copy of the ESP32 application would create drift. Hiding both applications in
one runtime entrypoint behind preprocessor branches would make composition and testing opaque.

The existing ESP32 runtime is working bench software and must not be behaviorally rewritten merely
to make the directory tree symmetrical.

## Decision

Use **one PlatformIO project** with four explicit layers:

1. Product composition roots live in `firmware/src/products/` and contain Arduino `setup()` /
   `loop()` plus target wiring. Each firmware environment selects exactly one composition root.
2. Compile-time product profiles in `firmware/include/atmosmesh/product_profile.hpp` define stable
   identity and board facts one-to-one with product builds. They are compile-time declarations, not
   runtime selection branches. Grove consumes its profile now; the unchanged v1 root can adopt its
   profile fields incrementally without a big-bang behavior refactor.
3. Shared modules remain under `firmware/src/` and `firmware/include/atmosmesh/`. Logic that does
   not require hardware is host-testable in the single `native` environment.
4. Canonical PlatformIO environments and Task targets use product names. Compatibility aliases
   remain while existing operator/CI workflows migrate.

The two product contracts are:

| Product | Stable product ID | Product variant | Default station ID | Composition root | Canonical environment |
| --- | --- | --- | --- | --- | --- |
| AtmosMesh v1 | `atmosmesh-v1` | `esp32-full-station` | `atmosmesh-0001` | `products/atmosmesh_v1.cpp` | `atmosmesh-v1` |
| AtmosMesh Grove v1.5 | `atmosmesh-grove-v1.5` | `atmosmesh-v1.5` | `atmosmesh-grove-0001` | `products/atmosmesh_grove_v1_5.cpp` | `atmosmesh-grove-v1_5` |

Product IDs are stable API identifiers and are not room names. Product variants describe the
hardware/firmware shape within that contract; product ID, product variant and station ID are
separate values. The `v1` / `v1.5` suffix in a product ID identifies a hardware/product contract,
not a repository release number. Backward-compatible firmware fixes do not change it. A new suffix
is required when wiring, required hardware, persisted protocol, or operator expectations become
incompatible. Software releases may use independent semantic tags in the future without changing
product IDs.

A future variant is added by:

1. proposing/accepting the product contract and safe wiring;
2. adding one compile-time profile and one product composition root;
3. composing existing shared modules or adding host-tested shared behavior;
4. adding one canonical PlatformIO environment and Task build/flash/monitor targets;
5. adding it to the product matrix and validation gates.

Do not add product selection branches throughout shared modules. A small compile-time adapter at a
composition boundary is acceptable when an API genuinely differs between frameworks.

## Alternatives considered

### Copy one complete firmware tree per product

Rejected. It makes the first variant quick but duplicates sensor validity, formatting and future
network behavior. Fixes would need to be applied and verified repeatedly.

### One entrypoint with pervasive `#ifdef` product branches

Rejected. It hides each product's hardware composition, permits impossible mixed configurations,
and makes local reasoning and native coverage progressively harder.

### Separate repositories

Rejected for now. The products share maintainers, domain contracts, tests and release gates; the
coordination cost is larger than the isolation benefit at this scale.

### Fully refactor the ESP32 runtime before adding Grove

Deferred. It offers symmetry but adds risk unrelated to Grove. Moving the unchanged ESP32 file into
an explicit composition-root location establishes the boundary; shared extraction can happen in
small, tested stories later.

## Consequences

- Both products are obvious and independently buildable without cloned applications.
- Native utilities compile/test once, while device builds compose only their intended root.
- Product identity, build names and compatibility promises become explicit.
- PlatformIO filters are part of the architecture and must be reviewed when files move.
- The ESP32 composition root remains large initially. This is acknowledged migration debt, not a
  claim that the v1 runtime is already thin/profile-driven.
- Compatibility environments/tasks add temporary maintenance surface until consumers migrate.
- Product-specific network transports may differ (ESP32 `esp_mqtt`, ESP8266 PubSubClient), while
  identity, topics, discovery payload construction and reconnect/publish sequencing stay shared and
  host-tested. This is composition at the transport boundary, not duplicated product contracts.

## Migration

This proposal moves the existing ESP32 and Grove entrypoints into `firmware/src/products/` without
rewriting their behavior. `esp32dev`, `esp8266-grove`, `task build`, and `task build-grove` remain
compatibility aliases. New work and documentation use `atmosmesh-v1`,
`atmosmesh-grove-v1_5`, `task build-v1`, and `task build-v1-5`.

The ESP32 root may consume more profile fields and shed shared logic later through separate TDD
stories. No big-bang rewrite is required to accept this ADR.

## Validation

- `task test` compiles shared host-testable utilities once and excludes `src/products/*`.
- `task build-v1` and `task build-v1-5` each build one canonical product.
- `task build-all` builds both canonical products.
- Compatibility `task build` and `task build-grove` remain green.
- PlatformIO verbose/source-filter inspection confirms each device environment selects exactly one
  product composition root.
- `task check` and `git diff --check` pass; no flash is required to validate this structural ADR.

### Evidence — 2026-08-24

- Independent review approved the product composition, filters, compatibility aliases and ADR at
  head `a681990` before hardware upload.
- The full native/canonical/compatibility gate matrix passed as recorded in V15-02.
- The coordinator then built/flashed the canonical Grove target and observed display name
  `AtmosMesh Grove`, variant `atmosmesh-v1.5` and station ID `atmosmesh-grove-0001`. That reviewed
  head did not emit the stable product ID separately. A follow-up test/build fixed the contract, and
  a second reviewed flash of final head `50ca2f3` captured
  `product_id=atmosmesh-grove-v1.5` alongside the distinct variant and station ID. AtmosMesh v1 and
  its ESP32 were untouched. Hardware sensor results belong to V15-03, not this ADR.

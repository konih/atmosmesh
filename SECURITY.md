# Security Policy

AtmosMesh is a bench-stage ESP32 air-quality station. Firmware talks to
sensors over I²C, UART, and ADC; later work will publish MQTT. Treat it as
experimental hardware, not a safety instrument.

## Reporting a vulnerability

Report suspected vulnerabilities **privately** — do not open a public issue.

- Use **GitHub Security Advisories** → *Report a vulnerability* on this
  repository, or email **konrad.heimel@gmail.com**.
- Include the commit or tag, what you found, how to reproduce it, and impact.
- This is a personal project without a formal SLA. Confirmed issues get a fix
  on `main` before any public write-up.

## Supported versions

Only `main` receives security fixes. There is no stable release yet.

## What this repo must not contain

- Wi-Fi passwords, broker credentials, or TLS private keys
- Live kubeconfigs or cloud tokens
- Photos of mains wiring that would let someone copy an unsafe layout as if it
  were approved

Examples belong in `*.example` files. Hardware pin maps in `docs/` and
`firmware/` are intentional and public.

## Scanning

GitHub **CodeQL** (default setup: Actions + C/C++), **secret scanning** with
push protection, **Dependabot** alerts and security updates, and **private
vulnerability reporting** are enabled on this repository.

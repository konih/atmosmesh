# Tests

The test strategy will be finalized with the firmware framework. It must cover, where applicable:

- sensor parsing and invalid/stale states;
- MQTT payload schema and units;
- reconnect and Last Will behavior;
- configuration/schema validation;
- rendered Kubernetes manifests;
- an end-to-end synthetic measurement from device boundary to dashboard/alert;
- documented manual electrical checks and the 30-minute/48-hour run evidence.

Hardware measurements are evidence, not a replacement for host-side automated tests.

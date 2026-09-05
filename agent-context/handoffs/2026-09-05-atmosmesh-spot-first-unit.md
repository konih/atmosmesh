# AtmosMesh Spot first unit handoff — 2026-09-05

- Status: In flight (SP-02 image running on the soldered unit; SP-01 acceptance tests pending)
- Done:
  - `atmosmesh-spot-v1` PlatformIO environment (ESP32-C3, Arduino core 3.x) and composition root
    `firmware/src/products/atmosmesh_spot_v1.cpp`; merged as PR #12, radar fixes as PR #13.
  - All four sensors read on the soldered carrier; presence from the radar's `OT2`; distance and
    raw target state from its UART; OLED pages; MQTT as `atmosmesh-spot-0001` with 8 discovered
    entities; host tests 169 cases.
  - Hi-Link's LD2410S Operation Guide filed under `docs/hardware/datasheets/`.
- Evidence: SP-01 and SP-02 "Evidence" sections; serial captures quoted there.
- Blocked by: nothing. The operator's Wi-Fi link at the bench sat at −90 dBm in the last runs and
  the board did not associate within a minute; earlier runs at −72 dBm connected and published.
- Next:
  1. Empty-room test (SP-01): the radar reported a target at ~71 cm for minutes after the operator
     said they would leave. If that is a fixed reflection, run the LD2410S threshold auto-generation
     (protocol command `0x0009`) with the room empty — SP-04 — or raise gate-1 thresholds.
  2. Side-by-side with Room for an hour (SP-01): the SHT41 reads ~3 K above the DS18B20 probe on
     this carrier and climbs as the board warms; decide lead, cut-out or offset.
  3. Home Assistant check of the eight entities and the unavailable-on-unplug behaviour.
  4. OLED orientation in the socket; set `ATMOSMESH_SPOT_OLED_ROTATION` if upside down.
  5. Consider a 10 kΩ pull-up from `OT1` to 3V3 on the carrier for the next unit, so the radar's
     weak TX driver does not depend on the C3's internal pull-up (see wiring.md 5.3).
- Do not:
  - Give U8g2 the I²C pin numbers on Arduino core 3.x (the OLED init hangs the bus).
  - Leave a pull-down on the radar RX pin, or read the radar's `OT1` against one and conclude
    "nothing driving it" — the driver is weak and follows the pull.
  - Let IDF logging run on UART0: it feeds the radar's RX and the module answers with ACKs.
  - Use `readLux(VEML_LUX_AUTO)` on this product: 5 s per read in the dark.

## Retrospective

What took the time, in order, and what would have cut it:

1. **The OLED-init hang (about 30 min).** U8g2 with pin numbers detaches the I²C peripheral on
   core 3.x. Found by bisecting with two extra log lines; the first flash had no per-stage
   timing, so the hang looked like a dead board. Lesson kept: every setup stage logs how long it
   took, and the 5 s status line carries the slowest loop stage.
2. **The radar UART (about 3 h, two sessions).** Four separate causes stacked on one line:
   a genuinely open `OT1` (fixed by the operator's reflow), the C3's internal pull-down left on the
   RX pin by the wiring check (the mux does not touch pulls), the module needing a break on its RX
   to start streaming after a reset, and IDF log output on UART0 waking the module's parser. Each
   fix revealed the next. The pin-edge probe and the "first bytes on RX" dump were the two
   diagnostics that produced facts instead of theories; the enable-config ACK appearing at every
   reset was the clue that the transmitter worked at all. What would have been faster: reading
   the RX line against both pulls from the first diagnostic instead of one, and trusting the
   Operation Guide's photo earlier — its pad column is the visible sign of the weak driver.
3. **The VEML7700 auto-range (found by accident).** The loop-stage timer exposed a 5 s read in the
   dark. A fixed gain with a no-wait read costs nothing and removes it; without the timer this
   would have surfaced as "the button sometimes does not work".
4. **What went well.** The host-tested decoder handled the real byte stream first time, including
   the ACK frames and the look-alike distance bytes. The contract-parametrised MQTT runtime meant
   the Spot published on its own topics with no copy of the Room file. The bring-up followed the
   story's staged plan: scan, then image, then one line at a time.
5. **Process.** Three PRs, all rebase-merged the same night; the datasheet folder got the guide
   the operator found rather than another download attempt. One miss: the first commit's README
   said the `OT1` path was open when the real fault was already partly ours (the pull-down); the
   handshake PR corrects it, and the stories keep both readings so the trail is honest.

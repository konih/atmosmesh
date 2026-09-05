#pragma once

namespace atmosmesh {

// Presence from a radar's occupancy pin: a debounce against edge noise on the header, then a
// hold so that a single dropped detection does not flip the entity off and on again. The radar's
// own "no one" delay does the real smoothing; this only keeps the firmware from adding jitter.
struct PresenceHold {
    bool occupied = false;
    bool stable_high = false;
    bool pending_high = false;
    bool have_sample = false;
    unsigned long pending_since_ms = 0;
    unsigned long last_high_ms = 0;
    unsigned long occupied_since_ms = 0;
};

// Returns true when `occupied` changed on this call.
bool presence_hold_update(PresenceHold& hold, bool raw_high, unsigned long now_ms,
                          unsigned long debounce_ms, unsigned long hold_ms);

}  // namespace atmosmesh

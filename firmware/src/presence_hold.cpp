#include "atmosmesh/presence_hold.hpp"

namespace atmosmesh {

bool presence_hold_update(PresenceHold& hold, bool raw_high, unsigned long now_ms,
                          unsigned long debounce_ms, unsigned long hold_ms) {
    if (!hold.have_sample) {
        hold.have_sample = true;
        hold.pending_high = raw_high;
        hold.pending_since_ms = now_ms;
        hold.stable_high = raw_high;
    } else if (raw_high != hold.pending_high) {
        hold.pending_high = raw_high;
        hold.pending_since_ms = now_ms;
    } else if (now_ms - hold.pending_since_ms >= debounce_ms) {
        hold.stable_high = hold.pending_high;
    }

    const bool was_occupied = hold.occupied;
    if (hold.stable_high) {
        hold.last_high_ms = now_ms;
        if (!hold.occupied) {
            hold.occupied = true;
            hold.occupied_since_ms = now_ms;
        }
    } else if (hold.occupied && now_ms - hold.last_high_ms >= hold_ms) {
        hold.occupied = false;
    }
    return hold.occupied != was_occupied;
}

}  // namespace atmosmesh

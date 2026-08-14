#include "atmosmesh/digital_edge.hpp"

namespace atmosmesh {

bool update_debounced_level(DebouncedLevel& state, bool sample, std::uint32_t now_ms,
                            int debounce_ms) {
    if (!state.has_stable) {
        state.has_stable = true;
        state.stable = sample;
        state.has_candidate = false;
        return true;
    }
    if (sample == state.stable) {
        state.has_candidate = false;
        return false;
    }
    if (!state.has_candidate || state.candidate != sample) {
        state.has_candidate = true;
        state.candidate = sample;
        state.candidate_since_ms = now_ms;
        return false;
    }
    const std::uint32_t elapsed = now_ms - state.candidate_since_ms;
    if (elapsed < static_cast<std::uint32_t>(debounce_ms)) {
        return false;
    }
    state.stable = sample;
    state.has_candidate = false;
    return true;
}

std::string format_pir_log(bool motion) {
    return motion ? "pir: motion" : "pir: idle";
}

std::string format_beep_boot_log() {
    return "beep: boot";
}

}  // namespace atmosmesh

#include "atmosmesh/digital_edge.hpp"

#include "atmosmesh/pins.hpp"

#include <cstdio>

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

std::string format_mic_log(bool sound) {
    return sound ? "mic: sound" : "mic: quiet";
}

std::string format_mic_raw_log(int raw_adc) {
    char line[32];
    std::snprintf(line, sizeof(line), "mic: raw=%d", raw_adc);
    return line;
}

bool mic_raw_is_sound(int raw_adc) {
    return raw_adc >= kMicSoundRawThreshold;
}

std::string format_beep_boot_log() {
    return "beep: boot";
}

}  // namespace atmosmesh

#pragma once

#include <cstdint>
#include <string>

namespace atmosmesh {

struct DebouncedLevel {
    bool has_stable = false;
    bool stable = false;
    bool candidate = false;
    bool has_candidate = false;
    std::uint32_t candidate_since_ms = 0;
};

// Returns true when the published stable level changes (including the first sample).
bool update_debounced_level(DebouncedLevel& state, bool sample, std::uint32_t now_ms,
                            int debounce_ms);

std::string format_pir_log(bool motion);
std::string format_mic_log(bool sound);
std::string format_mic_raw_log(int raw_adc);
bool mic_raw_is_sound(int raw_adc);
std::string format_beep_boot_log();

}  // namespace atmosmesh

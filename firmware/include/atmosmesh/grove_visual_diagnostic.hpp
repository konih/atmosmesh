#pragma once

#include <cstdint>
#include <string>

#include "atmosmesh/status_led.hpp"

#ifndef ATMOSMESH_GROVE_VISUAL_DIAGNOSTIC
#define ATMOSMESH_GROVE_VISUAL_DIAGNOSTIC 0
#endif

namespace atmosmesh {

enum class GroveVisualMode {
    Normal,
    OledFillAndLedCycle,
};

enum class GroveOledRenderAction {
    SkipUnavailable,
    RenderLiveMeasurements,
    HoldFullAreaFill,
};

enum class GroveVisualLedPhase {
    Red,
    Green,
    Amber,
    Off,
};

struct GroveVisualLedState {
    GroveVisualLedPhase phase = GroveVisualLedPhase::Off;
    std::uint32_t phase_started_ms = 0;
    bool initialized = false;
};

struct GroveVisualLedStep {
    GroveVisualLedPhase phase = GroveVisualLedPhase::Off;
    bool changed = false;
};

inline constexpr std::uint32_t kGroveVisualLedPhaseMs = 2000U;

inline constexpr GroveVisualMode compiled_grove_visual_mode() {
#if ATMOSMESH_GROVE_VISUAL_DIAGNOSTIC
    return GroveVisualMode::OledFillAndLedCycle;
#else
    return GroveVisualMode::Normal;
#endif
}

GroveOledRenderAction grove_oled_render_action(bool oled_ready, GroveVisualMode mode);
std::string grove_oled_mode_banner(GroveVisualMode mode, int width_px, int height_px);
GroveVisualLedStep grove_visual_led_begin(GroveVisualLedState& state, std::uint32_t now_ms);
GroveVisualLedStep grove_visual_led_tick(GroveVisualLedState& state, std::uint32_t now_ms);
LedPinLevels grove_visual_led_levels(GroveVisualLedPhase phase, LedPolarity polarity);
std::string grove_visual_led_transition_text(GroveVisualLedPhase phase,
                                             const LedPinLevels& levels);

}  // namespace atmosmesh

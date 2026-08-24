#include "atmosmesh/grove_visual_diagnostic.hpp"

#include <cstdio>

namespace atmosmesh {

GroveOledRenderAction grove_oled_render_action(bool oled_ready, GroveVisualMode mode) {
    if (!oled_ready) {
        return GroveOledRenderAction::SkipUnavailable;
    }
    return mode == GroveVisualMode::OledFillAndLedCycle
               ? GroveOledRenderAction::HoldFullAreaFill
               : GroveOledRenderAction::RenderLiveMeasurements;
}

std::string grove_visual_diagnostic_startup_banner(GroveVisualMode mode, int width_px,
                                                   int height_px) {
    char line[128];
    if (mode == GroveVisualMode::OledFillAndLedCycle) {
        std::snprintf(line, sizeof(line),
                      "visual-diagnostic: selected oled=full-area geometry=%dx%d "
                      "led=red-green-amber-off phase=2000ms",
                      width_px, height_px);
    } else {
        std::snprintf(line, sizeof(line),
                      "visual-diagnostic: not-selected oled=live geometry=%dx%d", width_px,
                      height_px);
    }
    return line;
}

std::string grove_oled_fill_result_text(GroveOledFillResult result, int width_px, int height_px) {
    char line[96];
    switch (result) {
        case GroveOledFillResult::Applied:
            std::snprintf(line, sizeof(line),
                          "oled-diagnostic: fill applied geometry=%dx%d pixels=all-on", width_px,
                          height_px);
            break;
        case GroveOledFillResult::OledNotFound:
            std::snprintf(line, sizeof(line),
                          "oled-diagnostic: fill not-applied geometry=%dx%d reason=oled-not-found",
                          width_px, height_px);
            break;
        case GroveOledFillResult::OledInitFailed:
            std::snprintf(line, sizeof(line),
                          "oled-diagnostic: fill not-applied geometry=%dx%d "
                          "reason=oled-init-failed",
                          width_px, height_px);
            break;
    }
    return line;
}

GroveVisualLedStep grove_visual_led_begin(GroveVisualLedState& state, std::uint32_t now_ms) {
    state.phase = GroveVisualLedPhase::Red;
    state.phase_started_ms = now_ms;
    state.initialized = true;
    return {state.phase, true};
}

GroveVisualLedStep grove_visual_led_tick(GroveVisualLedState& state, std::uint32_t now_ms) {
    if (!state.initialized) {
        return grove_visual_led_begin(state, now_ms);
    }
    if (now_ms - state.phase_started_ms < kGroveVisualLedPhaseMs) {
        return {state.phase, false};
    }
    switch (state.phase) {
        case GroveVisualLedPhase::Red:
            state.phase = GroveVisualLedPhase::Green;
            break;
        case GroveVisualLedPhase::Green:
            state.phase = GroveVisualLedPhase::Amber;
            break;
        case GroveVisualLedPhase::Amber:
            state.phase = GroveVisualLedPhase::Off;
            break;
        case GroveVisualLedPhase::Off:
            state.phase = GroveVisualLedPhase::Red;
            break;
    }
    // Start the next dwell when it is actually applied. Late cooperative ticks never skip a color.
    state.phase_started_ms = now_ms;
    return {state.phase, true};
}

LedPinLevels grove_visual_led_levels(GroveVisualLedPhase phase, LedPolarity polarity) {
    switch (phase) {
        case GroveVisualLedPhase::Red:
            return grove_led_pin_levels(GroveLedStatus::SensorFault, polarity);
        case GroveVisualLedPhase::Green:
            return grove_led_pin_levels(GroveLedStatus::Healthy, polarity);
        case GroveVisualLedPhase::Amber:
            return grove_led_pin_levels(GroveLedStatus::LocalOnly, polarity);
        case GroveVisualLedPhase::Off:
            return grove_led_off_levels(polarity);
    }
    return grove_led_off_levels(polarity);
}

std::string grove_visual_led_transition_text(GroveVisualLedPhase phase,
                                             const LedPinLevels& levels) {
    const char* color = "off";
    switch (phase) {
        case GroveVisualLedPhase::Red:
            color = "red";
            break;
        case GroveVisualLedPhase::Green:
            color = "green";
            break;
        case GroveVisualLedPhase::Amber:
            color = "amber";
            break;
        case GroveVisualLedPhase::Off:
            break;
    }
    char line[80];
    std::snprintf(line, sizeof(line), "visual-diagnostic-led: color=%s D6=%s D0=%s", color,
                  levels.red_high ? "HIGH" : "LOW", levels.green_high ? "HIGH" : "LOW");
    return line;
}

}  // namespace atmosmesh

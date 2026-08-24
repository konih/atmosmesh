#include "atmosmesh/rc_light.hpp"

#include <cstdio>

namespace atmosmesh {

RcLightStep rc_light_begin(RcLightState& state, std::uint32_t now_us) {
    state = {};
    state.phase = RcLightPhase::Discharging;
    state.phase_started_us = now_us;
    return {RcLightPinAction::DriveLow, false};
}

RcLightStep rc_light_tick(RcLightState& state, std::uint32_t now_us, bool input_high) {
    const std::uint32_t elapsed_us = now_us - state.phase_started_us;
    if (state.phase == RcLightPhase::Discharging) {
        if (elapsed_us < kRcLightDischargeUs) {
            return {};
        }
        state.phase = RcLightPhase::Charging;
        state.phase_started_us = now_us;
        return {RcLightPinAction::ReleaseInput, false};
    }

    if (state.phase != RcLightPhase::Charging) {
        return {};
    }

    if (elapsed_us >= kRcLightTimeoutUs) {
        state.phase = RcLightPhase::Complete;
        state.status = RcLightStatus::Timeout;
        state.measurement = {};
        return {RcLightPinAction::None, true};
    }

    if (input_high) {
        state.phase = RcLightPhase::Complete;
        if (elapsed_us == 0U) {
            state.status = RcLightStatus::Saturated;
            state.measurement = {};
        } else {
            state.status = RcLightStatus::Valid;
            state.measurement = {true, elapsed_us};
        }
        return {RcLightPinAction::None, true};
    }

    return {};
}

RcLightStep rc_light_note_released_level(RcLightState& state, bool input_high) {
    if (state.phase != RcLightPhase::Charging || !input_high) {
        return {};
    }
    state.phase = RcLightPhase::Complete;
    state.status = RcLightStatus::Saturated;
    state.measurement = {};
    return {RcLightPinAction::None, true};
}

bool rc_light_active(const RcLightState& state) {
    return state.phase == RcLightPhase::Discharging || state.phase == RcLightPhase::Charging;
}

std::string rc_light_serial_text(const RcLightState& state) {
    if (state.status == RcLightStatus::Valid && state.measurement.valid) {
        char text[64];
        std::snprintf(text, sizeof(text), "light: ok charge=%lu us (raw; lower=brighter)",
                      static_cast<unsigned long>(state.measurement.charge_us));
        return text;
    }
    if (state.status == RcLightStatus::Saturated) {
        return "light: unavailable saturated/immediate";
    }
    if (state.status == RcLightStatus::Timeout) {
        return "light: unavailable timeout/disconnected";
    }
    return "light: unavailable not sampled";
}

}  // namespace atmosmesh

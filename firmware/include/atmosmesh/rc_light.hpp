#pragma once

#include <cstdint>
#include <string>

namespace atmosmesh {

inline constexpr std::uint32_t kRcLightDischargeUs = 1000U;
inline constexpr std::uint32_t kRcLightTimeoutUs = 200000U;

enum class RcLightPhase {
    Idle,
    Discharging,
    Charging,
    Complete,
};

enum class RcLightStatus {
    Unavailable,
    Valid,
    Saturated,
    Timeout,
};

enum class RcLightPinAction {
    None,
    DriveLow,
    ReleaseInput,
};

struct RcLightMeasurement {
    bool valid = false;
    std::uint32_t charge_us = 0;
};

struct RcLightState {
    RcLightPhase phase = RcLightPhase::Idle;
    RcLightStatus status = RcLightStatus::Unavailable;
    RcLightMeasurement measurement{};
    std::uint32_t phase_started_us = 0;
};

struct RcLightStep {
    RcLightPinAction pin_action = RcLightPinAction::None;
    bool completed = false;
};

RcLightStep rc_light_begin(RcLightState& state, std::uint32_t now_us);
RcLightStep rc_light_tick(RcLightState& state, std::uint32_t now_us, bool input_high);
bool rc_light_active(const RcLightState& state);
std::string rc_light_serial_text(const RcLightState& state);

}  // namespace atmosmesh

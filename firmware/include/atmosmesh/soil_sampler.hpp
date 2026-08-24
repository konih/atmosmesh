#pragma once

#include <cstdint>
#include <string>

namespace atmosmesh {

inline constexpr std::uint32_t kSoilSampleIntervalMs = 30000U;
inline constexpr std::uint32_t kSoilSettleMs = 100U;
inline constexpr std::uint32_t kSoilSampleSpacingMs = 5U;
inline constexpr std::uint16_t kSoilSampleCount = 5U;
inline constexpr std::uint32_t kSoilMaxPowerOnMs = 250U;
inline constexpr std::uint32_t kSoilNormalPowerOnMs =
    kSoilSettleMs + (kSoilSampleCount - 1U) * kSoilSampleSpacingMs;
inline constexpr std::uint32_t kSoilMaxDutyPermille =
    (kSoilMaxPowerOnMs * 1000U + kSoilSampleIntervalMs - 1U) / kSoilSampleIntervalMs;

enum class SoilSamplerPhase {
    Waiting,
    Settling,
};

enum class SoilAcquisitionStatus {
    Unavailable,
    Valid,
    Timeout,
    Failed,
};

enum class SoilPowerAction {
    None,
    EnsureOff,
    PowerOn,
    PowerOff,
};

struct SoilAdcMeasurement {
    bool valid = false;
    std::uint16_t raw = 0;
};

struct SoilSamplerState {
    SoilSamplerPhase phase = SoilSamplerPhase::Waiting;
    SoilAcquisitionStatus status = SoilAcquisitionStatus::Unavailable;
    SoilAdcMeasurement measurement{};
    std::uint32_t next_cycle_ms = 0;
    std::uint32_t power_on_started_ms = 0;
    std::uint32_t next_sample_ms = 0;
    std::uint32_t sample_sum = 0;
    std::uint16_t samples_collected = 0;
    bool power_active = false;
};

struct SoilSamplerStep {
    SoilPowerAction power_action = SoilPowerAction::None;
    bool sample_adc = false;
    bool completed = false;
};

SoilSamplerStep soil_sampler_begin(SoilSamplerState& state, std::uint32_t now_ms);
SoilSamplerStep soil_sampler_tick(SoilSamplerState& state, std::uint32_t now_ms);
SoilSamplerStep soil_sampler_record_sample(SoilSamplerState& state, std::uint32_t now_ms,
                                            int adc_raw);
bool soil_sampler_power_active(const SoilSamplerState& state);
bool soil_sampler_acquisition_failed(const SoilSamplerState& state);
bool soil_power_pin_high(SoilPowerAction action);
std::string soil_sampler_serial_text(const SoilSamplerState& state);

}  // namespace atmosmesh

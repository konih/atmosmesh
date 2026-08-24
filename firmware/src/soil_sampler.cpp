#include "atmosmesh/soil_sampler.hpp"

#include <cstdio>

namespace atmosmesh {
namespace {

bool deadline_reached(std::uint32_t now_ms, std::uint32_t deadline_ms) {
    return static_cast<std::int32_t>(now_ms - deadline_ms) >= 0;
}

SoilSamplerStep fail_off(SoilSamplerState& state, SoilAcquisitionStatus status) {
    state.phase = SoilSamplerPhase::Waiting;
    state.status = status;
    state.measurement = {};
    state.power_active = false;
    return {SoilPowerAction::PowerOff, false, true};
}

}  // namespace

SoilSamplerStep soil_sampler_begin(SoilSamplerState& state, std::uint32_t now_ms) {
    state = {};
    state.next_cycle_ms = now_ms + kSoilSampleIntervalMs;
    return {SoilPowerAction::EnsureOff, false, false};
}

SoilSamplerStep soil_sampler_tick(SoilSamplerState& state, std::uint32_t now_ms) {
    if (state.power_active &&
        deadline_reached(now_ms, state.power_on_started_ms + kSoilMaxPowerOnMs)) {
        return fail_off(state, SoilAcquisitionStatus::Timeout);
    }

    if (state.phase == SoilSamplerPhase::Waiting) {
        if (!deadline_reached(now_ms, state.next_cycle_ms)) {
            return {};
        }
        state.phase = SoilSamplerPhase::Settling;
        state.sample_sum = 0;
        state.samples_collected = 0;
        state.power_active = true;
        state.power_on_started_ms = now_ms;
        state.next_sample_ms = now_ms + kSoilSettleMs;
        state.next_cycle_ms = now_ms + kSoilSampleIntervalMs;
        return {SoilPowerAction::PowerOn, false, false};
    }

    if (deadline_reached(now_ms, state.next_sample_ms)) {
        return {SoilPowerAction::None, true, false};
    }
    return {};
}

SoilSamplerStep soil_sampler_record_sample(SoilSamplerState& state, std::uint32_t now_ms,
                                            int adc_raw) {
    if (!state.power_active || state.phase != SoilSamplerPhase::Settling || adc_raw < 0 ||
        adc_raw > 1023) {
        return fail_off(state, SoilAcquisitionStatus::Failed);
    }
    state.sample_sum += static_cast<std::uint16_t>(adc_raw);
    ++state.samples_collected;
    if (state.samples_collected >= kSoilSampleCount) {
        state.measurement = {
            true,
            static_cast<std::uint16_t>((state.sample_sum + kSoilSampleCount / 2U) /
                                       kSoilSampleCount),
        };
        state.status = SoilAcquisitionStatus::Valid;
        state.phase = SoilSamplerPhase::Waiting;
        state.power_active = false;
        return {SoilPowerAction::PowerOff, false, true};
    }
    state.next_sample_ms = now_ms + kSoilSampleSpacingMs;
    return {};
}

bool soil_sampler_power_active(const SoilSamplerState& state) {
    return state.power_active;
}

bool soil_sampler_acquisition_failed(const SoilSamplerState& state) {
    return state.status == SoilAcquisitionStatus::Timeout ||
           state.status == SoilAcquisitionStatus::Failed;
}

bool soil_power_pin_high(SoilPowerAction action) {
    return action != SoilPowerAction::PowerOn;
}

std::string soil_sampler_serial_text(const SoilSamplerState& state) {
    if (state.status == SoilAcquisitionStatus::Valid && state.measurement.valid) {
        char text[64];
        std::snprintf(text, sizeof(text), "soil: ok adc_raw=%u samples=%u power=off",
                      static_cast<unsigned>(state.measurement.raw),
                      static_cast<unsigned>(kSoilSampleCount));
        return text;
    }
    if (state.status == SoilAcquisitionStatus::Timeout) {
        return "soil: unavailable acquisition-timeout power=off";
    }
    if (state.status == SoilAcquisitionStatus::Failed) {
        return "soil: unavailable acquisition-failed power=off";
    }
    return "soil: unavailable not-sampled power=off";
}

}  // namespace atmosmesh

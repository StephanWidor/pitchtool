#pragma once
#include "sw/basics/math.hpp"
#include "sw/containers/spinlockedbuffer.hpp"
#include "sw/containers/utils.hpp"
#include "sw/dft/utils.hpp"
#include "sw/notes.hpp"
#include "sw/spectrum.hpp"
#include "sw/tuningnoteenvelope.hpp"
#include <algorithm>
#include <variant>

namespace sw::pitchtool {

namespace tuning {

struct MidiTune
{
    int midiNoteNumber{-1};    ///< greater 0 -> tune to midi note, otherwise no tune
    int pitchBend{8192};
};

struct AutoTune
{
    AutoTune(const MidiTune midiTune): midiNoteNumber(midiTune.midiNoteNumber), pitchBend(midiTune.pitchBend) {}

    int midiNoteNumber{-1};    ///< greater 0 -> tune to midi note, otherwise autotune
    int pitchBend{8192};
};

using Type = std::variant<std::monostate, AutoTune, MidiTune>;
constexpr auto numTypes = std::variant_size_v<Type>;
constexpr std::array<std::string_view, numTypes> typeNames{"No Tuning", "Midi", "Auto Tune"};

}    // namespace tuning

template<std::floating_point F>
struct TuningParameters
{
    F standardPitch{static_cast<F>(440)};
    F averagingTime{static_cast<F>(0.01)};
    F holdTime{static_cast<F>(0.02)};
    F attackTime{static_cast<F>(0.01)};
};

template<std::floating_point F>
struct ChannelParameters
{
    tuning::Type tuningType;
    F pitchShift{math::zero<F>};
    F formantsShift{math::zero<F>};
    F mixGain{math::zero<F>};
};

template<std::floating_point F>
constexpr F defaultSampleRate()
{
    return static_cast<F>(48000);
}

template<std::floating_point F>
constexpr TuningParameters<F> defaultTuningParameters()
{
    return {};
}

template<std::floating_point F, std::uint8_t NumChannels>
constexpr auto defaultChannelParameters()
{
    return containers::makeArray<NumChannels>([](const size_t channel) {
        return ChannelParameters{.mixGain = (channel == 0u ? math::one<F> : math::zero<F>)};
    });
}

template<std::floating_point F>
constexpr F defaultDryMixGain()
{
    return {math::zero<F>};
}

template<std::floating_point F>
struct ChannelState
{
    ChannelState(const size_t fftLength)
        : coefficients(dft::nyquistLength(fftLength), math::zero<F>)
        , binSpectrum(dft::nyquistLength(fftLength))
        , phases(dft::nyquistLength(fftLength))
        , accumulator(fftLength)
        , spectrum(dft::nyquistLength(fftLength), {})
    {}

    void clear()
    {
        std::ranges::fill(coefficients, math::zero<F>);
        std::ranges::fill(binSpectrum, SpectrumValue<F>{});
        std::ranges::fill(phases, math::zero<F>);
        containers::ringPush(accumulator, math::zero<F>, accumulator.size());
        spectrum.clear();
        fundamentalFrequency = math::zero<F>;
    }

    TuningNoteEnvelope<F> tuningEnvelope;
    std::vector<std::complex<F>> coefficients;
    std::vector<SpectrumValue<F>> binSpectrum;
    std::vector<F> phases;
    std::vector<F> accumulator;
    containers::SpinLockedBuffer<SpectrumValue<F>> spectrum;
    std::atomic<F> fundamentalFrequency{math::zero<F>};    ///< leq 0 means no fundamental frequency found
};

}    // namespace sw::pitchtool

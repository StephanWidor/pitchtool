#pragma once
#include "sw/basics/math.hpp"
#include "sw/containers/spinlockedbuffer.hpp"
#include "sw/containers/utils.hpp"
#include "sw/dft/utils.hpp"
#include "sw/notes.hpp"
#include "sw/spectrum.hpp"
#include "sw/tuningnoteenvelope.hpp"
#include <variant>

namespace sw::pitchtool {

namespace tuning {

struct AutoTune
{};

struct MidiTune
{
    int midiNoteNumber{69};
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
    F averagingTime{static_cast<F>(0.1)};
    F holdTime{static_cast<F>(0.1)};
    F attackTime{static_cast<F>(0.1)};
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
        : binSpectrum(dft::nyquistLength(fftLength))
        , phases(dft::nyquistLength(fftLength))
        , accumulator(fftLength)
        , spectrum(dft::nyquistLength(fftLength), {})
    {}

    void clear()
    {
        fundamentalFrequency = math::zero<F>;
        spectrum.clear();
        containers::ringPush(accumulator, math::zero<F>, accumulator.size());
    }

    TuningNoteEnvelope<F> tuningEnvelope;
    std::vector<SpectrumValue<F>> binSpectrum;
    std::vector<F> phases;
    std::vector<F> accumulator;
    containers::SpinLockedBuffer<SpectrumValue<F>> spectrum;
    std::atomic<F> fundamentalFrequency{math::zero<F>};
};

}    // namespace sw::pitchtool
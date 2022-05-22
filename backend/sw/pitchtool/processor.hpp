#pragma once
#include "sw/basics/ranges.hpp"
#include "sw/basics/variant.hpp"
#include "sw/containers/spinlockedbuffer.hpp"
#include "sw/containers/utils.hpp"
#include "sw/dft/spectrum.hpp"
#include "sw/dft/transform.hpp"
#include "sw/frequencyfilter.hpp"
#include "sw/notes.hpp"
#include "sw/phases.hpp"
#include "sw/pitchtool/types.hpp"
#include "sw/signals.hpp"
#include <cstring>
#include <variant>

namespace sw::pitchtool {

template<std::floating_point F, std::uint8_t NumChannels>
class Processor
{
public:
    Processor(const size_t fftLength, const size_t overSampling)
        : m_fftLength(fftLength)
        , m_overSampling(overSampling)
        , m_fft(fftLength)
        , m_inputState(fftLength)
        , m_channelStates(
            containers::makeArray<NumChannels>([fftLength](const size_t) { return ChannelState<F>(fftLength); }))
        , m_signalWindow(makeVonHannWindow<F>(fftLength))
        , m_processingSignal(fftLength, math::zero<F>)
        , m_envelopeAlignmentFactors(dft::nyquistLength(fftLength), math::one<F>)
        , m_coefficients(dft::nyquistLength(fftLength), math::zero<F>)
        , m_formantsSpectrum(dft::nyquistLength(fftLength))
    {
        assert(overSampling > 1u && overSampling * overSampling < fftLength &&
               fftLength == (fftLength / overSampling) * overSampling);
    }

    void process(ranges::TypedInputRange<F> auto &&signal, ranges::TypedOutputRange<F> auto &&o_signal,
                 const F sampleRate, const TuningParameters<F> &tuningParameters,
                 const std::array<ChannelParameters<F>, NumChannels> &channelParameters, const F dryMixGain)
    {
        const auto stepSize = static_cast<int>(this->stepSize());
        const auto timeDiff = static_cast<F>(stepSize) / sampleRate;

        assert(std::ranges::ssize(signal) == stepSize);
        assert(std::ranges::ssize(o_signal) == stepSize);

        const auto alignFormants = [&](const std::vector<SpectrumValue<F>> &spectrum,
                                       std::vector<SpectrumValue<F>> &io_spectrumToAlign) {
            envelopeAlignmentFactors<F>(gains<F>(spectrum), gains<F>(io_spectrumToAlign), m_envelopeAlignmentFactors);
            std::transform(io_spectrumToAlign.begin(), io_spectrumToAlign.end(), m_envelopeAlignmentFactors.begin(),
                           io_spectrumToAlign.begin(), [](const auto &spectrumValue, const F factor) {
                               return SpectrumValue{spectrumValue.frequency, factor * spectrumValue.gain};
                           });
        };

        const auto filterSpectrum = [&](ChannelState<F> &io_state) {
            io_state.spectrum.apply([&](std::vector<SpectrumValue<F>> &o_buffer) {
                o_buffer.clear();
                std::copy_if(io_state.binSpectrum.begin() + 1, io_state.binSpectrum.end(), std::back_inserter(o_buffer),
                             [zeroGainThresholdLinear = dBToFactor(static_cast<F>(-60))](const auto &value) {
                                 return value.gain > zeroGainThresholdLinear;
                             });
                identifyFrequencies(o_buffer);
            });
        };

        const auto tuningFactor = [&](const tuning::Type &type, TuningNoteEnvelope<F> &tuningEnvelope) {
            const auto noteFactor = [&](const Note &note) {
                const auto envelopeFactor = tuningEnvelope.process(note, tuningParameters.attackTime, timeDiff);
                if (m_inputState.fundamentalFrequency <= math::zero<F>)
                    return math::one<F>;
                const auto noteFrequency = toFrequency(note, tuningParameters.standardPitch);
                const auto tunedFrequency =
                  std::pow(static_cast<F>(2), envelopeFactor * std::log2(m_inputState.fundamentalFrequency) +
                                                (math::one<F> - envelopeFactor) * std::log2(noteFrequency));
                return tunedFrequency / m_inputState.fundamentalFrequency;
            };
            return std::visit(overloaded{[](std::monostate) { return math::one<F>; },
                                         [&](tuning::AutoTune) {
                                             return noteFactor(toNote(m_inputState.fundamentalFrequency.load(),
                                                                      tuningParameters.standardPitch));
                                         },
                                         [&](const Note &note) { return noteFactor(note); }},
                              type);
        };

        const auto processChannel = [&](const ChannelParameters<F> &parameters, ChannelState<F> &io_state) {
            if (math::isZero(parameters.mixGain))
            {
                io_state.clear();
                return;
            }

            const auto pitchFactor =
              tuningFactor(parameters.tuningType, io_state.tuningEnvelope) * semitonesToFactor(parameters.pitchShift);
            io_state.fundamentalFrequency = pitchFactor * m_inputState.fundamentalFrequency;
            dft::shiftPitch<F>(pitchFactor, m_inputState.binSpectrum, io_state.binSpectrum);

            const auto formantsFactor = semitonesToFactor(parameters.formantsShift);
            if (!math::equal(pitchFactor, formantsFactor))
            {
                if (math::equal(formantsFactor, math::one<F>))
                    alignFormants(m_inputState.binSpectrum, io_state.binSpectrum);
                else
                {
                    dft::shiftPitch<F>(formantsFactor, m_inputState.binSpectrum, m_formantsSpectrum);
                    alignFormants(m_formantsSpectrum, io_state.binSpectrum);
                }
            }

            filterSpectrum(io_state);

            shiftPhases<F>(io_state.phases, frequencies<F>(io_state.binSpectrum), timeDiff, io_state.phases);

            dft::toBinCoefficients<F>(io_state.binSpectrum, io_state.phases, m_coefficients);
            m_fft.transform_inverse(m_coefficients, m_processingSignal, false);
            std::transform(m_processingSignal.begin(), m_processingSignal.end(), m_signalWindow.begin(),
                           m_processingSignal.begin(),
                           [factor = static_cast<F>(0.6)](const auto s, const auto w) { return factor * s * w; });

            containers::ringPush(io_state.accumulator, math::zero<F>, stepSize);
            std::transform(m_processingSignal.begin(), m_processingSignal.end(), io_state.accumulator.begin(),
                           io_state.accumulator.begin(), std::plus());

            std::transform(io_state.accumulator.begin(), io_state.accumulator.begin() + stepSize, o_signal.begin(),
                           o_signal.begin(),
                           [mixGain = parameters.mixGain](const auto stateSample, const auto outSample) {
                               return mixGain * stateSample + outSample;
                           });
        };

        containers::ringPush(m_inputState.accumulator, signal);
        std::transform(m_inputState.accumulator.begin(), m_inputState.accumulator.begin() + stepSize, o_signal.begin(),
                       [dryMixGain](const auto sample) { return dryMixGain * sample; });

        std::transform(m_signalWindow.begin(), m_signalWindow.end(),
                       m_inputState.accumulator.end() - static_cast<int>(m_fftLength), m_processingSignal.begin(),
                       std::multiplies());
        m_fft.transform(m_processingSignal, m_coefficients, false);
        dft::toSpectrumByPhase<F>(sampleRate, timeDiff, m_inputState.phases, m_coefficients, m_inputState.binSpectrum,
                                  m_inputState.phases);
        filterSpectrum(m_inputState);
        m_inputState.fundamentalFrequency =
          m_frequencyFilter.process(findFundamental<F>(m_inputState.spectrum.inBuffer()).frequency,
                                    tuningParameters.frequencyAveragingTime, timeDiff);

        for (auto i = 0u; i < NumChannels; ++i)
            processChannel(channelParameters[i], m_channelStates[i]);
    }

    void processByPassed(ranges::TypedInputRange<F> auto &&signal, ranges::TypedRange<F> auto &&o_signal)
    {
        const auto stepSize = static_cast<int>(this->stepSize());
        assert(static_cast<int>(std::ranges::ssize(signal)) == stepSize);
        assert(static_cast<int>(std::ranges::ssize(o_signal)) == stepSize);

        containers::ringPush(m_inputState.accumulator, signal);
        m_inputState.fundamentalFrequency = math::zero<F>;
        m_inputState.spectrum.clear();

        for (auto &channelState : m_channelStates)
            channelState.clear();

        std::copy(m_inputState.accumulator.begin(), m_inputState.accumulator.begin() + stepSize, o_signal.begin());
    }

    size_t fftLength() const { return m_fftLength; }

    size_t overSampling() const { return m_overSampling; }

    size_t stepSize() const { return m_fftLength / m_overSampling; }

    size_t overlapSize() const { return m_fftLength - stepSize(); }

    const std::vector<SpectrumValue<F>> &inSpectrum() const { return m_inputState.spectrum.outBuffer(); }

    F inFundamentalFrequency() const { return m_inputState.fundamentalFrequency; }

    const std::vector<SpectrumValue<F>> &outSpectrum(const size_t channel) const
    {
        return m_channelStates[channel].spectrum.outBuffer();
    }

    F outFundamentalFrequency(const size_t channel) const { return m_channelStates[channel].fundamentalFrequency; }

private:
    size_t m_fftLength{0u};
    size_t m_overSampling{0u};
    sw::dft::FFT<F> m_fft;

    ChannelState<F> m_inputState;
    std::array<ChannelState<F>, NumChannels> m_channelStates;

    FrequencyFilter<F> m_frequencyFilter{100u};

    // helpers
    std::vector<F> m_signalWindow, m_processingSignal, m_envelopeAlignmentFactors;
    std::vector<std::complex<F>> m_coefficients;
    std::vector<SpectrumValue<F>> m_formantsSpectrum;
};

}    // namespace sw::pitchtool

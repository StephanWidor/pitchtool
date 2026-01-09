#pragma once
#include "sw/pitchtool/types.hpp"
#include <cstring>
#include <sw/containers/spinlockedringbuffer.hpp>
#include <sw/containers/utils.hpp>
#include <sw/dft/spectrum.hpp>
#include <sw/dft/transform.hpp>
#include <sw/frequencyenvelope.hpp>
#include <sw/notes.hpp>
#include <sw/phases.hpp>
#include <sw/ranges/utils.hpp>
#include <sw/signals.hpp>
#include <sw/variant.hpp>
#include <variant>

namespace sw::pitchtool {

namespace detail {

template<std::floating_point F>
void toFilteredSpectrum(std::vector<SpectrumValue<F>> &binSpectrum, std::vector<SpectrumValue<F>> &o_spectrum)
{
    o_spectrum.clear();
    std::copy_if(binSpectrum.begin() + 1, binSpectrum.end(), std::back_inserter(o_spectrum),
                 [zeroGainThresholdLinear = dBToFactor(static_cast<F>(-60))](const auto &value) {
                     return value.gain > zeroGainThresholdLinear;
                 });
    identifyFrequencies(o_spectrum);
}

template<std::floating_point F>
F tuningFactor(const TuningParameters<F> &tuningParameters, const tuning::Type &type,
               TuningNoteEnvelope<F> &tuningEnvelope, const F fundamentalFrequency, const F timeDiff)
{
    const auto autoTuneToNote = [&](const tuning::AutoTune &autoTune) {
        return autoTune.midiNoteNumber >= 0 ? fromMidi(autoTune.midiNoteNumber) :
                                              toNote(fundamentalFrequency, tuningParameters.standardPitch);
    };
    const auto midiTuneToNote = [&](const tuning::MidiTune &midiTune) {
        return midiTune.midiNoteNumber >= 0 ? fromMidi(midiTune.midiNoteNumber) : Note{Note::Name::Invalid, 0};
    };

    const auto noteFactor = [&](const Note &note, const float deviation = math::zero<F>) {
        const auto envelopeFactor = tuningEnvelope.process(note, tuningParameters.attackTime, timeDiff);

        if (fundamentalFrequency <= math::zero<F>)
            return math::one<F>;

        const auto noteFrequency = toFrequency<F>(note, tuningParameters.standardPitch, deviation);

        const auto tunedFrequency =
          std::pow(static_cast<F>(2), (math::one<F> - envelopeFactor) * std::log2(fundamentalFrequency) +
                                        envelopeFactor * std::log2(noteFrequency));

        return tunedFrequency / fundamentalFrequency;
    };

    return std::visit(
      overloaded{[](std::monostate) { return math::one<F>; },
                 [&](const tuning::AutoTune autoTune) {
                     return noteFactor(autoTuneToNote(autoTune), midiPitchBendToSemitones<F>(autoTune.pitchBend));
                 },
                 [&](const tuning::MidiTune midiTune) {
                     return noteFactor(midiTuneToNote(midiTune), midiPitchBendToSemitones<F>(midiTune.pitchBend));
                 }},
      type);
}

template<std::floating_point F>
void shiftPitch(const ChannelState<F> &inputState, const F pitchFactor, const F sampleRate, const F timeDiff,
                ChannelState<F> &io_state)
{
    const auto numValues = static_cast<int>(std::ranges::ssize(io_state.binSpectrum));
    assert(std::ranges::ssize(io_state.binSpectrum) == numValues);
    assert(std::ranges::ssize(inputState.binSpectrum) == numValues);
    assert(std::ranges::ssize(io_state.coefficients) == numValues);
    assert(std::ranges::ssize(io_state.phases) == numValues);

    const auto sourceIndexRange = [&](const auto index) {
        const auto beginIndex =
          std::min(static_cast<int>(
                     std::max(math::zero<F>, std::ceil((static_cast<F>(index) - math::oneHalf<F>) / pitchFactor))),
                   numValues);
        const auto endIndex = std::clamp<int>(
          static_cast<int>(std::ceil((static_cast<F>(index) + math::oneHalf<F>) / pitchFactor)), 0, numValues);
        assert(endIndex >= beginIndex);
        return std::array<size_t, 2>{static_cast<size_t>(beginIndex), static_cast<size_t>(endIndex)};
    };

    const auto shiftedCoefficient = [&](const F lastPhase, const F refPhase, const F frequency, const F sourceGain) {
        const auto newPhase = math::angles::standardized(lastPhase + phaseAngle(frequency, timeDiff));
        const auto cosAngle = std::cos(refPhase - newPhase);
        constexpr auto absBound = static_cast<F>(0.7);
        const auto angleFactor = math::one<F> / std::max(std::abs(cosAngle), absBound);
        return std::polar<F>(std::min(math::one<F>, angleFactor * sourceGain), newPhase);
    };

    const auto accumulatedCoefficient = [&](const size_t targetIndex) {
        std::complex<F> c{math::zero<F>};
        const auto range = sourceIndexRange(targetIndex);
        for (size_t sourceIndex = range.front(); sourceIndex < range.back(); ++sourceIndex)
        {
            const auto sourceGain = inputState.binSpectrum[sourceIndex].gain;
            const auto frequency = pitchFactor * inputState.binSpectrum[sourceIndex].frequency;
            c +=
              shiftedCoefficient(io_state.phases[targetIndex], inputState.phases[sourceIndex], frequency, sourceGain);
        }
        return c;
    };

    const auto gainFactor = static_cast<F>(numValues - 1);
    const auto binFrequencyStep = dft::binFrequencyStep(dft::signalLength(numValues), sampleRate);
    for (auto targetIndex = 0; targetIndex < numValues; ++targetIndex)
    {
        const auto coefficient = accumulatedCoefficient(targetIndex);
        const auto lastPhase = io_state.phases[targetIndex];
        const auto gain = std::abs(coefficient);

        const auto newPhase = math::isZero(gain) ? inputState.phases[targetIndex] : std::arg(coefficient);
        const auto frequency =
          dft::correctedFrequency(lastPhase, newPhase, timeDiff, static_cast<F>(targetIndex) * binFrequencyStep);

        io_state.coefficients[targetIndex] = gainFactor * coefficient;
        io_state.binSpectrum[targetIndex].frequency = frequency;
        io_state.binSpectrum[targetIndex].gain = gain;
        io_state.phases[targetIndex] = newPhase;
    }

    const auto rotate = [](auto &io_c, auto &io_phase) {
        io_c *= std::polar(math::one<F>, -io_phase);
        io_phase = math::zero<F>;
        io_c.imag(math::zero<F>);    // to make imag part really zero, also after formants filter
    };

    rotate(io_state.coefficients.front(), io_state.phases.front());
    rotate(io_state.coefficients.back(), io_state.phases.back());
}

}    // namespace detail

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
        , m_formantsStates(
            containers::makeArray<NumChannels>([fftLength](const size_t) { return ChannelState<F>(fftLength); }))
        , m_signalWindow(makeVonHannWindow<F>(fftLength))
        , tmp_processingSignal(fftLength, math::zero<F>)
        , tmp_envelopeAlignmentFactors(dft::nyquistLength(fftLength), math::one<F>)
    {
        assert(overSampling > 1u && overSampling * overSampling < fftLength &&
               fftLength == (fftLength / overSampling) * overSampling);
    }

    template<ranges::TypedInputRange<F> InSignal, ranges::TypedOutputRange<F> OutSignal>
    void process(InSignal &&signal, OutSignal &&o_signal, const F sampleRate,
                 const TuningParameters<F> &tuningParameters,
                 const std::array<ChannelParameters<F>, NumChannels> &channelParameters, const F dryMixGain)
    {
        const auto stepSize = static_cast<int>(this->stepSize());
        const auto timeDiff = static_cast<F>(stepSize) / sampleRate;

        assert(std::ranges::ssize(signal) == stepSize);
        assert(std::ranges::ssize(o_signal) == stepSize);

        {    // update input state
            containers::ringPush(m_inputState.accumulator, signal);

            std::transform(m_signalWindow.begin(), m_signalWindow.end(),
                           m_inputState.accumulator.end() - static_cast<int>(m_fftLength), tmp_processingSignal.begin(),
                           std::multiplies());

            m_fft.transform(tmp_processingSignal, m_inputState.coefficients);

            dft::toSpectrumByPhase<F>(sampleRate, timeDiff, m_inputState.phases, m_inputState.coefficients,
                                      m_inputState.binSpectrum, m_inputState.phases);

            detail::toFilteredSpectrum(m_inputState.binSpectrum, m_inputState.spectrumSwap.inSwap());

            const auto squaredGainsThreshold =
              static_cast<F>(0.3) *
              ranges::accumulate<F>(gains<F>(m_inputState.binSpectrum) |
                                    std::views::transform([](const auto gain) { return gain * gain; }));

            m_inputState.fundamentalFrequency = m_frequencyEnvelope.process(
              findFundamental<F>(m_inputState.spectrumSwap.inSwap(), squaredGainsThreshold).frequency, timeDiff,
              tuningParameters.averagingTime, tuningParameters.holdTime);

            m_inputState.spectrumSwap.push();
        }

        {    // process channels
            for (auto i = 0u; i < NumChannels; ++i)
            {
                processChannel(channelParameters[i], tuningParameters, sampleRate, timeDiff, stepSize,
                               m_channelStates[i], m_formantsStates[i]);
            }
        }

        {    // fill output
            std::transform(m_inputState.accumulator.begin(), m_inputState.accumulator.begin() + stepSize,
                           o_signal.begin(), [dryMixGain](const auto sample) { return dryMixGain * sample; });
            for (auto i = 0u; i < NumChannels; ++i)
            {
                auto &channelState = m_channelStates[i];
                const auto mixGain = channelParameters[i].mixGain;
                if (!math::isZero(mixGain))
                {
                    std::transform(channelState.accumulator.begin(), channelState.accumulator.begin() + stepSize,
                                   o_signal.begin(), o_signal.begin(),
                                   [mixGain](const auto stateSample, const auto outSample) {
                                       return mixGain * stateSample + outSample;
                                   });
                }
            }
        }
    }

    void processByPassed(ranges::TypedInputRange<F> auto &&signal, ranges::TypedRange<F> auto &&o_signal)
    {
        const auto stepSize = static_cast<int>(this->stepSize());
        assert(static_cast<int>(std::ranges::ssize(signal)) == stepSize);
        assert(static_cast<int>(std::ranges::ssize(o_signal)) == stepSize);

        containers::ringPush(m_inputState.accumulator, signal);
        m_inputState.fundamentalFrequency = math::zero<F>;
        m_inputState.spectrumSwap.inSwap().clear();
        m_inputState.spectrumSwap.push();

        for (auto i = 0; i < NumChannels; ++i)
        {
            m_channelStates[i].clear();
            m_formantsStates[i].clear();
        }

        std::copy(m_inputState.accumulator.begin(), m_inputState.accumulator.begin() + stepSize, o_signal.begin());
    }

    size_t fftLength() const { return m_fftLength; }

    size_t overSampling() const { return m_overSampling; }

    size_t stepSize() const { return m_fftLength / m_overSampling; }

    size_t overlapSize() const { return m_fftLength - stepSize(); }

    const std::vector<SpectrumValue<F>> &inputSpectrum() const { return m_inputState.spectrumSwap.pull(); }

    F inFundamentalFrequency() const { return m_inputState.fundamentalFrequency; }

    const std::vector<SpectrumValue<F>> &outputSpectrum(const size_t channel) const
    {
        return m_channelStates[channel].spectrumSwap.pull();
    }

    F outFundamentalFrequency(const size_t channel) const { return m_channelStates[channel].fundamentalFrequency; }

private:
    void processChannel(const ChannelParameters<F> &parameters, const TuningParameters<F> &tuningParameters,
                        const F sampleRate, const F timeDiff, const int stepSize, ChannelState<F> &io_channelState,
                        ChannelState<F> &io_formantsState)
    {
        if (math::isZero(parameters.mixGain))
        {
            io_channelState.clear();
            io_formantsState.clear();
            return;
        }

        const auto pitchFactor =
          detail::tuningFactor<F>(tuningParameters, parameters.tuningType, io_channelState.tuningEnvelope,
                                  m_inputState.fundamentalFrequency, timeDiff) *
          semitonesToFactor(parameters.pitchShift);
        const auto formantsFactor = semitonesToFactor(parameters.formantsShift);

        io_channelState.fundamentalFrequency = pitchFactor * m_inputState.fundamentalFrequency;
        io_formantsState.fundamentalFrequency = formantsFactor * m_inputState.fundamentalFrequency;

        detail::shiftPitch(m_inputState, pitchFactor, sampleRate, timeDiff, io_channelState);
        detail::shiftPitch<F>(m_inputState, formantsFactor, sampleRate, timeDiff, io_formantsState);

        if (!math::equal(pitchFactor, formantsFactor))
        {
            if (math::equal(formantsFactor, math::one<F>))
            {
                envelopeAlignmentFactors<F>(gains<F>(m_inputState.binSpectrum), gains<F>(io_channelState.binSpectrum),
                                            tmp_envelopeAlignmentFactors);
            }
            else
            {
                envelopeAlignmentFactors<F>(gains<F>(io_formantsState.binSpectrum),
                                            gains<F>(io_channelState.binSpectrum), tmp_envelopeAlignmentFactors);
            }
            std::ranges::transform(tmp_envelopeAlignmentFactors, io_channelState.coefficients,
                                   io_channelState.coefficients.begin(), std::multiplies());
            auto stateGains = gains<F>(io_channelState.binSpectrum);
            std::ranges::transform(tmp_envelopeAlignmentFactors, stateGains, stateGains.begin(), std::multiplies());
        }

        detail::toFilteredSpectrum(io_channelState.binSpectrum, io_channelState.spectrumSwap.inSwap());

        m_fft.transform_inverse(io_channelState.coefficients, tmp_processingSignal);

        std::transform(tmp_processingSignal.begin(), tmp_processingSignal.end(), m_signalWindow.begin(),
                       tmp_processingSignal.begin(),
                       [factor = static_cast<F>(0.7)](const auto s, const auto w) { return factor * s * w; });

        containers::ringPush(io_channelState.accumulator, math::zero<F>, stepSize);
        std::transform(tmp_processingSignal.begin(), tmp_processingSignal.end(), io_channelState.accumulator.begin(),
                       io_channelState.accumulator.begin(), std::plus());

        io_channelState.spectrumSwap.push();
    }

    size_t m_fftLength{0u};
    size_t m_overSampling{0u};
    sw::dft::FFT<F> m_fft;

    ChannelState<F> m_inputState;
    std::array<ChannelState<F>, NumChannels> m_channelStates;
    std::array<ChannelState<F>, NumChannels> m_formantsStates;

    FrequencyEnvelope<F> m_frequencyEnvelope{100u};

    std::vector<F> m_signalWindow;

    // helpers
    std::vector<F> tmp_processingSignal, tmp_envelopeAlignmentFactors;
};

}    // namespace sw::pitchtool

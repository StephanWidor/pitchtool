#pragma once
#include "sw/dft/utils.hpp"
#include "sw/phases.hpp"
#include "sw/signals.hpp"
#include "sw/spectrum.hpp"

namespace sw::dft {

/// Use phases of dft coefficients to "correct" bin frequencies
/// lastBinPhases and o_binPhases can be the same range, to update phases in place
template<std::floating_point F, ranges::TypedOutputRange<SpectrumValue<F>> SpectrumRange>
void toSpectrumByPhase(const F sampleRate, const F timeDiff, ranges::TypedInputRange<F> auto &&lastBinPhases,
                       ranges::TypedInputRange<std::complex<F>> auto &&binCoefficients, SpectrumRange &&o_binSpectrum,
                       ranges::TypedOutputRange<F> auto &&o_binPhases)
{
    assert(std::ranges::size(binCoefficients) > 1u &&
           std::ranges::size(binCoefficients) == std::ranges::size(lastBinPhases) &&
           std::ranges::size(binCoefficients) == std::ranges::size(o_binSpectrum) &&
           std::ranges::size(binCoefficients) == std::ranges::size(o_binPhases));
    const auto halfSignalLength = std::ranges::size(binCoefficients) - 1u;
    const auto signalLength = 2u * halfSignalLength;

    // unfortunately, we don't have zip_view yet, so we do some dirty hacky storing binFrequencies and phases
    // temporarily in o_spectrum
    const auto o_gains = gains<F>(std::forward<SpectrumRange>(o_binSpectrum));
    const auto o_frequencies = frequencies<F>(std::forward<SpectrumRange>(o_binSpectrum));

    std::ranges::copy(dft::binFrequencies(signalLength, sampleRate) |
                        std::views::take(std::ranges::ssize(o_binSpectrum)),
                      o_frequencies.begin());
    std::ranges::transform(binCoefficients, o_gains.begin(), [](const auto &c) { return std::arg(c); });
    std::ranges::transform(lastBinPhases, o_binSpectrum, o_frequencies.begin(),
                           [timeDiff](const auto lastPhase, const auto &binFrequencyAndPhase) {
                               const auto coefficientPhase = binFrequencyAndPhase.gain;
                               const auto binFrequency = binFrequencyAndPhase.frequency;
                               const auto expectedAngle = phaseAngle(binFrequency, timeDiff);
                               const auto expectedPhase = standardized(lastPhase + expectedAngle);
                               const auto phaseDiff = standardized(coefficientPhase - expectedPhase);
                               const auto angle = expectedAngle + phaseDiff;
                               const auto frequency = sw::frequency(angle, timeDiff);
                               return std::abs(frequency);
                           });
    std::ranges::copy(o_gains, o_binPhases.begin());
    std::ranges::transform(binCoefficients, o_gains.begin(),
                           [gainFactor = math::one<F> / static_cast<F>(halfSignalLength)](const auto &c) {
                               return gainFactor * std::abs(c);
                           });
}

template<std::floating_point F>
void toBinCoefficients(ranges::TypedInputRange<SpectrumValue<F>> auto &&spectrum,
                       ranges::TypedInputRange<F> auto &&phases,
                       ranges::TypedOutputRange<std::complex<F>> auto &&o_coefficients)
{
    assert(std::ranges::size(spectrum) > 1u && std::ranges::size(spectrum) == std::ranges::size(phases) &&
           std::ranges::size(spectrum) == std::ranges::size(o_coefficients));
    const auto halfSignalLength = std::ranges::size(o_coefficients) - 1u;
    const auto gainFactor = static_cast<F>(halfSignalLength);

    std::transform(
      spectrum.begin(), spectrum.end(), phases.begin(), o_coefficients.begin(),
      [&](const auto spectrumValue, const auto phase) { return std::polar(gainFactor * spectrumValue.gain, phase); });
    o_coefficients.front().imag(math::zero<F>);
    o_coefficients.back().imag(math::zero<F>);
}

template<std::floating_point F>
void shiftPitch(const F pitchFactor, ranges::TypedInputRange<SpectrumValue<F>> auto &&binSpectrum,
                ranges::TypedOutputRange<SpectrumValue<F>> auto &&o_shiftedBinSpectrum)
{
    assert(pitchFactor > math::zero<F>);

    const auto numValues = std::ranges::ssize(binSpectrum);
    using SizeType = decltype(numValues);
    assert(std::ranges::ssize(o_shiftedBinSpectrum) == numValues);

    const auto rangeToMerge = [&](const auto i) {
        const auto beginIndex =
          std::min(static_cast<SizeType>(
                     std::max(math::zero<F>, std::ceil((static_cast<F>(i) - math::oneHalf<F>) / pitchFactor))),
                   numValues);
        const auto endIndex =
          std::clamp(static_cast<SizeType>(std::ceil((static_cast<F>(i) + math::oneHalf<F>) / pitchFactor)),
                     math::zero<SizeType>, numValues);
        assert(endIndex >= beginIndex);
        return std::views::counted(binSpectrum.begin() + beginIndex, endIndex - beginIndex);
    };

    const auto shiftedSpectrumValue = [&](const auto i) {
        const auto range = rangeToMerge(i);
        const auto unpitchedValue = toOneSpectrumValue<F>(range);
        return SpectrumValue<F>{pitchFactor * unpitchedValue.frequency, unpitchedValue.gain};
    };

    std::ranges::transform(std::views::iota(math::zero<SizeType>, numValues), o_shiftedBinSpectrum.begin(),
                           shiftedSpectrumValue);
}

}    // namespace sw::dft

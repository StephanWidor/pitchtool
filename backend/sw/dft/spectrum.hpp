#pragma once
#include "sw/dft/utils.hpp"
#include "sw/phases.hpp"
#include "sw/signals.hpp"
#include "sw/spectrum.hpp"

namespace sw::dft {

/// Use phase of dft coefficient to "correct" bin frequency
template<std::floating_point F>
F correctedFrequency(const F lastPhase, const F coefficientPhase, const F timeDiff, const F binFrequency)
{
    const auto expectedAngle = phaseAngle(binFrequency, timeDiff);
    const auto expectedPhase = standardized(lastPhase + expectedAngle);
    const auto phaseDiff = standardized(coefficientPhase - expectedPhase);
    const auto angle = expectedAngle + phaseDiff;
    const auto frequency = sw::frequency(angle, timeDiff);
    return std::abs(frequency);
}

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

    const auto o_gains = gains<F>(std::forward<SpectrumRange>(o_binSpectrum));
    const auto o_frequencies = frequencies<F>(std::forward<SpectrumRange>(o_binSpectrum));

    // unfortunately, we don't have zip_view yet, so we do some dirty hacky storing binFrequencies and phases
    // temporarily in o_spectrum
    //    std::ranges::transform(binCoefficients, o_binPhases.begin(), [](const auto &c) { return std::arg(c); });
    //    const auto binFrequencies =
    //      dft::binFrequencies(signalLength, sampleRate) | std::views::take(std::ranges::ssize(o_binSpectrum));
    //    const auto phasesAndBinFrequencies = std::views::zip(lastBinPhases, o_binPhases, binFrequencies);
    //    std::ranges::transform(phasesAndBinFrequencies, o_frequencies.begin(), correctedFrequency);

    std::ranges::copy(dft::binFrequencies(signalLength, sampleRate) |
                        std::views::take(std::ranges::ssize(o_binSpectrum)),
                      o_frequencies.begin());
    std::ranges::transform(binCoefficients, o_gains.begin(), [](const auto &c) { return std::arg(c); });
    std::ranges::transform(
      lastBinPhases, o_binSpectrum, o_frequencies.begin(), [&](const auto lastPhase, const auto &binFrequencyAndPhase) {
          return correctedFrequency(lastPhase, binFrequencyAndPhase.gain, timeDiff, binFrequencyAndPhase.frequency);
      });
    std::ranges::copy(o_gains, o_binPhases.begin());

    std::ranges::transform(binCoefficients, o_gains.begin(),
                           [gainFactor = math::one<F> / static_cast<F>(halfSignalLength)](const auto &c) {
                               return gainFactor * std::abs(c);
                           });
}

}    // namespace sw::dft

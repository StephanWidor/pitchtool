#pragma once
#include "sw/basics/math.hpp"
#include "sw/basics/ranges.hpp"
#include "sw/notes.hpp"
#include "sw/signals.hpp"
#include <numeric>

namespace sw {

template<std::floating_point F>
struct SpectrumValue
{
    F frequency{math::zero<F>};
    F gain{math::zero<F>};
};

template<std::floating_point F, ranges::TypedRange<SpectrumValue<F>> R>
auto gains(R &&spectrum)
{
    return std::forward<R>(spectrum) | std::views::transform([](auto &v) -> auto & { return v.gain; });
}

template<std::floating_point F, ranges::TypedRange<SpectrumValue<F>> R>
auto frequencies(R &&spectrum)
{
    return std::forward<R>(spectrum) | std::views::transform([](auto &v) -> auto & { return v.frequency; });
}

template<std::floating_point F>
void removeSmallGains(std::vector<SpectrumValue<F>> &io_spectrum, const F minGainDB)
{
    io_spectrum.erase(
      std::remove_if(io_spectrum.begin(), io_spectrum.end(), [minGainDB](const auto &v) { return v.gain < minGainDB; }),
      io_spectrum.end());
}

template<std::floating_point F, ranges::TypedInputRange<SpectrumValue<F>> Spectrum>
SpectrumValue<F> toOneSpectrumValue(Spectrum &&spectrumOfIdentifyableFrequencies)
{
    if (std::ranges::empty(std::forward<Spectrum>(spectrumOfIdentifyableFrequencies)))
        return {math::zero<F>, math::zero<F>};
    if (std::ranges::size(spectrumOfIdentifyableFrequencies) == 1)
        return *spectrumOfIdentifyableFrequencies.begin();
    const auto frequencies = sw::frequencies<F>(std::forward<Spectrum>(spectrumOfIdentifyableFrequencies));
    const auto gains = sw::gains<F>(std::forward<Spectrum>(spectrumOfIdentifyableFrequencies));
    const auto gain = std::sqrt(std::inner_product(gains.begin(), gains.end(), gains.begin(), math::zero<F>));
    const auto frequency = math::isZero(gain) ?
                             average<F>(std::forward<decltype(frequencies)>(frequencies)) :
                             weightedGeometricAverage<F>(std::forward<decltype(frequencies)>(frequencies),
                                                         std::forward<decltype(gains)>(gains));
    return {frequency, gain};
};

template<std::floating_point F>
void identifyFrequencies(std::vector<SpectrumValue<F>> &io_spectrum, const F frequencyRatioTolerance = semitoneRatio<F>,
                         bool sort = false)
{
    if (std::ranges::ssize(io_spectrum) < 2)
        return;
    if (sort)
        std::ranges::sort(io_spectrum, [](const auto &s0, const auto &s1) { return s0.frequency < s1.frequency; });
    auto mergeStartIndex = 0u, j = 0u;
    for (auto i = 1u; i < io_spectrum.size(); ++i)
    {
        if (math::maxRatio(io_spectrum[i].frequency, io_spectrum[i - 1].frequency) > frequencyRatioTolerance)
        {
            io_spectrum[j++] = toOneSpectrumValue<F>(
              std::span(io_spectrum.begin() + static_cast<int>(mergeStartIndex), i - mergeStartIndex));
            mergeStartIndex = i;
        }
    }
    io_spectrum[j++] =
      toOneSpectrumValue<F>(std::span(io_spectrum.begin() + static_cast<int>(mergeStartIndex), io_spectrum.end()));
    io_spectrum.resize(j);
}

template<std::floating_point F, ranges::TypedInputRange<SpectrumValue<F>> Spectrum>
F maxGain(Spectrum &&spectrum)
{
    return std::ranges::max_element(std::forward<Spectrum>(spectrum),
                                    [](const auto &v0, const auto &v1) { return v0.gain < v1.gain; })
      ->gain;
}

template<std::floating_point F, ranges::TypedInputRange<SpectrumValue<F>> Spectrum>
SpectrumValue<F> findFundamental(Spectrum &&spectrum)
{
    if (std::ranges::empty(spectrum))
        return {};
    if (std::ranges::ssize(spectrum) == 1)
        return *spectrum.begin();
    const auto gainThreshold =
      std::ranges::max_element(std::forward<Spectrum>(spectrum),
                               [](const auto &v0, const auto &v1) { return v0.gain < v1.gain; })
        ->gain *
      math::oneHalf<F>;

    if (gainThreshold <= dBToFactor(static_cast<F>(-60)))
        return {};

    auto itMax = spectrum.begin();
    auto maxHarmonicsGain = math::zero<F>;
    for (auto it = spectrum.begin(); it != spectrum.end(); ++it)
    {
        if (it->gain <= gainThreshold)
            continue;
        auto harmonicsGain =
          std::accumulate(it + 1, spectrum.end(), it->gain, [&](const auto accumulator, const auto &value) {
              return isHarmonic(it->frequency, value.frequency, semitoneRatio<F>) ? accumulator + value.gain :
                                                                                    value.gain;
          });
        if (harmonicsGain > maxHarmonicsGain)
        {
            itMax = it;
            maxHarmonicsGain = harmonicsGain;
        }
    }

    return *itMax;
}

template<std::floating_point F>
void envelopeAlignmentFactors(ranges::TypedInputRange<F> auto &&gains,
                              ranges::TypedInputRange<F> auto &&gainsToBeAligned,
                              ranges::TypedOutputRange<F> auto &&o_factors)
{
    const auto numGains = std::ranges::ssize(gains);
    assert(numGains == std::ranges::ssize(gainsToBeAligned) && numGains == std::ranges::ssize(o_factors));

    constexpr std::array<F, 21> c{0.0180, 0.0243, 0.0310, 0.0378, 0.0445, 0.0508, 0.0564,
                                  0.0611, 0.0646, 0.0667, 0.0675, 0.0667, 0.0646, 0.0611,
                                  0.0564, 0.0508, 0.0445, 0.0378, 0.0310, 0.0243, 0.0180};
    constexpr auto offset = std::ranges::ssize(c) / 2;

    const auto envelopeValue = [&](auto &&envGains, const auto i) {
        const auto gainsStart = std::max<int>(0, i - offset);
        const auto gainsEnd = std::min<int>(i + offset + 1, numGains);
        const auto cStart = gainsStart - i + offset;
        return std::inner_product(envGains.begin() + gainsStart, envGains.begin() + gainsEnd, c.begin() + cStart,
                                  math::zero<F>);
    };

    std::ranges::transform(std::views::iota(0, numGains), o_factors.begin(), [&](const auto i) {
        const auto envelopeToBeAligned = envelopeValue(gainsToBeAligned, i);
        if (envelopeToBeAligned == math::zero<F>)
            return math::one<F>;
        const auto envelope = envelopeValue(gains, i);
        return envelope / envelopeToBeAligned;
    });
}

}    // namespace sw

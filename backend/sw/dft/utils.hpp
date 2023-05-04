#pragma once
#include "sw/basics/math.hpp"
#include "sw/basics/ranges.hpp"
#include <cmath>
#include <vector>

namespace sw::dft {

constexpr size_t nyquistLength(const size_t signalLength)
{
    return (signalLength / 2u) + 1u;
}

constexpr size_t signalLength(const size_t nyquistLength)
{
    assert(nyquistLength > 0u);
    return 2u * (nyquistLength - 1u);
}

template<std::floating_point F>
constexpr F binFrequencyStep(const size_t signalLength, const F sampleRate)
{
    return sampleRate / static_cast<F>(signalLength);
}

template<std::floating_point F>
auto binFrequencies(const size_t fftLength, const F sampleRate)
{
    return std::views::iota(0) |
           std::views::transform([frequencyStep = binFrequencyStep<F>(fftLength, sampleRate)](const auto i) -> F {
               return static_cast<F>(i) * frequencyStep;
           });
}
template<std::floating_point F>
std::vector<F> makeBinFrequencies(const size_t fftLength, const F sampleRate, const size_t numFrequencies)
{
    const auto infiniteFrequencies = binFrequencies(fftLength, sampleRate);
    return ranges::to_vector(std::views::counted(infiniteFrequencies.begin(), static_cast<int>(numFrequencies)));
}

template<std::floating_point F>
size_t floorIndex(const F frequency, const F frequencyStep)
{
    return static_cast<size_t>(std::floor(frequency / frequencyStep));
}

template<std::floating_point F>
size_t ceilIndex(const F frequency, const F frequencyStep)
{
    return static_cast<size_t>(std::ceil(frequency / frequencyStep));
}

}    // namespace sw::dft

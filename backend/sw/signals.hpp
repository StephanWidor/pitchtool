#pragma once
#include "sw/basics/math.hpp"
#include "sw/basics/ranges.hpp"
#include <cassert>
#include <complex>
#include <numeric>
#include <random>
#include <span>
#include <vector>

namespace sw {

template<std::floating_point F>
constexpr F factorToDB(const F factor)
{
    return factor <= math::zero<F> ? -std::numeric_limits<F>::infinity() :
                                     static_cast<F>(20.0 / std::log2(10.0)) * std::log2(factor);
}

template<std::floating_point F>
constexpr F dBToFactor(const F dB)
{
    return std::pow(static_cast<F>(10), dB * static_cast<F>(0.05));
}

template<std::floating_point F>
std::vector<std::complex<F>> makeRandomComplexSignal(const F amplitude, const size_t length,
                                                     const unsigned int seed = 0u)
{
    assert(amplitude >= math::zero<F>);
    std::mt19937 generator(seed);
    std::uniform_real_distribution<F> value(math::zero<F>, math::one<F>);

    return ranges::to_vector(std::views::iota(0, static_cast<int>(length)) | std::views::transform([&](const auto) {
                                 return std::polar(amplitude * value(generator), math::twoPi<F> * value(generator));
                             }));
}

template<std::floating_point F>
std::vector<F> makeRandomRealSignal(const F amplitude, const size_t length, const unsigned int seed)
{
    assert(amplitude >= math::zero<F>);
    std::mt19937 generator(seed);
    std::uniform_real_distribution<F> value(-amplitude, amplitude);

    return ranges::to_vector(std::views::iota(0, static_cast<int>(length)) |
                             std::views::transform([&](const auto) { return value(generator); }));
}

template<std::floating_point F>
auto sineWave(const F amplitude, const F frequency, const F sampleRate)
{
    return std::views::iota(0) |
           std::views::transform([amplitude, deltaT = math::twoPi<F> * (frequency / sampleRate)](const auto i) -> F {
               return amplitude * std::sin(static_cast<F>(i) * deltaT);
           });
}
template<std::floating_point F>
std::vector<F> makeSineWave(const F amplitude, const F frequency, const F sampleRate, const size_t length)
{
    const auto infiniteSignal = sineWave(amplitude, frequency, sampleRate);
    return ranges::to_vector(std::views::counted(infiniteSignal.begin(), static_cast<int>(length)));
}

template<std::floating_point F>
auto phasor(const F amplitude, const F frequency, const F sampleRate)
{
    return std::views::iota(0) | std::views::transform([amplitude, deltaT = math::twoPi<F> * (frequency / sampleRate)](
                                                         const auto i) -> std::complex<F> {
               return std::polar(amplitude, static_cast<F>(i) * deltaT);
           });
}
template<std::floating_point F>
std::vector<std::complex<F>> makePhasor(const F amplitude, const F frequency, const F sampleRate, const size_t length)
{
    const auto infiniteSignal = phasor(amplitude, frequency, sampleRate);
    return ranges::to_vector(std::views::counted(infiniteSignal.begin(), static_cast<int>(length)));
}

template<std::floating_point F>
std::vector<F> makeDirac(const F amplitude, const size_t length)
{
    std::vector<F> signal(length, math::zero<F>);
    signal.front() = amplitude;
    return signal;
}

template<std::floating_point F>
auto cosineWindow(size_t length, const F a0)
{
    assert(length > 1u);
    return std::views::iota(0, static_cast<int>(length)) |
           std::views::transform([a0, oneMinusA0 = math::one<F> - a0,
                                  step = math::twoPi<F> / static_cast<F>(length - 1u)](const auto i) -> F {
               return a0 - oneMinusA0 * cos(static_cast<F>(i) * step);
           });
}

template<std::floating_point F>
auto vonHannWindow(size_t length)
{
    return cosineWindow<F>(length, math::oneHalf<F>);
}

template<std::floating_point F>
auto hammingWindow(size_t length)
{
    return cosineWindow<F>(length, static_cast<F>(25.0 / 46.0));
}

template<std::floating_point F>
std::vector<F> makeHammingWindow(size_t length)
{
    return ranges::to_vector(hammingWindow<F>(length));
}

template<std::floating_point F>
std::vector<F> makeVonHannWindow(size_t length)
{
    return ranges::to_vector(vonHannWindow<F>(length));
}

template<typename F>
bool equal(ranges::TypedInputRange<F> auto &&signal0, ranges::TypedInputRange<F> auto &&signal1,
           const math::underlying_t<F> tolerance = math::defaultTolerance<F>)
{
    return std::ranges::size(signal0) == std::ranges::size(signal1) &&
           std::equal(signal0.begin(), signal0.end(), signal1.begin(), signal1.end(),
                      [&](const auto s0, const auto s1) { return math::equal(s0, s1, tolerance); });
}

template<std::floating_point F>
F average(ranges::TypedInputRange<F> auto &&signal)
{
    if constexpr (std::ranges::sized_range<decltype(signal)>)
    {
        return std::ranges::empty(signal) ? math::zero<F> :
                                            std::accumulate(signal.begin(), signal.end(), math::zero<F>) /
                                              static_cast<F>(std::ranges::size(signal));
    }
    else
    {
        auto count = 0u;
        const auto sum =
          std::accumulate(signal.begin(), signal.end(), math::zero<F>, [&count](const auto s0, const auto s1) {
              ++count;
              return s0 + s1;
          });
        return count == 0u ? math::zero<F> : sum / static_cast<F>(count);
    }
}

template<std::floating_point F>
F weightedAverage(ranges::TypedInputRange<F> auto &&signal, ranges::TypedInputRange<F> auto &&weights)
{
    assert(std::ranges::size(weights) >= std::ranges::size(signal));
    const auto weightsAccumulated = std::accumulate(weights.begin(), weights.end(), math::zero<F>);
    return std::inner_product(signal.begin(), signal.end(), weights.begin(), math::zero<F>) / weightsAccumulated;
}

template<std::floating_point F>
F geometricAverage(ranges::TypedInputRange<F> auto &&signal)
{
    return std::pow(static_cast<F>(2), average<F>(std::forward<decltype(signal)>(signal) |
                                                  std::views::transform([](const auto s) { return std::log2(s); })));
}

template<std::floating_point F>
F weightedGeometricAverage(ranges::TypedInputRange<F> auto &&signal, ranges::TypedInputRange<F> auto &&weights)
{
    return std::pow(static_cast<F>(2),
                    weightedAverage<F>(std::forward<decltype(signal)>(signal) |
                                         std::views::transform([](const auto s) { return std::log2(s); }),
                                       std::forward<decltype(weights)>(weights)));
}

template<std::floating_point F>
F rms(ranges::TypedInputRange<F> auto &&signal)
{
    return std::sqrt(std::inner_product(signal.begin(), signal.end(), signal.begin(), math::zero<F>) /
                     static_cast<F>(std::ranges::size(signal)));
}

}    // namespace sw

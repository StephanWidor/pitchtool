#include <gtest/gtest.h>
#include <list>
#include <sw/basics/ranges.hpp>
#include <sw/dft/spectrum.hpp>
#include <sw/phases.hpp>

namespace sw::dft::tests {

template<typename F>
class SpectrumTest : public ::testing::Test
{};

using FloatTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(SpectrumTest, FloatTypes);

TYPED_TEST(SpectrumTest, toSpectrumByPhase)
{
    using F = TypeParam;
    const F sampleRate = static_cast<F>(48000);
    const auto fftLength = 512u;
    const F timeDiff = static_cast<F>(0.01);
    const auto nyquistLength = dft::nyquistLength(fftLength);
    const auto binFrequencies = dft::binFrequencies(fftLength, sampleRate);

    const auto check = [&](auto &&frequencies, auto &&phases, auto &&coefficients, auto &&spectrum) {
        frequencies.clear();
        std::ranges::transform(binFrequencies | std::views::take(nyquistLength), std::back_inserter(frequencies),
                               [](const auto f) { return f + math::one<F>; });
        phases.clear();
        std::ranges::transform(frequencies, std::back_inserter(phases),
                               [&](const auto frequency) { return standardized(-phaseAngle(frequency, timeDiff)); });
        coefficients.resize(nyquistLength);
        spectrum.resize(nyquistLength);

        toSpectrumByPhase(sampleRate, timeDiff, phases, coefficients, spectrum, phases);

        EXPECT_TRUE(std::ranges::all_of(phases, [](const auto phase) { return math::isZero(phase); }));
        EXPECT_TRUE(equal<F>(frequencies, sw::frequencies<F>(spectrum)));
    };

    check(std::vector<F>{}, std::vector<F>{}, std::vector<std::complex<F>>{}, std::vector<SpectrumValue<F>>{});
    check(std::list<F>{}, std::list<F>{}, std::list<std::complex<F>>{}, std::list<SpectrumValue<F>>{});
}

}    // namespace sw::dft::tests

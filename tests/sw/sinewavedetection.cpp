#include <gtest/gtest.h>
#include <sw/containers/utils.hpp>
#include <sw/dft/spectrum.hpp>
#include <sw/dft/transform.hpp>
#include <sw/signals.hpp>

namespace sw::tests {

template<std::floating_point F>
class SineWaveDetectionTest : public ::testing::Test
{};
using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(SineWaveDetectionTest, FloatTypes);

TYPED_TEST(SineWaveDetectionTest, detectSineWaves)
{
    using F = TypeParam;

    constexpr auto sampleRate = static_cast<F>(48000.0);
    constexpr auto fftLength = 4096u;
    constexpr auto nyquistLength = dft::nyquistLength(fftLength);
    constexpr auto oversampling = 4u;
    constexpr auto stepSize = fftLength / oversampling;
    constexpr auto timeDiff = static_cast<F>(stepSize) / sampleRate;
    constexpr auto signalLength = fftLength + stepSize;

    auto binCoefficients = containers::makeFilledArray<2>(std::vector<std::complex<F>>(nyquistLength, math::zero<F>));
    std::vector<SpectrumValue<F>> spectrum(nyquistLength);
    std::vector<F> dummyPhases(nyquistLength);

    dft::FFT<F> fft(fftLength);
    constexpr auto zeroGainThresholdLinear = dBToFactor(static_cast<F>(-30));
    constexpr auto useOMP = true;

    for (auto frequency = static_cast<F>(100); frequency < math::oneHalf<F> * sampleRate;
         frequency += static_cast<F>(100))
    {
        const auto signal = makeSineWave<F>(static_cast<F>(1), frequency, sampleRate, signalLength);

        fft.transform(std::span(signal.begin(), fftLength), binCoefficients.front(), useOMP);
        const auto lastBinPhases = binCoefficients.front() | std::views::transform([](const auto &c) {
                                       return math::isZero(c) ? math::zero<F> : std::arg(c);
                                   });

        fft.transform(std::span(signal.begin() + stepSize, fftLength), binCoefficients.back(), useOMP);

        spectrum.resize(std::ranges::size(lastBinPhases));
        dft::toSpectrumByPhase<F>(sampleRate, timeDiff, lastBinPhases, binCoefficients.back(), spectrum, dummyPhases);
        removeSmallGains(spectrum, zeroGainThresholdLinear);
        identifyFrequencies<F>(spectrum);

        const auto backFrequency = findFundamental<F>(spectrum).frequency;
        const auto frequencyRatio = math::maxRatio(frequency, backFrequency);

        EXPECT_LE(frequencyRatio, sqrtSemitoneRatio<F>);
    }
}

}    // namespace sw::tests

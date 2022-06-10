#if __has_include(<matplotlib-cpp/matplotlibcpp.h>)

#include <gtest/gtest.h>
#include <sw/containers/utils.hpp>
#include <sw/pitchtool/processor.hpp>
#include <sw/signals.hpp>
#include <sw/spectrum.hpp>

#define WITHOUT_NUMPY
#include <matplotlib-cpp/matplotlibcpp.h>

namespace sw::dft::tests {

TEST(PitchProcessorTest, plotting)
{
    namespace plt = matplotlibcpp;

    constexpr auto sampleRate = 48000.0;
    constexpr auto fftLength = 2048u;
    constexpr auto nyquistLength = dft::nyquistLength(fftLength);
    constexpr auto oversampling = 4u;
    constexpr auto stepSize = fftLength / oversampling;
    constexpr auto numTransforms = 10u;
    constexpr auto timeDiff = static_cast<double>(stepSize) / sampleRate;
    constexpr auto signalLength = numTransforms * stepSize;
    const auto binFrequencyStep = dft::binFrequencyStep(fftLength, sampleRate);

    constexpr auto doPlotSpectrum = true;

    using Processor = pitchtool::Processor<double, 1>;

    pitchtool::TuningParameters<double> tuneParameters;
    pitchtool::ChannelParameters<double> channelParameters{std::monostate{}, 12.0, 12.0, 1.0};

    Processor processor(fftLength, oversampling);

    const auto plotSpectrum = [&](const auto frequency) {
        const auto pitchedFrequency = semitonesToFactor(channelParameters.pitchShift) * frequency;
        const auto [minFreq, maxFreq] = std::minmax(frequency, pitchedFrequency);

        plt::figure_size(1000u, 800u);
        plt::xlim(std::max(0.0, minFreq - 500.0), std::min(0.5 * sampleRate, maxFreq + 500.0));
        plt::bar(ranges::to_vector(frequencies<double>(processor.inSpectrum())),
                 ranges::to_vector(gains<double>(processor.inSpectrum())), "r");
        plt::bar(ranges::to_vector(frequencies<double>(processor.outSpectrum(0))),
                 ranges::to_vector(gains<double>(processor.outSpectrum(0))), "b");
        plt::show();
    };

    std::vector<double> stepOutSignal(processor.stepSize());
    for (auto frequency = 100.0; frequency < 0.5 * sampleRate; frequency += 500.0)
    {
        auto signal = makeSineWave<double>(1.0, frequency, sampleRate, signalLength);

        std::vector<double> outSignal;
        for (auto i = 0u; i < numTransforms; ++i)
        {
            processor.process(std::span(signal.begin() + (i * stepSize), stepSize), stepOutSignal, sampleRate,
                              tuneParameters, {channelParameters}, 0.0);
            outSignal.insert(outSignal.end(), stepOutSignal.begin(), stepOutSignal.end());

            if (doPlotSpectrum)
                plotSpectrum(frequency);
        }

        plt::figure_size(1000u, 800u);
        plt::plot(signal, "r");
        plt::plot(outSignal, "b");
        plt::show();
    }
}

}    // namespace sw::dft::tests

#endif

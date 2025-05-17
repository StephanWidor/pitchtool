#undef MATPLOTLIB_HEADER
#if __has_include(<matplotlibcpp.h>)
#define MATPLOTLIB_HEADER <matplotlibcpp.h>
#elif __has_include(<matplotlib-cpp/matplotlibcpp.h>)
#define MATPLOTLIB_HEADER <matplotlib-cpp/matplotlibcpp.h>
#endif

#ifdef MATPLOTLIB_HEADER

#include <gtest/gtest.h>
#include <sw/containers/utils.hpp>
#include <sw/pitchtool/processor.hpp>
#include <sw/signals.hpp>
#include <sw/spectrum.hpp>

#define WITHOUT_NUMPY
#include MATPLOTLIB_HEADER

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

    constexpr auto doPlotSpectrum = false;

    using Processor = pitchtool::Processor<double, 1>;

    pitchtool::TuningParameters<double> tuneParameters;
    pitchtool::ChannelParameters<double> channelParameters{std::monostate{}, 12.0, 12.0, 1.0};

    Processor processor(fftLength, oversampling);

    const auto plotSpectrum = [&](const auto frequency) {
        const auto pitchedFrequency = semitonesToFactor(channelParameters.pitchShift) * frequency;
        const auto [minFreq, maxFreq] = std::minmax(frequency, pitchedFrequency);

        plt::figure_size(1000u, 800u);
        plt::xlim(std::max(0.0, minFreq - 500.0), std::min(0.5 * sampleRate, maxFreq + 500.0));
        plt::bar(std::ranges::to<std::vector>(frequencies<double>(processor.inputSpectrum())),
                 std::ranges::to<std::vector>(gains<double>(processor.inputSpectrum())), "r");
        plt::bar(std::ranges::to<std::vector>(frequencies<double>(processor.outputSpectrum(0))),
                 std::ranges::to<std::vector>(gains<double>(processor.outputSpectrum(0))), "b");
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

TEST(PitchProcessorTest, plot_different_parameters)
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

    using Processor = pitchtool::Processor<double, 1>;

    pitchtool::TuningParameters<double> tuneParameters{440.0, 0.0, 0.0, 0.0};
    std::vector<pitchtool::ChannelParameters<double>> channelParameters{{std::monostate{}, 0.0, 0.0, 1.0},
                                                                        {std::monostate{}, 12.0, 0.0, 1.0},
                                                                        {std::monostate{}, 12.0, 7.0, 1.0},
                                                                        {std::monostate{}, 12.0, 12.0, 1.0},
                                                                        {std::monostate{}, 0.0, 0.0, 1.0}};

    const auto minBin = 4.0;
    const auto maxBin = 5.0;
    const auto numFrequencies = 20u;
    const auto minFrequency = minBin * binFrequencyStep;
    const auto maxFrequency = maxBin * binFrequencyStep;
    const auto frequencyStep = (maxFrequency - minFrequency) / static_cast<double>(numFrequencies - 1u);
    for (auto frequencyIndex = 0u; frequencyIndex < numFrequencies; ++frequencyIndex)
    {
        const auto frequency = minFrequency + static_cast<double>(frequencyIndex) * frequencyStep;
        const auto signal = makeSineWave<double>(1.0, frequency, sampleRate, channelParameters.size() * signalLength);

        Processor processor(fftLength, oversampling);
        std::vector<double> stepOutSignal(processor.stepSize());
        std::vector<double> outSignal;

        auto signalIt = signal.begin();
        for (auto paramIndex = 0; paramIndex < channelParameters.size(); ++paramIndex)
        {
            const auto &parameters = channelParameters[paramIndex];
            for (auto stepIndex = 0u; stepIndex < numTransforms; ++stepIndex)
            {
                processor.process(std::span(signalIt, stepSize), stepOutSignal, sampleRate, tuneParameters,
                                  {parameters}, 0.0);
                outSignal.insert(outSignal.end(), stepOutSignal.begin(), stepOutSignal.end());
                signalIt += stepSize;
            }
        }

        plt::figure_size(1000u, 800u);
        plt::plot(signal, "r");
        plt::plot(outSignal, "g");
        plt::title(std::to_string(frequencyIndex) + ", " + std::to_string(frequency));
        plt::show();
    }
}

}    // namespace sw::dft::tests

#endif

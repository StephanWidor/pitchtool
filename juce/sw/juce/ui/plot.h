#pragma once
#include <array>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include <sw/containers/utils.hpp>
#include <sw/ranges/utils.hpp>
#include <sw/signals.hpp>

namespace sw::juce::ui::plot {

enum class DrawType
{
    LineConnected,
    LinesFromBottom
};

class Graph
{
public:
    Graph(const ::juce::Colour &lineColor, DrawType drawType = DrawType::LineConnected, float strokeThickness = 1.0f,
          size_t initialSize = 1024u);

    bool setValues(ranges::TypedInputRange<float> auto &&xValues, ranges::TypedInputRange<float> auto &&yValues)
    {
        const auto numX = std::ranges::size(xValues), numY = std::ranges::size(yValues);
        if (numX != numY)
            return false;
        m_xValues.resize(numX);
        m_yValues.resize(numY);
        setXValues(xValues);
        setYValues(yValues);
        return true;
    }

    bool setXValues(ranges::TypedInputRange<float> auto &&xValues)
    {
        const auto numValues = static_cast<size_t>(std::ranges::size(xValues));
        if (numValues != m_xValues.size())
            return false;
        std::copy(xValues.begin(), xValues.end(), m_xValues.begin());
        return true;
    }

    bool setYValues(ranges::TypedInputRange<float> auto &&yValues)
    {
        const auto numValues = static_cast<size_t>(std::ranges::size(yValues));
        if (numValues != m_yValues.size())
            return false;
        std::copy(yValues.begin(), yValues.end(), m_yValues.begin());
        return true;
    }

    bool setAllYValues(float value)
    {
        for (auto &v : m_yValues)
            v = value;
        return true;
    }

    template<ranges::TypedInputRange<float> R>
    void pushYValues(R &&yValues)
    {
        containers::ringPush(m_yValues, std::forward<R>(yValues));
    }

    void paint(::juce::Graphics &, const ::juce::Range<float> &xRange, const ::juce::Range<float> &yRange,
               const ::juce::Rectangle<float> &plotArea);

private:
    std::vector<float> m_xValues, m_yValues;
    ::juce::Path m_path;
    ::juce::Colour m_lineColor{::juce::Colours::white};
    DrawType m_drawType{DrawType::LineConnected};
    ::juce::PathStrokeType m_strokeType;
};

namespace signal {

constexpr size_t numBlocks(const size_t signalLength, const size_t blockSize)
{
    assert(signalLength > blockSize && blockSize > 0u);
#ifdef __GNUC__
    return static_cast<size_t>(std::ceil(static_cast<float>(signalLength) / static_cast<float>(blockSize)));
#else
    const auto div = static_cast<float>(signalLength) / static_cast<float>(blockSize);
    const auto sDiv = static_cast<size_t>(div);
    if (static_cast<float>(sDiv) == div)
        return sDiv;
    else
        return sDiv + 1;
#endif
}

inline ::juce::Range<float> xRange(const size_t signalLength, const size_t blockSize)
{
    return ::juce::Range<float>(0.0f, static_cast<float>(signal::numBlocks(signalLength, blockSize) - 1));
}

inline ::juce::Range<float> yRange()
{
    return ::juce::Range<float>(-1.0f, 1.0f);
}

template<ranges::TypedInputRange<float> R>
auto blockSignal(R &&signal, const size_t blockSize)
{
    const auto numSamples = static_cast<size_t>(std::ranges::size(signal));

    auto blockView = [blockSize, numSamples, &signal](const size_t blockIndex) {
        const auto startIndex = blockIndex * blockSize;
        const auto effectiveBlockSize = std::min(blockSize, numSamples - startIndex);
        return std::views::counted(signal.begin() + static_cast<int>(startIndex), static_cast<int>(effectiveBlockSize));
    };

    auto maxAbs = [](auto signalBlock) {
        return *std::ranges::max_element(signalBlock,
                                         [](const auto f0, const auto f1) { return (std::abs(f0) < std::abs(f1)); });
    };

    return std::views::iota(0, static_cast<int>(numBlocks(numSamples, blockSize))) | std::views::transform(blockView) |
           std::views::transform(maxAbs);
}

}    // namespace signal

namespace spectrum {

inline ::juce::Range<float> xRange(bool logScale)
{
    return logScale ? ::juce::Range<float>(std::log2(20.0f), std::log2(24000.0f)) :
                      ::juce::Range<float>(0.0f, 24000.0f);
}

inline ::juce::Range<float> yRange(bool logScale)
{
    return logScale ? ::juce::Range<float>(-60.0f, 0.0f) : ::juce::Range<float>(0.0f, 1.0f);
}

template<ranges::TypedInputRange<float> R>
inline auto xValues(R &&frequencies, bool logScale)
{
    const auto xValue =
      logScale ? [](const float f) { return (f > 0.0f) ? std::log2(f) : -std::numeric_limits<float>::infinity(); } :
                 [](const float f) { return f; };
    return std::forward<R>(frequencies) | std::views::transform(xValue);
}

template<ranges::TypedInputRange<float> R>
inline auto yValues(R &&gains, bool logScale)
{
    const auto yValue = logScale ? [](const float g) { return factorToDB(g); } : [](const float g) { return g; };
    return std::forward<R>(gains) | std::views::transform(yValue);
}

}    // namespace spectrum

class Plot : public ::juce::Component
{
public:
    Plot(const ::juce::Range<float> &xRange, const ::juce::Range<float> &yRange, std::vector<Graph> &&);

    void setRanges(const ::juce::Range<float> &xRange, const ::juce::Range<float> &yRange);

    void paint(::juce::Graphics &) override;

    enum ColourIds
    {
        backgroundColourId = ::juce::TextEditor::backgroundColourId,
        borderColourId = ::juce::TextEditor::outlineColourId
    };

    std::vector<Graph> graphs;

private:
    ::juce::Range<float> m_xRange, m_yRange;
    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plot)
};

}    // namespace sw::juce::ui::plot

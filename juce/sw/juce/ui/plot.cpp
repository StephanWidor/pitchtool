#include "sw/juce/ui/plot.h"
#include <cassert>

sw::juce::ui::plot::Graph::Graph(const ::juce::Colour &lineColor, const DrawType drawType, const float strokeThickness,
                                 const size_t initialSize)
    : m_xValues(ranges::to_vector(std::views::iota(0, static_cast<int>(initialSize)) |
                                  std::views::transform([](const auto i) { return static_cast<float>(i); })))
    , m_yValues(initialSize, 0.0f)
    , m_lineColor(lineColor)
    , m_drawType(drawType)
    , m_strokeType(strokeThickness)
{}

void sw::juce::ui::plot::Graph::paint(::juce::Graphics &juceGraphics, const ::juce::Range<float> &xRange,
                                      const ::juce::Range<float> &yRange, const ::juce::Rectangle<float> &plotArea)
{
    const auto updatedPath = [&]() -> const ::juce::Path & {
        const auto makePlotX = [&](const float x) {
            const auto clampedX = std::clamp(x, xRange.getStart(), xRange.getEnd());
            return plotArea.getTopLeft().getX() +
                   (clampedX - xRange.getStart()) * plotArea.getWidth() / xRange.getLength();
        };

        const auto makePlotY = [&](const float y) {
            const auto clampedY = std::clamp(y, yRange.getStart(), yRange.getEnd());
            return plotArea.getBottomRight().getY() -
                   (clampedY - yRange.getStart()) * plotArea.getHeight() / yRange.getLength();
        };

        const auto makePlotPoint = [&](const float x, const float y) {
            return ::juce::Point<float>(makePlotX(x), makePlotY(y));
        };

        m_path.clear();
        m_path.preallocateSpace(m_drawType == DrawType::LineConnected ? m_xValues.size() : 2 * m_xValues.size());
        if (m_drawType == DrawType::LineConnected)
        {
            m_path.startNewSubPath(makePlotPoint(m_xValues.front(), m_yValues.front()));
            for (auto i = 1u; i < m_xValues.size(); ++i)
                m_path.lineTo(makePlotPoint(m_xValues[i], m_yValues[i]));
        }
        else
        {
            const auto lowerY = makePlotY(yRange.getStart());
            for (auto i = 0u; i < m_xValues.size(); ++i)
            {
                const auto plotX = makePlotX(m_xValues[i]);
                m_path.startNewSubPath(plotX, lowerY);
                m_path.lineTo(plotX, makePlotY(m_yValues[i]));
            }
        }
        return m_path;
    };

    juceGraphics.setColour(m_lineColor);
    juceGraphics.strokePath(updatedPath(), m_strokeType);
}

sw::juce::ui::plot::Plot::Plot(const ::juce::Range<float> &xRange, const ::juce::Range<float> &yRange,
                               std::vector<Graph> &&_graphs)
    : graphs(std::forward<std::vector<Graph>>(_graphs)), m_xRange(xRange), m_yRange(yRange)
{
    setOpaque(true);
}

void sw::juce::ui::plot::Plot::setRanges(const ::juce::Range<float> &xRange, const ::juce::Range<float> &yRange)
{
    m_xRange = xRange;
    m_yRange = yRange;
}

void sw::juce::ui::plot::Plot::paint(::juce::Graphics &juceGraphics)
{
    juceGraphics.fillAll(getLookAndFeel().findColour(backgroundColourId));
    const auto plotArea = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
    for (auto &graph : graphs)
        graph.paint(juceGraphics, m_xRange, m_yRange, plotArea);
}

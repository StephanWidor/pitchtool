#include "sw/juce/ui/roundslider.h"

#include <cmath>

sw::juce::ui::RoundSlider::RoundSlider(const std::string &labelText)
    : ::juce::Slider(::juce::Slider::RotaryVerticalDrag, ::juce::Slider::TextBoxBelow), m_label("", labelText)
{
    addAndMakeVisible(m_label);
    m_label.setJustificationType(::juce::Justification::horizontallyCentred | ::juce::Justification::verticallyCentred);
    m_label.setInterceptsMouseClicks(false, false);
}

void sw::juce::ui::RoundSlider::resized()
{
    ::juce::Slider::resized();
    m_label.setBounds(getLookAndFeel().getSliderLayout(*this).sliderBounds);
}

double sw::juce::ui::RoundSlider::snapValue(const double attemptedValue, const DragMode dragMode)
{
    if (dragMode != notDragging)
        return m_snapFunction(attemptedValue);
    return attemptedValue;
}

void sw::juce::ui::RoundSlider::setSnapFunction(const std::function<double(double)> function)
{
    m_snapFunction = function;
}

std::function<double(double)> sw::juce::ui::RoundSlider::noSnap()
{
    return [](const double d) { return d; };
}

std::function<double(double)> sw::juce::ui::RoundSlider::snapToMultiples(const double value, const double snapEps)
{
    return [value, snapEps](const double d) {
        const auto rounded = std::round(d / value) * value;
        return std::abs(rounded - d) <= snapEps ? rounded : d;
    };
}

void sw::juce::ui::RoundSlider::setLabelText(const std::string &text)
{
    m_label.setText(text, ::juce::dontSendNotification);
}

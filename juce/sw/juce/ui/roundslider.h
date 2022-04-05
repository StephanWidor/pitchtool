#pragma once
#include <functional>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace sw::juce::ui {

class RoundSlider : public ::juce::Slider
{
public:
    RoundSlider(const std::string &labelText);

    void resized() override;

    double snapValue(double, DragMode) override;

    void setSnapFunction(std::function<double(double)>);
    static std::function<double(double)> noSnap();
    static std::function<double(double)> snapToMultiples(double value, double snapEps);

    void setLabelText(const std::string &text);

private:
    ::juce::Label m_label;

    std::function<double(double)> m_snapFunction = noSnap();
};

}    // namespace sw::juce::ui
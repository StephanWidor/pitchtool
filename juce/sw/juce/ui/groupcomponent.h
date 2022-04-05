#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sw::juce::ui {

class GroupComponent : public ::juce::Component
{
public:
    GroupComponent(const std::string &label, float cornerSize = 10.0f, bool drawBorder = false);

    void paint(::juce::Graphics &) override;

    void resized() override;

    enum ColourIds
    {
        borderColourId = ::juce::TextEditor::textColourId
    };

private:
    ::juce::Label m_label;
    float m_cornerSize{10.0f};
    bool m_drawBorder{false};

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plot)
};

}    // namespace sw::juce::ui

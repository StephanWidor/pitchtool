#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sw::juce::ui {

class GroupComponent : public ::juce::Component
{
public:
    GroupComponent(const std::string &label, float cornerSize = 5.0f, bool drawBorder = false);

    void paint(::juce::Graphics &) override;

    void resized() override;

    enum ColourIds
    {
        borderColourId = ::juce::TextEditor::outlineColourId
    };

private:
    ::juce::Label m_label;
    float m_cornerSize{5.0f};
    bool m_drawBorder{false};
};

}    // namespace sw::juce::ui

#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sw::juce::ui {

class GroupComponent : public ::juce::Component
{
public:
    static constexpr int defaultCornerSize = 5;

    GroupComponent(const std::string &label, int cornerSize = defaultCornerSize, bool drawBorder = false);

    void paint(::juce::Graphics &) override;

    void resized() override;

    enum ColourIds
    {
        borderColourId = ::juce::TextEditor::outlineColourId
    };

private:
    ::juce::Label m_label;
    int m_cornerSize{defaultCornerSize};
    bool m_drawBorder{false};
};

}    // namespace sw::juce::ui

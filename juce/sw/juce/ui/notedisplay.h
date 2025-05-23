#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <sw/notes.hpp>
#include <sw/spectrum.hpp>

namespace sw::juce::ui {

class NoteDisplay : public ::juce::Component
{
public:
    enum class Layout
    {
        Horizontal,
        Vertical
    };

    NoteDisplay(Layout, float cornerSize = 5.0f);

    void set(float frequency, float standardPitch);

    void set(const Note &, float semitoneDeviation);

    void paint(::juce::Graphics &) override;

    void resized() override;

    enum ColourIds
    {
        backgroundColourId = ::juce::TextEditor::backgroundColourId,
        borderColourId = ::juce::TextEditor::outlineColourId
    };

private:
    ::juce::Rectangle<float> setLabelBounds();
    Note m_note;
    float m_semitoneDeviation{0.0f};
    Layout m_layout{Layout::Horizontal};
    float m_cornerSize{5.0f};
    ::juce::Label m_label;
};

}    // namespace sw::juce::ui

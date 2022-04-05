#include "sw/juce/ui/notedisplay.h"

sw::juce::ui::NoteDisplay::NoteDisplay(Layout layout): m_layout(layout)
{
    m_label.setJustificationType(::juce::Justification::centred);
    m_label.setMinimumHorizontalScale(0.1f);
    addAndMakeVisible(m_label);
}

::juce::Rectangle<float> sw::juce::ui::NoteDisplay::setLabelBounds()
{
    if (m_note.name != sw::Note::Name::Invalid)
    {
        const auto nameIndex = static_cast<float>(m_note.name);
        if (m_layout == Layout::Horizontal)
        {
            const auto width = getWidth() / 12.0f;
            m_label.setBounds(getLocalBounds().withX(nameIndex * width).withWidth(width).toNearestInt());
        }
        else
        {
            const auto height = getHeight() / 12.0f;
            m_label.setBounds(getLocalBounds().withY(nameIndex * height).withHeight(height).toNearestInt());
        }
    }
    return m_label.getBounds().toFloat();
}

void sw::juce::ui::NoteDisplay::set(const float frequency, const float standardPitch)
{
    float deviation = 0.0f;
    const auto note = sw::toNote(frequency, standardPitch, &deviation);
    set(note, deviation);
}

void sw::juce::ui::NoteDisplay::set(const sw::Note &note, const float semitoneDeviation)
{
    m_note = note;
    m_semitoneDeviation = semitoneDeviation;
    setLabelBounds();
    m_label.setText(sw::toString(m_note), ::juce::dontSendNotification);
    repaint();
}

void sw::juce::ui::NoteDisplay::paint(::juce::Graphics &graphics)
{
    graphics.fillAll(getLookAndFeel().findColour(backgroundColourId));

    if (m_note.name == Note::Name::Invalid)
        return;

    const auto localBounds = getLocalBounds().toFloat();
    const auto center = localBounds.getCentre();

    if (m_layout == Layout::Horizontal)
    {
        const auto outerX = std::clamp(center.getX() + 0.5f * localBounds.getWidth() * m_semitoneDeviation,
                                       localBounds.getX(), localBounds.getRight());
        if (math::equal(outerX, center.getX(), 1.0f))
        {
            graphics.setColour(::juce::Colours::green);
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(center.getX() - 1.0f, localBounds.getY()),
                                                   ::juce::Point<float>(center.getX() + 1.0f, localBounds.getBottom()));
            graphics.fillRect(deviationRect);
        }
        else
        {
            graphics.setGradientFill(::juce::ColourGradient(::juce::Colours::green, center, ::juce::Colours::red,
                                                            ::juce::Point<float>(outerX, center.getY()), false));
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(center.getX(), localBounds.getY()),
                                                   ::juce::Point<float>(outerX, localBounds.getBottom()));
            graphics.fillRect(deviationRect);
        }
    }
    else
    {
        const auto outerY = std::clamp(center.getY() - 0.5f * localBounds.getHeight() * m_semitoneDeviation,
                                       localBounds.getY(), localBounds.getBottom());
        if (math::equal(outerY, center.getY(), 1.0f))
        {
            graphics.setColour(::juce::Colours::green);
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(localBounds.getX(), center.getY() - 1.0f),
                                                   ::juce::Point<float>(localBounds.getRight(), center.getY() + 1.0f));
            graphics.fillRect(deviationRect);
        }
        else
        {
            graphics.setGradientFill(::juce::ColourGradient(::juce::Colours::green, center, ::juce::Colours::red,
                                                            ::juce::Point<float>(center.getX(), outerY), false));
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(localBounds.getX(), center.getY()),
                                                   ::juce::Point<float>(localBounds.getRight(), outerY));
            graphics.fillRect(deviationRect);
        }
    }
}

void sw::juce::ui::NoteDisplay::resized()
{
    const auto labelBounds = setLabelBounds();
    const auto fontSize = std::min(0.8f * labelBounds.getHeight(), 0.4f * labelBounds.getWidth());
    m_label.setFont(::juce::Font(fontSize, ::juce::Font::bold));
}

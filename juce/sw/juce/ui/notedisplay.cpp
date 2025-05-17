#include "sw/juce/ui/notedisplay.h"

sw::juce::ui::NoteDisplay::NoteDisplay(Layout layout, float cornerSize): m_layout(layout), m_cornerSize(cornerSize)
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
            m_label.setBounds(getLocalBounds().withY((11.0f - nameIndex) * height).withHeight(height).toNearestInt());
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
    const auto localBounds = getLocalBounds().toFloat();

    graphics.setColour(getLookAndFeel().findColour(backgroundColourId));
    graphics.fillRoundedRectangle(localBounds, m_cornerSize);

    constexpr auto strokeWidth = 2.0f;
    graphics.setColour(getLookAndFeel().findColour(NoteDisplay::borderColourId));
    graphics.drawRoundedRectangle(getLocalBounds().toFloat(), m_cornerSize, strokeWidth);

    if (m_note.name == Note::Name::Invalid)
        return;

    const auto center = localBounds.getCentre();

    if (m_layout == Layout::Horizontal)
    {
        const auto drawBounds = localBounds.reduced(std::max(m_cornerSize, strokeWidth), strokeWidth);
        const auto outerX = std::clamp(center.getX() + drawBounds.getWidth() * m_semitoneDeviation, drawBounds.getX(),
                                       drawBounds.getRight());
        if (math::equal(outerX, center.getX(), 1.0f))
        {
            graphics.setColour(::juce::Colours::green);
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(center.getX() - 1.0f, drawBounds.getY()),
                                                   ::juce::Point<float>(center.getX() + 1.0f, drawBounds.getBottom()));
            graphics.fillRect(deviationRect);
        }
        else
        {
            graphics.setGradientFill(::juce::ColourGradient(::juce::Colours::green, center, ::juce::Colours::red,
                                                            ::juce::Point<float>(outerX, center.getY()), false));
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(center.getX(), drawBounds.getY()),
                                                   ::juce::Point<float>(outerX, drawBounds.getBottom()));
            graphics.fillRect(deviationRect);
        }
    }
    else
    {
        const auto drawBounds = localBounds.reduced(strokeWidth, std::max(m_cornerSize, strokeWidth));
        const auto outerY = std::clamp(center.getY() - drawBounds.getHeight() * m_semitoneDeviation, drawBounds.getY(),
                                       drawBounds.getBottom());
        if (math::equal(outerY, center.getY(), 1.0f))
        {
            graphics.setColour(::juce::Colours::green);
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(drawBounds.getX(), center.getY() - 1.0f),
                                                   ::juce::Point<float>(drawBounds.getRight(), center.getY() + 1.0f));
            graphics.fillRect(deviationRect);
        }
        else
        {
            graphics.setGradientFill(::juce::ColourGradient(::juce::Colours::green, center, ::juce::Colours::red,
                                                            ::juce::Point<float>(center.getX(), outerY), false));
            ::juce::Rectangle<float> deviationRect(::juce::Point<float>(drawBounds.getX(), center.getY()),
                                                   ::juce::Point<float>(drawBounds.getRight(), outerY));
            graphics.fillRect(deviationRect);
        }
    }
}

void sw::juce::ui::NoteDisplay::resized()
{
    const auto labelBounds = setLabelBounds();
    const auto fontSize = std::min(labelBounds.getHeight(), 0.4f * labelBounds.getWidth());
    m_label.setFont(::juce::FontOptions(fontSize, ::juce::Font::bold));
}

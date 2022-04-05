#include "sw/juce/ui/groupcomponent.h"

sw::juce::ui::GroupComponent::GroupComponent(const std::string &label, const float cornerSize, const bool drawBorder)
    : m_label("", label), m_cornerSize(cornerSize), m_drawBorder(drawBorder)
{
    m_label.setJustificationType(::juce::Justification::topLeft);
    addAndMakeVisible(m_label);
}

void sw::juce::ui::GroupComponent::paint(::juce::Graphics &graphics)
{
    if (m_drawBorder)
    {
        graphics.setColour(getLookAndFeel().findColour(GroupComponent::borderColourId));
        graphics.drawRoundedRectangle(getLocalBounds().toFloat(), m_cornerSize, 1.0f);
    }
}

void sw::juce::ui::GroupComponent::resized()
{
    m_label.setBounds(m_cornerSize, m_cornerSize, getWidth(), m_label.getFont().getHeight());
}
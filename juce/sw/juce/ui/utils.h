#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <sw/basics/ranges.hpp>

namespace sw::juce::ui {

template<typename... Components>
void layoutHorizontal(const ::juce::Rectangle<float> &bounds, const float componentsDistance,
                      ::juce::Component &component, Components &...components)
{
    constexpr auto numOtherComponents = static_cast<int>(sizeof...(Components));
    const auto componentWidth = (bounds.getWidth() - static_cast<float>(numOtherComponents) * componentsDistance) /
                                static_cast<float>(numOtherComponents + 1);
    component.setBounds(bounds.withWidth(componentWidth).toNearestInt());
    if constexpr (numOtherComponents > 0)
    {
        const auto shift = componentWidth + componentsDistance;
        layoutHorizontal(bounds.withX(bounds.getX() + shift).withWidth(bounds.getWidth() - shift), componentsDistance,
                         components...);
    }
}

void layoutHorizontal(const ::juce::Rectangle<float> &bounds, const float componentsDistance,
                      ::juce::Array<::juce::Component *> components)
{
    const auto numComponents = components.size();
    if (numComponents < 1)
        return;
    const auto componentWidth = (bounds.getWidth() - static_cast<float>(numComponents - 1) * componentsDistance) /
                                static_cast<float>(numComponents);
    const auto templateBounds = bounds.withWidth(componentWidth);
    const auto componentWidthPlusDist = componentWidth + componentsDistance;
    for (int i = 0; i < numComponents; ++i)
        components[i]->setBounds(
          templateBounds.translated(static_cast<float>(i) * componentWidthPlusDist, 0.0f).toNearestInt());
}

template<typename... Components>
void layoutVertical(const ::juce::Rectangle<float> &bounds, const float componentsDistance,
                    ::juce::Component &component, Components &...components)
{
    constexpr auto numOtherComponents = static_cast<int>(sizeof...(Components));
    const auto componentHeight = (bounds.getHeight() - static_cast<float>(numOtherComponents) * componentsDistance) /
                                 static_cast<float>(numOtherComponents + 1);
    component.setBounds(bounds.withHeight(componentHeight).toNearestInt());
    if constexpr (numOtherComponents > 0)
    {
        const auto shift = componentHeight + componentsDistance;
        layoutHorizontal(bounds.withY(bounds.getY() + shift).withHeight(bounds.getHeight() - shift), componentsDistance,
                         components...);
    }
}

void layoutVertical(const ::juce::Rectangle<float> &bounds, const float componentsDistance,
                    ::juce::Array<::juce::Component *> components)
{
    const auto numComponents = components.size();
    if (numComponents < 1)
        return;
    const auto componentHeight = (bounds.getHeight() - static_cast<float>(numComponents - 1) * componentsDistance) /
                                 static_cast<float>(numComponents);
    const auto templateBounds = bounds.withHeight(componentHeight);
    const auto componentHeightPlusDist = componentHeight + componentsDistance;
    for (int i = 0; i < numComponents; ++i)
        components[i]->setBounds(
          templateBounds.translated(0.0f, static_cast<float>(i) * componentHeightPlusDist).toNearestInt());
}

::juce::Array<::juce::Component *> toJucePointerArray(ranges::TypedRange<::juce::Component> auto &&components)
{
    ::juce::Array<::juce::Component *> juceArray;
    for (auto &component : components)
        juceArray.add(&component);
    return juceArray;
}

}    // namespace sw::juce::ui
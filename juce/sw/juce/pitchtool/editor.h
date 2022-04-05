#pragma once
#include "sw/juce/pitchtool/processor.h"
#include <sw/basics/chrono.hpp>
#include <sw/juce/ui/groupcomponent.h>
#include <sw/juce/ui/notedisplay.h>
#include <sw/juce/ui/plot.h>
#include <sw/juce/ui/roundslider.h>

namespace sw::juce::pitchtool {

class PlotComponent : public ui::GroupComponent
{
public:
    PlotComponent(Processor &);

    void updatePlots();

    void resized() override;

private:
    Processor &m_processor;

    size_t m_signalPlotBlockSize{20u};
    size_t m_signalPlotLength{48000u};
    ui::plot::Plot m_signalPlot;
    ui::plot::Plot m_spectrumPlot;
    ::juce::Label m_logScaleLabel{"", "Log Scale: "};
    ::juce::ToggleButton m_frequenciesLogScaleButton{"Frequencies"};
    ::juce::ToggleButton m_gainsLogScaleButton{"Gains"};

    ::juce::AudioProcessorValueTreeState::ButtonAttachment m_frequenciesLogScaleAttachment;
    ::juce::AudioProcessorValueTreeState::ButtonAttachment m_gainsLogScaleAttachment;
};

class TuningComponent : public ui::GroupComponent
{
public:
    TuningComponent(::juce::AudioProcessorValueTreeState &);

    void resized() override;

    void setFrequency(float frequency, float standardPitch);

private:
    ui::RoundSlider m_standardPitchSlider{"Standard\nPitch"};
    ui::RoundSlider m_frequencyAveragingTimeSlider{"Averaging\nTime"};
    ui::RoundSlider m_attackTimeSlider{"Attack\nTime"};
    ui::NoteDisplay m_noteDisplay{ui::NoteDisplay::Layout::Vertical};

    ::juce::AudioProcessorValueTreeState::SliderAttachment m_standardPitchAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_frequencyAveragingTimeAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_attackTimeAttachment;

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plot)
};

class ChannelComponent : public ui::GroupComponent
{
public:
    ChannelComponent(::juce::AudioProcessorValueTreeState &, size_t channel);

    void resized() override;

    void setFrequency(float frequency, float standardPitch);

private:
    ::juce::ToggleButton m_autoTuneButton{"Auto Tune"};
    ui::RoundSlider m_pitchShiftSlider{"Pitch"};
    ui::RoundSlider m_formantsShiftSlider{"Formants"};
    ui::NoteDisplay m_noteDisplay{ui::NoteDisplay::Layout::Vertical};

    ::juce::AudioProcessorValueTreeState::ButtonAttachment m_autoTuneAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_pitchShiftAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_formantsShiftAttachment;

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plot)
};

class MixComponent : public ui::GroupComponent
{
public:
    MixComponent(::juce::AudioProcessorValueTreeState &);

    void resized() override;

    const std::array<ui::RoundSlider, Processor::NumChannels> &channelSliders() const { return m_channelSliders; }
    std::array<ui::RoundSlider, Processor::NumChannels> &channelSliders() { return m_channelSliders; }

private:
    ui::RoundSlider m_drySlider{"Dry"};
    std::array<ui::RoundSlider, Processor::NumChannels> m_channelSliders;

    ::juce::AudioProcessorValueTreeState::SliderAttachment m_dryAttachment;
    std::array<::juce::AudioProcessorValueTreeState::SliderAttachment, Processor::NumChannels> m_channelAttachments;

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plot)
};

class Editor : public ::juce::AudioProcessorEditor, public ::juce::ChangeListener, public ::juce::Slider::Listener
{
public:
    explicit Editor(Processor &);

    ~Editor() override;

    void paint(::juce::Graphics &) override;

    void resized() override;

    void changeListenerCallback(::juce::ChangeBroadcaster *) override;

    void sliderValueChanged(::juce::Slider *) override;

private:
    Processor &m_processor;
    chrono::StopWatch m_redrawStopWatch;

    PlotComponent m_plotComponent;
    TuningComponent m_tuningComponent;
    std::array<ChannelComponent, Processor::NumChannels> m_channelComponents;

    MixComponent m_mixComponent;

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

}    // namespace sw::juce::pitchtool

#pragma once
#include "sw/juce/pitchtool/processor.h"
#include <sw/chrono/stopwatch.hpp>
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

    void mouseUp(const ::juce::MouseEvent &) override;

private:
    void setSignalPlotEnabled(bool);
    void setSpectrumPlotEnabled(bool);

    Processor &m_processor;

    size_t m_signalPlotBlockSize{20u};
    size_t m_signalPlotLength{48000u};
    ui::plot::Plot m_signalPlot;
    ::juce::Label m_signalPlotEnableLabel{"", "Click to enable Signal Plotting"};
    ui::plot::Plot m_spectrumPlot;
    ::juce::Label m_spectrumPlotEnableLabel{"", "Click to enable Spectrum Plotting"};
    ::juce::Label m_logScaleLabel{"", "Log Scale: "};
    ::juce::ToggleButton m_frequenciesLogScaleButton{"Frequencies"};
    ::juce::ToggleButton m_gainsLogScaleButton{"Gains"};

    ::juce::AudioProcessorValueTreeState::ButtonAttachment m_frequenciesLogScaleAttachment;
    ::juce::AudioProcessorValueTreeState::ButtonAttachment m_gainsLogScaleAttachment;
};

class TuningComponent : public ui::GroupComponent
{
public:
    TuningComponent(Processor &);

    void resized() override;

    void setFrequency(float frequency, float standardPitch);

private:
    ui::RoundSlider m_standardPitchSlider{"Standard\nPitch"};
    ui::RoundSlider m_averagingTimeSlider{"Averaging\nTime"};
    ui::RoundSlider m_holdTimeSlider{"Hold\nTime"};
    ui::RoundSlider m_attackTimeSlider{"Attack\nTime"};
    ui::NoteDisplay m_noteDisplay{ui::NoteDisplay::Layout::Vertical};
    ::juce::ImageButton m_resetMidiButton;

    ::juce::AudioProcessorValueTreeState::SliderAttachment m_standardPitchAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_averagingTimeAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_holdTimeAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_attackTimeAttachment;
};

class ChannelComponent : public ui::GroupComponent
{
public:
    ChannelComponent(Processor &, size_t oneBasedChannel);

    void resized() override;

    void setFrequency(float frequency, float standardPitch);

private:
    ui::GroupComponent m_tuningComponent{""};
    ::juce::ComboBox m_tuningComboBox{"Tuning"};
    ui::RoundSlider m_pitchShiftSlider{"Pitch"};
    ui::RoundSlider m_formantsShiftSlider{"Formants"};
    ui::NoteDisplay m_noteDisplay{ui::NoteDisplay::Layout::Vertical};

    ::juce::AudioProcessorValueTreeState::ComboBoxAttachment m_tuningAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_pitchShiftAttachment;
    ::juce::AudioProcessorValueTreeState::SliderAttachment m_formantsShiftAttachment;
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
};

}    // namespace sw::juce::pitchtool

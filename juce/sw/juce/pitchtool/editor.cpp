#include "sw/juce/pitchtool/editor.h"
#include "sw/juce/pitchtool/processor.h"
#include "sw/juce/ui/utils.h"
#include "sw/notes.hpp"
#include <ranges>

namespace {

constexpr auto marginsSize{10.0f};
constexpr auto noteDisplayWidthFraction{1.0f / 13.0f};

std::array<::juce::Colour, sw::juce::pitchtool::Processor::NumChannels> signalGraphColors{
  ::juce::Colour::fromRGBA(193u, 193u, 193u, 200u), ::juce::Colour::fromRGBA(100u, 100u, 100u, 100u)};

constexpr auto NumSpectrumGraphs{sw::juce::pitchtool::Processor::NumChannels + 1U};

std::array<::juce::Colour, NumSpectrumGraphs> spectrumGraphColors{::juce::Colour::fromRGBA(42u, 131u, 200u, 200u),
                                                                  ::juce::Colour::fromRGBA(200u, 127u, 36u, 200u),
                                                                  ::juce::Colour::fromRGBA(193u, 193u, 193u, 200u)};

std::vector<sw::juce::ui::plot::Graph> makeSpectrumGraphs()
{
    std::vector<sw::juce::ui::plot::Graph> graphs;
    graphs.reserve(NumSpectrumGraphs);
    for (auto i = 0u; i < NumSpectrumGraphs; ++i)
        graphs.emplace_back(spectrumGraphColors[i], sw::juce::ui::plot::DrawType::LinesFromBottom, 2.0f);
    return graphs;
}

}    // namespace

sw::juce::pitchtool::PlotComponent::PlotComponent(sw::juce::pitchtool::Processor &processor)
    : ui::GroupComponent("", marginsSize, true)
    , m_processor(processor)
    , m_signalPlot(ui::plot::signal::xRange(m_signalPlotLength, m_signalPlotBlockSize), ui::plot::signal::yRange(),
                   {ui::plot::Graph(signalGraphColors[0], ui::plot::DrawType::LineConnected, 2.0f,
                                    ui::plot::signal::numBlocks(m_signalPlotLength, m_signalPlotBlockSize)),
                    ui::plot::Graph(signalGraphColors[1], ui::plot::DrawType::LineConnected, 2.0f,
                                    ui::plot::signal::numBlocks(m_signalPlotLength, m_signalPlotBlockSize))})
    , m_spectrumPlot(ui::plot::spectrum::xRange(processor.parameterValue<bool>("frequenciesLogScale")),
                     ui::plot::spectrum::yRange(processor.parameterValue<bool>("gainsLogScale")), makeSpectrumGraphs())
    , m_frequenciesLogScaleAttachment(processor.parameterState(), "frequenciesLogScale", m_frequenciesLogScaleButton)
    , m_gainsLogScaleAttachment(processor.parameterState(), "gainsLogScale", m_gainsLogScaleButton)
{
    addAndMakeVisible(m_signalPlot);
    addAndMakeVisible(m_spectrumPlot);
    addAndMakeVisible(m_logScaleLabel);
    addAndMakeVisible(m_frequenciesLogScaleButton);
    addAndMakeVisible(m_gainsLogScaleButton);
}

void sw::juce::pitchtool::PlotComponent::resized()
{
    ui::layoutHorizontal(getLocalBounds().toFloat().reduced(marginsSize), marginsSize, m_spectrumPlot, m_signalPlot);

    const auto plotButtonBounds =
      m_spectrumPlot.getBounds().toFloat().reduced(marginsSize).withHeight(2.0f * marginsSize);
    ui::layoutHorizontal(plotButtonBounds, marginsSize, m_logScaleLabel, m_gainsLogScaleButton,
                         m_frequenciesLogScaleButton);
}

void sw::juce::pitchtool::PlotComponent::updatePlots()
{
    m_signalPlot.graphs.front().pushYValues(
      ui::plot::signal::blockSignal(m_processor.inputBuffer(), m_signalPlotBlockSize));
    m_signalPlot.graphs.back().pushYValues(
      ui::plot::signal::blockSignal(m_processor.outputBuffer(), m_signalPlotBlockSize));

    m_signalPlot.repaint();

    const auto frequenciesLogScale = static_cast<bool>(m_processor.parameterValue<bool>("frequenciesLogScale"));
    const auto gainsLogScale = static_cast<bool>(m_processor.parameterValue<bool>("gainsLogScale"));

    const auto xRange = ui::plot::spectrum::xRange(frequenciesLogScale);
    const auto yRange = ui::plot::spectrum::yRange(gainsLogScale);
    m_spectrumPlot.setRanges(xRange, yRange);

    const auto plotSpectrum = [&](const std::vector<SpectrumValue<float>> &spectrum, ui::plot::Graph &o_graph) {
        o_graph.setValues(ui::plot::spectrum::xValues(frequencies<float>(spectrum), frequenciesLogScale),
                          ui::plot::spectrum::yValues(gains<float>(spectrum), gainsLogScale));
    };

    plotSpectrum(m_processor.pitchProcessor().inSpectrum(), m_spectrumPlot.graphs.back());
    for (auto channel = 0u; channel < Processor::NumChannels; ++channel)
        plotSpectrum(m_processor.pitchProcessor().outSpectrum(channel), m_spectrumPlot.graphs[channel]);

    m_spectrumPlot.repaint();
}

sw::juce::pitchtool::TuningComponent::TuningComponent(::juce::AudioProcessorValueTreeState &processorState)
    : ui::GroupComponent("Tuning", marginsSize, true)
    , m_standardPitchAttachment(processorState, "standardPitch", m_standardPitchSlider)
    , m_frequencyAveragingTimeAttachment(processorState, "frequencyAveragingTime", m_frequencyAveragingTimeSlider)
    , m_attackTimeAttachment(processorState, "attackTime", m_attackTimeSlider)
{
    addAndMakeVisible(m_standardPitchSlider);
    addAndMakeVisible(m_frequencyAveragingTimeSlider);
    addAndMakeVisible(m_attackTimeSlider);
    addAndMakeVisible(m_noteDisplay);

    m_standardPitchSlider.setTextValueSuffix(" Hz");
    m_frequencyAveragingTimeSlider.setTextValueSuffix(" sec");
    m_attackTimeSlider.setTextValueSuffix(" sec");
}

void sw::juce::pitchtool::TuningComponent::resized()
{
    ui::GroupComponent::resized();

    const auto localBounds = getLocalBounds().toFloat().reduced(marginsSize);
    const auto noteDisplayWidth = noteDisplayWidthFraction * localBounds.getWidth();
    const auto otherWidth = localBounds.getWidth() - noteDisplayWidth;

    m_noteDisplay.setBounds(localBounds.withTrimmedLeft(otherWidth).toNearestInt());

    const auto sliderBounds = localBounds.withWidth(otherWidth).reduced(marginsSize);
    ui::layoutHorizontal(sliderBounds, marginsSize, m_standardPitchSlider, m_frequencyAveragingTimeSlider,
                         m_attackTimeSlider);
}

void sw::juce::pitchtool::TuningComponent::setFrequency(const float frequency, const float standardPitch)
{
    m_noteDisplay.set(frequency, standardPitch);
}

sw::juce::pitchtool::ChannelComponent::ChannelComponent(Processor &processor, size_t channel)
    : ui::GroupComponent("Channel " + std::to_string(channel), marginsSize, true)
    , m_tuningAttachment(processor.parameterState(), "tuning_" + ::juce::String(channel), m_tuningComboBox)
    , m_pitchShiftAttachment(processor.parameterState(), "pitchShift_" + ::juce::String(channel), m_pitchShiftSlider)
    , m_formantsShiftAttachment(processor.parameterState(), "formantsShift_" + ::juce::String(channel), m_formantsShiftSlider)
{
    m_tuningComboBox.addItem(std::string(::sw::pitchtool::tuning::typeNames[tuning::NoTuning]), 1);
    m_tuningComboBox.addItem(std::string(::sw::pitchtool::tuning::typeNames[tuning::Midi]) + " Ch" +
                               std::to_string(tuning::processingToMidiChannel(channel)),
                             2);
    m_tuningComboBox.addItem(std::string(::sw::pitchtool::tuning::typeNames[tuning::AutoTune]), 3);

    // doesn't seem to be initially synced, so we do that by hand
    m_tuningComboBox.setSelectedId(processor.parameterValue<int>("tuning_" + std::to_string(channel)) + 1);

    addAndMakeVisible(m_tuningComponent);
    m_tuningComponent.addAndMakeVisible(m_tuningComboBox);
    addAndMakeVisible(m_pitchShiftSlider);
    addAndMakeVisible(m_formantsShiftSlider);
    addAndMakeVisible(m_noteDisplay);

    m_pitchShiftSlider.setTextValueSuffix(" st");
    m_formantsShiftSlider.setTextValueSuffix(" st");
}

void sw::juce::pitchtool::ChannelComponent::resized()
{
    ui::GroupComponent::resized();

    const auto localBounds = getLocalBounds().toFloat().reduced(marginsSize);
    const auto noteDisplayWidth = noteDisplayWidthFraction * localBounds.getWidth();
    const auto otherWidth = localBounds.getWidth() - noteDisplayWidth;

    m_noteDisplay.setBounds(localBounds.withTrimmedLeft(otherWidth).toNearestInt());

    const auto sliderBounds = localBounds.withWidth(otherWidth).reduced(marginsSize);
    ui::layoutHorizontal(sliderBounds, marginsSize, m_tuningComponent, m_pitchShiftSlider, m_formantsShiftSlider);

    m_tuningComboBox.setBounds(0, m_tuningComponent.getHeight() / 3, m_tuningComponent.getWidth(),
                               m_tuningComponent.getHeight() / 5);
}

void sw::juce::pitchtool::ChannelComponent::setFrequency(const float frequency, const float standardPitch)
{
    m_noteDisplay.set(frequency, standardPitch);
}

sw::juce::pitchtool::MixComponent::MixComponent(::juce::AudioProcessorValueTreeState &processorState)
    : ui::GroupComponent("Out Mix", marginsSize, true)
    , m_channelSliders(containers::makeArray<Processor::NumChannels>(
        [&](const size_t channel) { return ui::RoundSlider("Channel " + std::to_string(channel)); }))
    , m_dryAttachment(processorState, "dryMixGain", m_drySlider)
    , m_channelAttachments(containers::makeArray<Processor::NumChannels>([&](const size_t channel) {
        return ::juce::AudioProcessorValueTreeState::SliderAttachment(
          processorState, "mixGain_" + ::juce::String(channel), m_channelSliders[channel]);
    }))
{
    addAndMakeVisible(m_drySlider);
    for (auto &channelSlider : m_channelSliders)
        addAndMakeVisible(channelSlider);
}

void sw::juce::pitchtool::MixComponent::resized()
{
    ui::GroupComponent::resized();

    ::juce::Array<::juce::Component *> sliders;
    sliders.add(&m_drySlider);
    for (auto &channelSlider : m_channelSliders)
        sliders.add(&channelSlider);
    ui::layoutVertical(getLocalBounds().toFloat().reduced(marginsSize), marginsSize, sliders);
}

sw::juce::pitchtool::Editor::Editor(sw::juce::pitchtool::Processor &processor)
    : ::juce::AudioProcessorEditor(&processor)
    , m_processor(processor)
    , m_plotComponent(processor)
    , m_tuningComponent(processor.parameterState())
    , m_channelComponents(containers::makeArray<Processor::NumChannels>(
        [&](const size_t channel) { return ChannelComponent(processor, channel); }))
    , m_mixComponent(processor.parameterState())
{
    setSize(600, 800);
    setResizeLimits(300, 400, 900, 1200);

    addAndMakeVisible(m_plotComponent);
    addAndMakeVisible(m_tuningComponent);
    for (auto &channelComponent : m_channelComponents)
        addAndMakeVisible(channelComponent);
    addAndMakeVisible(m_mixComponent);

    m_processor.newDataBroadCaster().addChangeListener(this);

    auto &channelMixSliders = m_mixComponent.channelSliders();
    for (auto channel = 0u; channel < Processor::NumChannels; ++channel)
    {
        m_channelComponents[channel].setEnabled(channelMixSliders[channel].getValue() != 0.0);
        m_mixComponent.channelSliders()[channel].addListener(this);
    }
}

sw::juce::pitchtool::Editor::~Editor()
{
    m_processor.newDataBroadCaster().removeChangeListener(this);
}

void sw::juce::pitchtool::Editor::paint(::juce::Graphics &juceGraphics)
{
    juceGraphics.fillAll(getLookAndFeel().findColour(::juce::ResizableWindow::backgroundColourId));
}

void sw::juce::pitchtool::Editor::resized()
{
    const auto editorBounds = getLocalBounds().toFloat();

    const auto plotsHeight = 0.2f * editorBounds.getHeight();
    const auto otherHeight = (editorBounds.getHeight() - plotsHeight) / static_cast<float>(Processor::NumChannels + 1);
    const auto mixWidth = 0.25f * editorBounds.getWidth();
    const auto otherWidth = editorBounds.getWidth() - mixWidth;

    m_plotComponent.setBounds(editorBounds.withHeight(plotsHeight).reduced(marginsSize).toNearestInt());

    const auto templateBounds = editorBounds.withWidth(otherWidth).withHeight(otherHeight);

    m_tuningComponent.setBounds(templateBounds.withY(plotsHeight).reduced(marginsSize).toNearestInt());
    for (auto channel = 0u; channel < Processor::NumChannels; ++channel)
    {
        m_channelComponents[channel].setBounds(
          templateBounds.withY(plotsHeight + static_cast<float>(channel + 1) * otherHeight)
            .reduced(marginsSize)
            .toNearestInt());
    }

    m_mixComponent.setBounds(
      editorBounds.withTrimmedLeft(otherWidth).withTrimmedTop(plotsHeight).reduced(marginsSize).toNearestInt());
}

void sw::juce::pitchtool::Editor::changeListenerCallback(::juce::ChangeBroadcaster *sender)
{
    if (sender == &m_processor.newDataBroadCaster() && m_redrawStopWatch.elapsed() > 0.05)
    {
        m_redrawStopWatch.reset();
        m_plotComponent.updatePlots();

        const auto &pitchProcessor = m_processor.pitchProcessor();
        const auto standardPitch = m_processor.parameterValue<float>("standardPitch");

        m_tuningComponent.setFrequency(pitchProcessor.inFundamentalFrequency(), standardPitch);

        for (auto channel = 0u; channel < Processor::NumChannels; ++channel)
            m_channelComponents[channel].setFrequency(pitchProcessor.outFundamentalFrequency(channel), standardPitch);
    }
}

void sw::juce::pitchtool::Editor::sliderValueChanged(::juce::Slider *slider)
{
    const auto &channelSliders = m_mixComponent.channelSliders();
    for (auto channel = 0u; channel < Processor::NumChannels; ++channel)
    {
        if (&(channelSliders[channel]) == slider)
            m_channelComponents[channel].setEnabled(slider->getValue() != 0.0);
    }
}

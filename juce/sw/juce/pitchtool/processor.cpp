#include "sw/juce/pitchtool/processor.h"
#include "sw/juce/pitchtool/editor.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <ranges>
#include <span>

namespace {

std::unique_ptr<::juce::AudioProcessorParameterGroup> createMainParameterGroup()
{
    return std::make_unique<::juce::AudioProcessorParameterGroup>(
      "main", "Main", "|",
      std::make_unique<::juce::AudioParameterFloat>("dryMixGain", "Dry Mix",
                                                    ::juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                    ::sw::pitchtool::defaultDryMixGain<float>()),
      std::make_unique<::juce::AudioParameterBool>("frequenciesLogScale", "Frequencies Log Scale", true),
      std::make_unique<::juce::AudioParameterBool>("gainsLogScale", "Gains Log Scale", true));
}

float toSeconds(const float milliseconds)
{
    return std::round(milliseconds / 1000.0f);
}

float toMilliseconds(const float seconds)
{
    return seconds * 1000.0f;
}

std::unique_ptr<::juce::AudioProcessorParameterGroup> createTuningParameterGroup()
{
    using namespace sw::pitchtool;

    return std::make_unique<juce::AudioProcessorParameterGroup>(
      "tuning", "Tuning", "|",
      std::make_unique<::juce::AudioParameterFloat>(
        "standardPitch", "Standard Pitch",
        ::juce::NormalisableRange<float>(defaultTuningParameters<float>().standardPitchRange[0],
                                         defaultTuningParameters<float>().standardPitchRange[1], 1.0f),
        defaultTuningParameters<float>().standardPitch),
      std::make_unique<::juce::AudioParameterFloat>(
        "averagingTime", "Averaging Time",
        ::juce::NormalisableRange<float>(toMilliseconds(defaultTuningParameters<float>().averagingTimeRange[0]),
                                         toMilliseconds(defaultTuningParameters<float>().averagingTimeRange[1]), 1.0f),
        toMilliseconds(defaultTuningParameters<float>().averagingTime)),
      std::make_unique<::juce::AudioParameterFloat>(
        "holdTime", "Hold Time",
        ::juce::NormalisableRange<float>(toMilliseconds(defaultTuningParameters<float>().holdTimeRange[0]),
                                         toMilliseconds(defaultTuningParameters<float>().holdTimeRange[1]), 1.0f),
        toMilliseconds(defaultTuningParameters<float>().holdTime)),
      std::make_unique<::juce::AudioParameterFloat>(
        "attackTime", "Attack Time",
        ::juce::NormalisableRange<float>(toMilliseconds(defaultTuningParameters<float>().attackTimeRange[0]),
                                         toMilliseconds(defaultTuningParameters<float>().attackTimeRange[1]), 1.0f),
        toMilliseconds(defaultTuningParameters<float>().attackTime)));
}

std::unique_ptr<::juce::AudioProcessorParameterGroup> createChannelGroup(const size_t zeroBasedChannel)
{
    static constexpr auto defaultChannelParameters =
      sw::pitchtool::defaultChannelParameters<float, sw::juce::pitchtool::Processor::NumChannels>();

    const ::juce::String oneBasedChannelAsString(zeroBasedChannel + 1);
    const auto &defaultParameters = defaultChannelParameters[zeroBasedChannel];
    return std::make_unique<juce::AudioProcessorParameterGroup>(
      "channel_" + oneBasedChannelAsString, "Channel " + oneBasedChannelAsString, "|",
      std::make_unique<::juce::AudioParameterInt>(
        "tuning_" + oneBasedChannelAsString, "Tuning " + oneBasedChannelAsString, sw::juce::pitchtool::tuning::NoTuning,
        sw::juce::pitchtool::tuning::AutoTune, sw::juce::pitchtool::tuning::NoTuning),
      std::make_unique<::juce::AudioParameterFloat>(
        "pitchShift_" + oneBasedChannelAsString, "Pitch Shift " + oneBasedChannelAsString,
        ::juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), defaultParameters.pitchShift),
      std::make_unique<::juce::AudioParameterFloat>(
        "formantsShift_" + oneBasedChannelAsString, "Formants Filter Shift " + oneBasedChannelAsString,
        ::juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), defaultParameters.formantsShift),
      std::make_unique<::juce::AudioParameterFloat>(
        "mixGain_" + oneBasedChannelAsString, "Mix " + oneBasedChannelAsString,
        ::juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), defaultParameters.mixGain));
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(const std::uint8_t numChannels)
{
    std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> parameters;
    parameters.reserve(numChannels + 2u);
    parameters.push_back(createMainParameterGroup());
    parameters.push_back(createTuningParameterGroup());
    for (auto channel = 0u; channel < numChannels; ++channel)
        parameters.push_back(createChannelGroup(channel));
    return {parameters.begin(), parameters.end()};
}

void processMidiBuffer(
  const ::juce::MidiBuffer &midiBuffer,
  std::array<sw::pitchtool::tuning::MidiTune, sw::juce::pitchtool::Processor::NumChannels> &o_midiTunes)
{
    using namespace sw::juce::pitchtool;
    for (const auto &metaMessage : midiBuffer)
    {
        const auto message = metaMessage.getMessage();
        const auto midiChannel = static_cast<size_t>(message.getChannel());
        const auto processingChannel = midiChannel - 1u;

        if (processingChannel < o_midiTunes.size())
        {
            auto &midiTune = o_midiTunes[processingChannel];
            if (message.isNoteOn())
                midiTune.midiNoteNumber = message.getNoteNumber();
            else if (message.isNoteOff() && message.getNoteNumber() == midiTune.midiNoteNumber)
                midiTune.midiNoteNumber = -1;
        }

        if (message.isPitchWheel())
        {
            const auto pitchWheelValue = message.getPitchWheelValue();
            if (midiChannel == 0)
                std::ranges::for_each(o_midiTunes, [&](auto &midiTune) { midiTune.pitchBend = pitchWheelValue; });
            else if (processingChannel < o_midiTunes.size())
                o_midiTunes[processingChannel].pitchBend = pitchWheelValue;
        }
    }
}

}    // namespace

sw::juce::pitchtool::Processor::Processor()
    : ::juce::AudioProcessor(BusesProperties()
                               .withInput("Input", ::juce::AudioChannelSet::mono(), true)
                               .withOutput("Output", ::juce::AudioChannelSet::mono(), true))
    , m_processingBuffer(m_signalBufferSize, m_pitchProcessor.stepSize())
    , m_parameterState(*this, nullptr, "state", createParameterLayout(NumChannels))
{
    setLatencySamples(static_cast<int>(m_pitchProcessor.overlapSize()));
}

bool sw::juce::pitchtool::Processor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainInputChannelSet() == ::juce::AudioChannelSet::mono() &&
           layouts.getMainOutputChannelSet() == ::juce::AudioChannelSet::mono();
}

sw::pitchtool::TuningParameters<float> sw::juce::pitchtool::Processor::tuningParameters()
{
    return {parameterValue<float>("standardPitch"), toSeconds(parameterValue<float>("averagingTime")),
            toSeconds(parameterValue<float>("holdTime")), toSeconds(parameterValue<float>("attackTime"))};
}

sw::pitchtool::ChannelParameters<float> sw::juce::pitchtool::Processor::channelParameters(size_t zeroBasedChannel)
{
    const auto channelAsString = std::to_string(zeroBasedChannel + 1);

    const auto tuningType = [&]() -> sw::pitchtool::tuning::Type {
        const auto typeAsInt = parameterValue<int>("tuning_" + channelAsString);
        if (typeAsInt == tuning::Midi && m_currentMidiTunes[zeroBasedChannel].midiNoteNumber > 0)
            return m_currentMidiTunes[zeroBasedChannel];
        else if (typeAsInt == tuning::AutoTune)
            return sw::pitchtool::tuning::AutoTune{m_currentMidiTunes[zeroBasedChannel]};
        return {};
    };

    return {tuningType(), parameterValue<float>("pitchShift_" + channelAsString),
            parameterValue<float>("formantsShift_" + channelAsString),
            parameterValue<float>("mixGain_" + channelAsString)};
}

void sw::juce::pitchtool::Processor::processBlock(::juce::AudioBuffer<float> &audioBuffer,
                                                  ::juce::MidiBuffer &midiBuffer)
{
    processMidiBuffer(midiBuffer, m_currentMidiTunes);

    const auto processStep = [&](auto &&inStepSignal, auto &&outStepSignal) {
        m_pitchProcessor.process(inStepSignal, outStepSignal, static_cast<float>(getSampleRate()), tuningParameters(),
                                 allChannelParameters(), parameterValue<float>("dryMixGain"));
        m_newDataBroadCaster.sendChangeMessage();
    };

    const auto numSamples = audioBuffer.getNumSamples();
    m_processingBuffer.process(std::span(audioBuffer.getReadPointer(0), numSamples),
                               std::span(audioBuffer.getWritePointer(0), numSamples), processStep);
}

void sw::juce::pitchtool::Processor::processBlockBypassed(::juce::AudioBuffer<float> &audioBuffer,
                                                          ::juce::MidiBuffer &)
{
    resetMidi();

    const auto processStep = [&](auto &&inStepSignal, auto &&outStepSignal) {
        m_pitchProcessor.processByPassed(inStepSignal, outStepSignal);
        m_newDataBroadCaster.sendChangeMessage();
    };

    const auto numSamples = audioBuffer.getNumSamples();
    m_processingBuffer.process(std::span(audioBuffer.getReadPointer(0), numSamples),
                               std::span(audioBuffer.getWritePointer(0), numSamples), processStep);
}

::juce::AudioProcessorEditor *sw::juce::pitchtool::Processor::createEditor()
{
    return new sw::juce::pitchtool::Editor(*this);
}

void sw::juce::pitchtool::Processor::getStateInformation(::juce::MemoryBlock &destData)
{
    if (const auto xmlState = m_parameterState.copyState().createXml())
        copyXmlToBinary(*xmlState, destData);
}

void sw::juce::pitchtool::Processor::setStateInformation(const void *data, int sizeInBytes)
{
    if (const auto xmlState = getXmlFromBinary(data, sizeInBytes))
        m_parameterState.replaceState(::juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new sw::juce::pitchtool::Processor();
}

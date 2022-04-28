#include "sw/juce/pitchtool/processor.h"
#include "sw/juce/pitchtool/editor.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <ranges>
#include <span>

namespace {

using PitchProcessor = sw::juce::pitchtool::Processor::PitchProcessor;

std::unique_ptr<::juce::AudioProcessorParameterGroup> createMainParameterGroup()
{
    return std::make_unique<::juce::AudioProcessorParameterGroup>(
      "main", "Main", "|",
      std::make_unique<::juce::AudioParameterFloat>("dryMixGain", "Dry Mix",
                                                    ::juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                    PitchProcessor::defaultDryMixGain),
      std::make_unique<::juce::AudioParameterBool>("frequenciesLogScale", "Frequencies Log Scale", true),
      std::make_unique<::juce::AudioParameterBool>("gainsLogScale", "Gains Log Scale", true));
}

std::unique_ptr<::juce::AudioProcessorParameterGroup> createTuningParameterGroup()
{
    return std::make_unique<juce::AudioProcessorParameterGroup>(
      "tuning", "Tuning", "|",
      std::make_unique<::juce::AudioParameterFloat>("standardPitch", "Standard Pitch",
                                                    ::juce::NormalisableRange<float>(400.0f, 480.0f, 1.0f),
                                                    PitchProcessor::defaultTuningParameters.standardPitch),
      std::make_unique<::juce::AudioParameterFloat>("frequencyAveragingTime", "Frequency Averaging Time",
                                                    ::juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
                                                    PitchProcessor::defaultTuningParameters.frequencyAveragingTime),
      std::make_unique<::juce::AudioParameterFloat>("attackTime", "Attack Time",
                                                    ::juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
                                                    PitchProcessor::defaultTuningParameters.attackTime));
}

std::unique_ptr<::juce::AudioProcessorParameterGroup> createChannelGroup(const size_t channel)
{
    const ::juce::String channelAsString(channel);
    const auto &defaultParameters = PitchProcessor::defaultChannelParameters[channel];
    return std::make_unique<juce::AudioProcessorParameterGroup>(
      "channel_" + channelAsString, "Channel " + channelAsString, "|",
      std::make_unique<::juce::AudioParameterInt>(
        "tuning_" + channelAsString, "Tuning " + channelAsString, sw::juce::pitchtool::tuning::NoTuning,
        sw::juce::pitchtool::tuning::AutoTune, sw::juce::pitchtool::tuning::NoTuning),
      std::make_unique<::juce::AudioParameterFloat>("pitchShift_" + channelAsString, "Pitch Shift " + channelAsString,
                                                    ::juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f),
                                                    defaultParameters.pitchShift),
      std::make_unique<::juce::AudioParameterFloat>(
        "formantsShift_" + channelAsString, "Formants Filter Shift " + channelAsString,
        ::juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), defaultParameters.formantsShift),
      std::make_unique<::juce::AudioParameterFloat>("mixGain_" + channelAsString, "Mix " + channelAsString,
                                                    ::juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                    defaultParameters.mixGain));
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

void processMidiNotes(const ::juce::MidiBuffer &midiBuffer, const bool byPassed,
                      std::array<std::optional<sw::Note>, sw::juce::pitchtool::Processor::NumChannels> &o_midiNotes)
{
    using namespace sw::juce::pitchtool;
    if (byPassed)
    {
        for (auto &note : o_midiNotes)
            note = std::nullopt;
    }
    else
    {
        for (const auto &metaMessage : midiBuffer)
        {
            const auto message = metaMessage.getMessage();
            const auto channel = tuning::midiToProcessingChannel(message.getChannel());
            if (channel < Processor::NumChannels)
            {
                if (message.isNoteOff())
                    o_midiNotes[channel] = std::nullopt;
                else if (message.isNoteOn())
                    o_midiNotes[channel] = sw::fromMidi(message.getNoteNumber());
            }
        }
    }
}

}    // namespace

sw::juce::pitchtool::Processor::Processor()
    : ::juce::AudioProcessor(BusesProperties()
                               .withInput("Input", ::juce::AudioChannelSet::mono(), true)
                               .withOutput("Output", ::juce::AudioChannelSet::mono(), true))
    , m_processedSignalBuffer(m_pitchProcessor.stepSize())
    , m_state(*this, nullptr, "state", createParameterLayout(NumChannels))
{
    m_numOutSamples = m_pitchProcessor.stepSize();
    setLatencySamples(m_pitchProcessor.overlapSize());
}

bool sw::juce::pitchtool::Processor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainInputChannelSet() == ::juce::AudioChannelSet::mono() &&
           layouts.getMainOutputChannelSet() == ::juce::AudioChannelSet::mono();
}

sw::juce::pitchtool::Processor::PitchProcessor::TuningParameters sw::juce::pitchtool::Processor::tuneParameters()
{
    return {parameterValue<float>("standardPitch"), parameterValue<float>("frequencyAveragingTime"),
            parameterValue<float>("attackTime")};
}

sw::juce::pitchtool::Processor::PitchProcessor::ChannelParameters
  sw::juce::pitchtool::Processor::channelParameters(size_t channel)
{
    const auto channelAsString = std::to_string(channel);

    const auto tuningType = [&]() {
        const auto typeAsInt = parameterValue<int>("tuning_" + channelAsString);
        sw::tuning::Type type;
        if (typeAsInt == tuning::Midi && m_currentMidiNotes[channel] != std::nullopt)
            type = *m_currentMidiNotes[channel];
        else if (typeAsInt == tuning::AutoTune)
            type = sw::tuning::AutoTune{};
        return type;
    };

    return {tuningType(), parameterValue<float>("pitchShift_" + channelAsString),
            parameterValue<float>("formantsShift_" + channelAsString),
            parameterValue<float>("mixGain_" + channelAsString)};
}

void sw::juce::pitchtool::Processor::processBlock(::juce::AudioBuffer<float> &audioBuffer,
                                                  ::juce::MidiBuffer &midiBuffer, const bool byPassed)
{
    processMidiNotes(midiBuffer, byPassed, m_currentMidiNotes);

    const auto numSamples = static_cast<size_t>(audioBuffer.getNumSamples());
    const auto stepSize = m_pitchProcessor.stepSize();

    m_inputBuffer.ringPush(std::span(audioBuffer.getReadPointer(0), numSamples));

    for (m_numNewProcessingSamples += numSamples; m_numNewProcessingSamples >= stepSize;
         m_numNewProcessingSamples -= stepSize)
    {
        const auto signal =
          std::span(m_inputBuffer.inBuffer().end() - static_cast<int>(m_numNewProcessingSamples), stepSize);

        if (byPassed)
            m_pitchProcessor.processByPassed(signal, m_processedSignalBuffer);
        else
        {
            m_pitchProcessor.process(signal, m_processedSignalBuffer, static_cast<float>(getSampleRate()),
                                     tuneParameters(), allChannelParameters(), parameterValue<float>("dryMixGain"));
        }

        m_newDataBroadCaster.sendChangeMessage();

        m_outputBuffer.ringPush(m_processedSignalBuffer);
        m_numOutSamples += stepSize;
    }

    assert(m_numOutSamples <= m_outputBuffer.inBuffer().size());
    const auto outStart = m_outputBuffer.inBuffer().end() - static_cast<int>(m_numOutSamples);
    std::copy(outStart, outStart + audioBuffer.getNumSamples(),
              std::span(audioBuffer.getWritePointer(0), static_cast<size_t>(audioBuffer.getNumSamples())).begin());

    assert(m_numOutSamples >= numSamples);
    m_numOutSamples -= numSamples;
}

void sw::juce::pitchtool::Processor::processBlock(::juce::AudioBuffer<float> &audioBuffer,
                                                  ::juce::MidiBuffer &midiBuffer)
{
    processBlock(audioBuffer, midiBuffer, false);
}

void sw::juce::pitchtool::Processor::processBlockBypassed(::juce::AudioBuffer<float> &audioBuffer,
                                                          ::juce::MidiBuffer &midiBuffer)
{
    processBlock(audioBuffer, midiBuffer, true);
}

::juce::AudioProcessorEditor *sw::juce::pitchtool::Processor::createEditor()
{
    return new sw::juce::pitchtool::Editor(*this);
}

void sw::juce::pitchtool::Processor::getStateInformation(::juce::MemoryBlock &destData)
{
    if (const auto xmlState = m_state.copyState().createXml())
        copyXmlToBinary(*xmlState, destData);
}

void sw::juce::pitchtool::Processor::setStateInformation(const void *data, int sizeInBytes)
{
    if (const auto xmlState = getXmlFromBinary(data, sizeInBytes))
        m_state.replaceState(::juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new sw::juce::pitchtool::Processor();
}

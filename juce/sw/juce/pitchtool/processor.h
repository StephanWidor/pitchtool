#pragma once
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <sw/containers/spinlockedbuffer.hpp>
#include <sw/pitchtool/processor.hpp>

namespace sw::juce::pitchtool {

namespace tuning {

enum Type : int
{
    NoTuning = 0,
    Midi = 1,
    AutoTune = 2
};

constexpr int processingToMidiChannel(const size_t channel)
{
    return static_cast<int>(channel) + 1;
}

constexpr size_t midiToProcessingChannel(const int channel)
{
    return static_cast<size_t>(channel) - 1u;
}

}    // namespace tuning

class Processor : public ::juce::AudioProcessor
{
public:
    static constexpr std::uint8_t NumChannels{2u};

    Processor();
    ~Processor() override {}

    void prepareToPlay(double, int) override {}

    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(::juce::AudioBuffer<float> &, ::juce::MidiBuffer &) override;

    void processBlockBypassed(::juce::AudioBuffer<float> &, ::juce::MidiBuffer &) override;

    ::juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const ::juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override
    {
        return 1;    // NB: some hosts don't cope very well if you tell them there are 0 programs,
          // so this should be at least 1, even if you're not really implementing programs.
    }

    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const ::juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const ::juce::String &) override{};

    void getStateInformation(::juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    ::juce::ChangeBroadcaster &newDataBroadCaster() { return m_newDataBroadCaster; }

    size_t signalBufferSize() const { return m_signalBufferSize; }

    const std::vector<float> &inputBuffer() const { return m_inputBuffer.outBuffer(); }
    const std::vector<float> &outputBuffer() const { return m_outputBuffer.outBuffer(); }

    const ::sw::pitchtool::Processor<float, NumChannels> &pitchProcessor() const { return m_pitchProcessor; }

    ::juce::AudioProcessorValueTreeState &state() { return m_state; }

    template<typename T>
    T parameterValue(const std::string &parameterName)
    {
        const auto parameter = m_state.getParameter(parameterName);
        return static_cast<T>(parameter->convertFrom0to1(parameter->getValue()));
    }

    ::sw::pitchtool::TuningParameters<float> tuningParameters();

    ::sw::pitchtool::ChannelParameters<float> channelParameters(size_t channel);

    std::array<::sw::pitchtool::ChannelParameters<float>, NumChannels> allChannelParameters()
    {
        return containers::makeArray<NumChannels>([&](const auto channel) { return channelParameters(channel); });
    }

private:
    void processBlock(::juce::AudioBuffer<float> &, ::juce::MidiBuffer &, bool byPassed);

    const size_t m_signalBufferSize{48000u};
    containers::SpinLockedBuffer<float> m_inputBuffer{m_signalBufferSize, 0.0f};
    containers::SpinLockedBuffer<float> m_outputBuffer{m_signalBufferSize, 0.0f};

    std::array<std::optional<Note>, NumChannels> m_currentMidiNotes;

    ::juce::ChangeBroadcaster m_newDataBroadCaster;

    size_t m_numNewProcessingSamples{0u};
    size_t m_numOutSamples{0u};

    ::sw::pitchtool::Processor<float, NumChannels> m_pitchProcessor{2048u, 4u};
    std::vector<float> m_processedSignalBuffer;

    ::juce::AudioProcessorValueTreeState m_state;

    //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

}    // namespace sw::juce::pitchtool

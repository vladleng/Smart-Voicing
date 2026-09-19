#include "PluginProcessor.h"
#include "PluginEditor.h"

SmartVoicingAudioProcessor::SmartVoicingAudioProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SmartVoicingAudioProcessor::prepareToPlay(double, int)
{
}

void SmartVoicingAudioProcessor::releaseResources()
{
}

bool SmartVoicingAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void SmartVoicingAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Этап 0: никакого DSP и никакой MIDI-трансформации.
    // Аудио и MIDI проходят через плагин без изменений.
    juce::ignoreUnused(buffer, midiMessages);
}

juce::AudioProcessorEditor* SmartVoicingAudioProcessor::createEditor()
{
    return new SmartVoicingAudioProcessorEditor(*this);
}

void SmartVoicingAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    static constexpr char state[] = "SmartVoicingStateV1";
    destData.replaceAll(state, sizeof(state));
}

void SmartVoicingAudioProcessor::setStateInformation(const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartVoicingAudioProcessor();
}

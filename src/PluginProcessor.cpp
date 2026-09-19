#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JucePlugin_Enable_ARA
#include "ARAContextDocumentController.h"
#endif

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

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto seconds = position->getTimeInSeconds())
                lastPositionSeconds.store(*seconds, std::memory_order_relaxed);

            if (const auto ppq = position->getPpqPosition())
                lastPpqPosition.store(*ppq, std::memory_order_relaxed);
        }
    }

    // Smart Voicing 0.0b is an ARA context proof of concept only.
    // Audio and MIDI still pass through unchanged.
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

#if JucePlugin_Enable_ARA
void SmartVoicingAudioProcessor::didBindToARA() noexcept
{
    juce::AudioProcessorARAExtension::didBindToARA();
    araBound.store(true, std::memory_order_relaxed);
}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartVoicingAudioProcessor();
}

#if JucePlugin_Enable_ARA
const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return juce::ARADocumentControllerSpecialisation::createARAFactory<SmartVoicingARADocumentController>();
}
#endif

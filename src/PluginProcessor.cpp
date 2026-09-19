#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JucePlugin_Enable_ARA
#include "ARAContextDocumentController.h"
#endif

SmartVoicingAudioProcessor::SmartVoicingAudioProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              // For the Instrument experiment the audio input is optional and disabled by default.
              // This keeps the normal instrument use-case clean while still leaving an input bus
              // available for hosts that may require it for ARA/Event FX binding.
              .withInput("Input", juce::AudioChannelSet::stereo(), false)
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

    const auto outputSupported = output == juce::AudioChannelSet::mono()
                              || output == juce::AudioChannelSet::stereo();

    if (! outputSupported)
        return false;

    // Instrument slot: no main audio input. Event/FX use: matching mono/stereo input is accepted.
    return input.isDisabled() || input == output;
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

    // Smart Voicing 0.0b is still a routing/ARA proof of concept.
    // MIDI is intentionally left unchanged. No sound generation is implemented yet.
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

#pragma once

#include <JuceHeader.h>
#include <atomic>

class SmartVoicingAudioProcessor final : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
                                      , public juce::AudioProcessorARAExtension
#endif
{
public:
    SmartVoicingAudioProcessor();
    ~SmartVoicingAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isARABoundForDebug() const noexcept { return araBound.load(std::memory_order_relaxed); }
    double getLastPositionSecondsForDebug() const noexcept { return lastPositionSeconds.load(std::memory_order_relaxed); }
    double getLastPpqPositionForDebug() const noexcept { return lastPpqPosition.load(std::memory_order_relaxed); }

protected:
#if JucePlugin_Enable_ARA
    void didBindToARA() noexcept override;
#endif

private:
    std::atomic<bool> araBound { false };
    std::atomic<double> lastPositionSeconds { -1.0 };
    std::atomic<double> lastPpqPosition { -1.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingAudioProcessor)
};

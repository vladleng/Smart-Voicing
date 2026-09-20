#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cstdint>

class SmartVoicingInstrumentProcessor final : public juce::AudioProcessor
{
public:
    enum class MidiProbeEventType : int
    {
        none = 0,
        noteOn,
        noteOff,
        controller,
        pitchBend,
        otherChannel,
        systemOrOther
    };

    struct MidiProbeSnapshot
    {
        std::uint32_t revision = 0;
        std::uint32_t totalEvents = 0;
        std::uint32_t noteOnEvents = 0;
        std::uint32_t noteOffEvents = 0;
        std::uint32_t controllerEvents = 0;
        std::uint32_t pitchBendEvents = 0;
        std::uint32_t otherEvents = 0;
        std::uint32_t channelMask = 0;
        MidiProbeEventType lastEventType = MidiProbeEventType::none;
        int lastChannel = 0;
        int lastData1 = 0;
        int lastData2 = 0;
    };

    SmartVoicingInstrumentProcessor();
    ~SmartVoicingInstrumentProcessor() override = default;

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

    double getLastPositionSecondsForDebug() const noexcept { return lastPositionSeconds.load(std::memory_order_relaxed); }
    double getLastPpqPositionForDebug() const noexcept { return lastPpqPosition.load(std::memory_order_relaxed); }

    MidiProbeSnapshot getMidiProbeSnapshot() const noexcept;
    void resetMidiProbeStatistics() noexcept;

private:
    void recordMidiEventForProbe(const juce::MidiMessageMetadata&) noexcept;

    std::atomic<double> lastPositionSeconds { -1.0 };
    std::atomic<double> lastPpqPosition { -1.0 };

    std::atomic<std::uint32_t> midiRevision { 0 };
    std::atomic<std::uint32_t> midiTotalEvents { 0 };
    std::atomic<std::uint32_t> midiNoteOnEvents { 0 };
    std::atomic<std::uint32_t> midiNoteOffEvents { 0 };
    std::atomic<std::uint32_t> midiControllerEvents { 0 };
    std::atomic<std::uint32_t> midiPitchBendEvents { 0 };
    std::atomic<std::uint32_t> midiOtherEvents { 0 };
    std::atomic<std::uint32_t> midiChannelMask { 0 };
    std::atomic<int> lastMidiEventType { static_cast<int>(MidiProbeEventType::none) };
    std::atomic<int> lastMidiChannel { 0 };
    std::atomic<int> lastMidiData1 { 0 };
    std::atomic<int> lastMidiData2 { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingInstrumentProcessor)
};

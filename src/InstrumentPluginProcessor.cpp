#include "InstrumentPluginProcessor.h"
#include "InstrumentPluginEditor.h"

namespace
{
constexpr std::uint8_t statusMask = 0xF0;
constexpr std::uint8_t channelMask = 0x0F;
constexpr std::uint8_t noteOffStatus = 0x80;
constexpr std::uint8_t noteOnStatus = 0x90;
constexpr std::uint8_t controllerStatus = 0xB0;
constexpr std::uint8_t pitchBendStatus = 0xE0;
constexpr std::uint8_t systemStatus = 0xF0;
}

SmartVoicingInstrumentProcessor::SmartVoicingInstrumentProcessor()
    : juce::AudioProcessor(BusesProperties()
                              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SmartVoicingInstrumentProcessor::prepareToPlay(double, int)
{
    resetMidiProbeStatistics();
}

void SmartVoicingInstrumentProcessor::releaseResources()
{
}

bool SmartVoicingInstrumentProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void SmartVoicingInstrumentProcessor::processBlock(juce::AudioBuffer<float>& buffer,
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

    // 0.1a is deliberately a transparent MIDI pass-through probe. The range-for
    // iterator exposes a non-owning view of the raw MIDI bytes, so diagnostics do
    // not allocate or copy MidiMessage objects on the real-time thread.
    for (const auto metadata : midiMessages)
        recordMidiEventForProbe(metadata);

    // This Instrument produces no audio. MIDI is intentionally left untouched so
    // Studio Pro can be tested as the routing host before voice allocation begins.
    buffer.clear();
}

void SmartVoicingInstrumentProcessor::recordMidiEventForProbe(const juce::MidiMessageMetadata& metadata) noexcept
{
    midiTotalEvents.fetch_add(1, std::memory_order_relaxed);

    if (metadata.data == nullptr || metadata.numBytes <= 0)
    {
        midiOtherEvents.fetch_add(1, std::memory_order_relaxed);
        lastMidiEventType.store(static_cast<int>(MidiProbeEventType::systemOrOther), std::memory_order_relaxed);
        lastMidiChannel.store(0, std::memory_order_relaxed);
        lastMidiData1.store(0, std::memory_order_relaxed);
        lastMidiData2.store(0, std::memory_order_relaxed);
        midiRevision.fetch_add(1, std::memory_order_release);
        return;
    }

    const auto status = metadata.data[0];
    const auto messageType = static_cast<std::uint8_t>(status & statusMask);
    const auto data1 = metadata.numBytes > 1 ? static_cast<int>(metadata.data[1]) : 0;
    const auto data2 = metadata.numBytes > 2 ? static_cast<int>(metadata.data[2]) : 0;

    auto eventType = MidiProbeEventType::systemOrOther;
    auto channel = 0;

    if (status < systemStatus)
    {
        channel = static_cast<int>(status & channelMask) + 1;
        midiChannelMask.fetch_or(1u << static_cast<unsigned int>(channel - 1), std::memory_order_relaxed);

        if (messageType == noteOnStatus && data2 > 0)
        {
            eventType = MidiProbeEventType::noteOn;
            midiNoteOnEvents.fetch_add(1, std::memory_order_relaxed);
        }
        else if (messageType == noteOffStatus || (messageType == noteOnStatus && data2 == 0))
        {
            eventType = MidiProbeEventType::noteOff;
            midiNoteOffEvents.fetch_add(1, std::memory_order_relaxed);
        }
        else if (messageType == controllerStatus)
        {
            eventType = MidiProbeEventType::controller;
            midiControllerEvents.fetch_add(1, std::memory_order_relaxed);
        }
        else if (messageType == pitchBendStatus)
        {
            eventType = MidiProbeEventType::pitchBend;
            midiPitchBendEvents.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            eventType = MidiProbeEventType::otherChannel;
            midiOtherEvents.fetch_add(1, std::memory_order_relaxed);
        }
    }
    else
    {
        midiOtherEvents.fetch_add(1, std::memory_order_relaxed);
    }

    lastMidiEventType.store(static_cast<int>(eventType), std::memory_order_relaxed);
    lastMidiChannel.store(channel, std::memory_order_relaxed);
    lastMidiData1.store(data1, std::memory_order_relaxed);
    lastMidiData2.store(data2, std::memory_order_relaxed);
    midiRevision.fetch_add(1, std::memory_order_release);
}

SmartVoicingInstrumentProcessor::MidiProbeSnapshot SmartVoicingInstrumentProcessor::getMidiProbeSnapshot() const noexcept
{
    MidiProbeSnapshot snapshot;
    snapshot.revision = midiRevision.load(std::memory_order_acquire);
    snapshot.totalEvents = midiTotalEvents.load(std::memory_order_relaxed);
    snapshot.noteOnEvents = midiNoteOnEvents.load(std::memory_order_relaxed);
    snapshot.noteOffEvents = midiNoteOffEvents.load(std::memory_order_relaxed);
    snapshot.controllerEvents = midiControllerEvents.load(std::memory_order_relaxed);
    snapshot.pitchBendEvents = midiPitchBendEvents.load(std::memory_order_relaxed);
    snapshot.otherEvents = midiOtherEvents.load(std::memory_order_relaxed);
    snapshot.channelMask = midiChannelMask.load(std::memory_order_relaxed);
    snapshot.lastEventType = static_cast<MidiProbeEventType>(lastMidiEventType.load(std::memory_order_relaxed));
    snapshot.lastChannel = lastMidiChannel.load(std::memory_order_relaxed);
    snapshot.lastData1 = lastMidiData1.load(std::memory_order_relaxed);
    snapshot.lastData2 = lastMidiData2.load(std::memory_order_relaxed);
    return snapshot;
}

void SmartVoicingInstrumentProcessor::resetMidiProbeStatistics() noexcept
{
    midiRevision.store(0, std::memory_order_relaxed);
    midiTotalEvents.store(0, std::memory_order_relaxed);
    midiNoteOnEvents.store(0, std::memory_order_relaxed);
    midiNoteOffEvents.store(0, std::memory_order_relaxed);
    midiControllerEvents.store(0, std::memory_order_relaxed);
    midiPitchBendEvents.store(0, std::memory_order_relaxed);
    midiOtherEvents.store(0, std::memory_order_relaxed);
    midiChannelMask.store(0, std::memory_order_relaxed);
    lastMidiEventType.store(static_cast<int>(MidiProbeEventType::none), std::memory_order_relaxed);
    lastMidiChannel.store(0, std::memory_order_relaxed);
    lastMidiData1.store(0, std::memory_order_relaxed);
    lastMidiData2.store(0, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SmartVoicingInstrumentProcessor::createEditor()
{
    return new SmartVoicingInstrumentEditor(*this);
}

void SmartVoicingInstrumentProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    static constexpr char state[] = "SmartVoicingInstrumentStateV1";
    destData.replaceAll(state, sizeof(state));
}

void SmartVoicingInstrumentProcessor::setStateInformation(const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartVoicingInstrumentProcessor();
}

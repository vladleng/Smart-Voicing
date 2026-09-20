#include "InstrumentPluginProcessor.h"
#include "InstrumentPluginEditor.h"

#include <algorithm>

namespace
{
constexpr std::uint8_t statusMask = 0xF0;
constexpr std::uint8_t channelMask = 0x0F;
constexpr std::uint8_t noteOffStatus = 0x80;
constexpr std::uint8_t noteOnStatus = 0x90;
constexpr std::uint8_t controllerStatus = 0xB0;
constexpr std::uint8_t pitchBendStatus = 0xE0;
constexpr std::uint8_t systemStatus = 0xF0;
constexpr int sustainController = 64;
constexpr int voiceCount = 4;
constexpr int firstOutputChannel = 1;
constexpr int midiOutputReserveBytes = 32768;

bool isNoteMessage(const juce::MidiMessageMetadata& metadata) noexcept
{
    if (metadata.data == nullptr || metadata.numBytes <= 0)
        return false;

    const auto status = metadata.data[0];
    if (status >= systemStatus)
        return false;

    const auto type = static_cast<std::uint8_t>(status & statusMask);
    return type == noteOnStatus || type == noteOffStatus;
}

bool isNoteOffMessage(const juce::MidiMessageMetadata& metadata) noexcept
{
    if (! isNoteMessage(metadata) || metadata.numBytes < 2)
        return false;

    const auto type = static_cast<std::uint8_t>(metadata.data[0] & statusMask);
    const auto velocity = metadata.numBytes > 2 ? static_cast<int>(metadata.data[2]) : 0;
    return type == noteOffStatus || (type == noteOnStatus && velocity == 0);
}

bool getSustainState(const juce::MidiMessageMetadata& metadata, bool& down) noexcept
{
    if (metadata.data == nullptr || metadata.numBytes < 3)
        return false;

    const auto status = metadata.data[0];
    if (status >= systemStatus || (status & statusMask) != controllerStatus)
        return false;

    if (static_cast<int>(metadata.data[1]) != sustainController)
        return false;

    down = static_cast<int>(metadata.data[2]) >= 64;
    return true;
}
}

SmartVoicingInstrumentProcessor::SmartVoicingInstrumentProcessor()
    : juce::AudioProcessor(BusesProperties()
                              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    noteVoiceOwners.fill(-1);

    for (auto& voiceNote : voiceNotesForUi)
        voiceNote.store(-1, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::prepareToPlay(double, int)
{
    routedMidi.clear();
    routedMidi.ensureSize(static_cast<std::size_t>(midiOutputReserveBytes));
    resetRouterState();
    resetMidiProbeStatistics();
}

void SmartVoicingInstrumentProcessor::releaseResources()
{
    routedMidi.clear();
    resetRouterState();
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

    routedMidi.clear();

    int currentSamplePosition = -1;
    bool noteStateChangedAtCurrentSample = false;

    const auto flushPendingNoteState = [this, &currentSamplePosition, &noteStateChangedAtCurrentSample]
    {
        if (noteStateChangedAtCurrentSample && currentSamplePosition >= 0)
            applyVoiceState(currentSamplePosition);

        noteStateChangedAtCurrentSample = false;
    };

    for (const auto metadata : midiMessages)
    {
        if (currentSamplePosition >= 0 && metadata.samplePosition != currentSamplePosition)
            flushPendingNoteState();

        currentSamplePosition = metadata.samplePosition;
        recordMidiInputEventForProbe(metadata);

        if (isNoteMessage(metadata))
        {
            // Once a chord/interval starts being edited, preserve the identity of
            // the existing Voice slots instead of re-ranking the remaining notes.
            if (isNoteOffMessage(metadata) && ! stableOwnership && getActiveVoiceCount() >= 2)
            {
                stableOwnership = true;
                stableOwnershipForUi.store(true, std::memory_order_relaxed);
            }

            noteStateChangedAtCurrentSample = updateHeldNoteFromEvent(metadata)
                                            || noteStateChangedAtCurrentSample;
            continue;
        }

        // Preserve MIDI event order: finish any pending note ownership change before
        // broadcasting a controller/pitch/pressure event at the same sample.
        flushPendingNoteState();

        bool newSustainState = false;
        if (getSustainState(metadata, newSustainState))
        {
            if (newSustainState != sustainDown)
            {
                sustainDown = newSustainState;
                sustainDownForUi.store(sustainDown, std::memory_order_relaxed);

                if (sustainDown)
                {
                    // Sustain requires stable ownership even for fewer than four voices:
                    // released keys keep their Voice slot until pedal-up.
                    stableOwnership = true;
                    stableOwnershipForUi.store(true, std::memory_order_relaxed);
                }
                else
                {
                    // Emit deferred Note Offs and free sustain-held ownership before
                    // CC64-up reaches the destination instrument. This makes Sustain
                    // work even when the destination plug-in itself ignores CC64.
                    applyVoiceState(metadata.samplePosition);
                }
            }

            routeNonNoteEvent(metadata);
            continue;
        }

        routeNonNoteEvent(metadata);

        if (shouldClearHeldNotes(metadata))
            clearHeldNotes();
    }

    flushPendingNoteState();

    midiMessages.swapWith(routedMidi);

    // The Instrument is a MIDI engine and does not generate audio.
    buffer.clear();
}

bool SmartVoicingInstrumentProcessor::updateHeldNoteFromEvent(const juce::MidiMessageMetadata& metadata) noexcept
{
    if (metadata.data == nullptr || metadata.numBytes < 2)
        return false;

    const auto status = metadata.data[0];
    if (status >= systemStatus)
        return false;

    const auto type = static_cast<std::uint8_t>(status & statusMask);
    const auto note = static_cast<int>(metadata.data[1]);
    const auto velocity = metadata.numBytes > 2 ? static_cast<int>(metadata.data[2]) : 0;

    if (note < 0 || note >= static_cast<int>(heldNoteCounts.size()))
        return false;

    const auto isNoteOn = type == noteOnStatus && velocity > 0;
    const auto isNoteOff = type == noteOffStatus || (type == noteOnStatus && velocity == 0);

    if (isNoteOn)
    {
        auto& count = heldNoteCounts[static_cast<std::size_t>(note)];
        if (count == 0)
            ++heldDistinctNoteCount;

        if (count < 255)
            ++count;

        heldNoteVelocities[static_cast<std::size_t>(note)] = static_cast<std::uint8_t>(velocity);
        heldNoteCountForUi.store(heldDistinctNoteCount, std::memory_order_relaxed);
        return true;
    }

    if (isNoteOff)
    {
        auto& count = heldNoteCounts[static_cast<std::size_t>(note)];
        if (count == 0)
            return false;

        --count;
        if (count == 0)
            heldDistinctNoteCount = (std::max)(0, heldDistinctNoteCount - 1);

        heldNoteCountForUi.store(heldDistinctNoteCount, std::memory_order_relaxed);
        return true;
    }

    return false;
}

void SmartVoicingInstrumentProcessor::applyVoiceState(int samplePosition)
{
    if (sustainDown)
        stableOwnership = true;

    if (stableOwnership)
        reconcileStableAssignments(samplePosition);
    else
        rebuildRankedAssignments(samplePosition);

    // A complete four-note voicing becomes a stable ownership frame. From this
    // point a released/replaced note edits its own Voice instead of shifting all
    // neighbouring voices.
    if (! stableOwnership && getActiveVoiceCount() >= voiceCount)
        stableOwnership = true;

    if (! sustainDown && heldDistinctNoteCount == 0 && getActiveVoiceCount() == 0)
        stableOwnership = false;

    sustainDownForUi.store(sustainDown, std::memory_order_relaxed);
    stableOwnershipForUi.store(stableOwnership, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::rebuildRankedAssignments(int samplePosition)
{
    std::array<int, voiceCount> desiredNotes { -1, -1, -1, -1 };
    int desiredIndex = 0;

    for (int note = 127; note >= 0 && desiredIndex < voiceCount; --note)
    {
        if (heldNoteCounts[static_cast<std::size_t>(note)] == 0)
            continue;

        desiredNotes[static_cast<std::size_t>(desiredIndex)] = note;
        ++desiredIndex;
    }

    // Release every ranked Voice that is about to change.
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        if (activeVoiceNotes[index] >= 0 && activeVoiceNotes[index] != desiredNotes[index])
            sendVoiceNoteOff(voice, samplePosition);
    }

    noteVoiceOwners.fill(-1);

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto oldNote = activeVoiceNotes[index];
        const auto newNote = desiredNotes[index];

        if (oldNote != newNote)
        {
            activeVoiceNotes[index] = newNote;
            voiceNoteOnActive[index] = false;
            voiceReleasedUnderSustain[index] = false;
        }

        if (newNote >= 0)
        {
            noteVoiceOwners[static_cast<std::size_t>(newNote)] = voice;
            if (! voiceNoteOnActive[index])
                sendVoiceNoteOn(voice, samplePosition);
        }

        voiceNotesForUi[index].store(newNote, std::memory_order_relaxed);
    }
}

void SmartVoicingInstrumentProcessor::reconcileStableAssignments(int samplePosition)
{
    // Once stable, each Voice keeps its channel identity. Sustain is implemented
    // inside the router: physical key release is remembered, but downstream Note Off
    // is deferred until pedal-up so this behaviour does not depend on the synth.
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto note = activeVoiceNotes[index];
        if (note < 0)
            continue;

        const auto physicallyHeld = heldNoteCounts[static_cast<std::size_t>(note)] > 0;

        if (physicallyHeld)
        {
            if (voiceReleasedUnderSustain[index])
            {
                // Re-articulate a sustain-held note on the same Voice/channel.
                sendVoiceNoteOff(voice, samplePosition);
                sendVoiceNoteOn(voice, samplePosition);
                voiceReleasedUnderSustain[index] = false;
            }
            else if (! voiceNoteOnActive[index])
            {
                sendVoiceNoteOn(voice, samplePosition);
            }
            continue;
        }

        if (sustainDown)
        {
            voiceReleasedUnderSustain[index] = true;
            continue;
        }

        if (voiceNoteOnActive[index])
            sendVoiceNoteOff(voice, samplePosition);

        voiceReleasedUnderSustain[index] = false;
        clearVoiceOwnership(voice);
    }

    assignUnownedHeldNotes(samplePosition);
}

void SmartVoicingInstrumentProcessor::assignUnownedHeldNotes(int samplePosition)
{
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto voiceIndex = static_cast<std::size_t>(voice);
        if (activeVoiceNotes[voiceIndex] >= 0)
            continue;

        int candidate = -1;
        for (int note = 127; note >= 0; --note)
        {
            const auto noteIndex = static_cast<std::size_t>(note);
            if (heldNoteCounts[noteIndex] == 0 || noteVoiceOwners[noteIndex] >= 0)
                continue;

            candidate = note;
            break;
        }

        if (candidate < 0)
            continue;

        activeVoiceNotes[voiceIndex] = candidate;
        noteVoiceOwners[static_cast<std::size_t>(candidate)] = voice;
        voiceNoteOnActive[voiceIndex] = false;
        voiceReleasedUnderSustain[voiceIndex] = false;
        voiceNotesForUi[voiceIndex].store(candidate, std::memory_order_relaxed);
        sendVoiceNoteOn(voice, samplePosition);
    }
}

void SmartVoicingInstrumentProcessor::sendVoiceNoteOn(int voice, int samplePosition)
{
    if (voice < 0 || voice >= voiceCount)
        return;

    const auto index = static_cast<std::size_t>(voice);
    const auto note = activeVoiceNotes[index];
    if (note < 0)
        return;

    const auto velocity = heldNoteVelocities[static_cast<std::size_t>(note)];
    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOnStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        velocity > 0 ? velocity : static_cast<std::uint8_t>(1)
    };

    addOutputEvent(bytes, 3, samplePosition);
    voiceNoteOnActive[index] = true;
    voiceReleasedUnderSustain[index] = false;
}

void SmartVoicingInstrumentProcessor::sendVoiceNoteOff(int voice, int samplePosition)
{
    if (voice < 0 || voice >= voiceCount)
        return;

    const auto index = static_cast<std::size_t>(voice);
    const auto note = activeVoiceNotes[index];
    if (note < 0 || ! voiceNoteOnActive[index])
        return;

    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOffStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        0
    };

    addOutputEvent(bytes, 3, samplePosition);
    voiceNoteOnActive[index] = false;
}

void SmartVoicingInstrumentProcessor::clearVoiceOwnership(int voice) noexcept
{
    if (voice < 0 || voice >= voiceCount)
        return;

    const auto index = static_cast<std::size_t>(voice);
    const auto note = activeVoiceNotes[index];

    if (note >= 0 && note < static_cast<int>(noteVoiceOwners.size()))
    {
        const auto noteIndex = static_cast<std::size_t>(note);
        if (noteVoiceOwners[noteIndex] == voice)
            noteVoiceOwners[noteIndex] = -1;

        if (heldNoteCounts[noteIndex] == 0)
            heldNoteVelocities[noteIndex] = 0;
    }

    activeVoiceNotes[index] = -1;
    voiceNoteOnActive[index] = false;
    voiceReleasedUnderSustain[index] = false;
    voiceNotesForUi[index].store(-1, std::memory_order_relaxed);
}

int SmartVoicingInstrumentProcessor::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (const auto note : activeVoiceNotes)
    {
        if (note >= 0)
            ++count;
    }
    return count;
}

void SmartVoicingInstrumentProcessor::routeNonNoteEvent(const juce::MidiMessageMetadata& metadata)
{
    if (metadata.data == nullptr || metadata.numBytes <= 0)
        return;

    const auto status = metadata.data[0];

    // Channel messages are copied to all four Voice channels so expression,
    // sustain, pitch bend, aftertouch etc. reach every destination instrument.
    if (status < systemStatus)
    {
        if (metadata.numBytes > 3)
            return;

        std::uint8_t bytes[3] { 0, 0, 0 };
        for (int i = 0; i < metadata.numBytes; ++i)
            bytes[i] = metadata.data[i];

        for (int voice = 0; voice < voiceCount; ++voice)
        {
            bytes[0] = static_cast<std::uint8_t>((status & statusMask)
                                                | ((firstOutputChannel + voice - 1) & channelMask));
            addOutputEvent(bytes, metadata.numBytes, metadata.samplePosition);
        }
        return;
    }

    // System/common/SysEx messages do not have a MIDI channel and are preserved once.
    routedMidi.addEvent(metadata.data, metadata.numBytes, metadata.samplePosition);
    midiTotalOutputEvents.fetch_add(1, std::memory_order_relaxed);
}

bool SmartVoicingInstrumentProcessor::shouldClearHeldNotes(const juce::MidiMessageMetadata& metadata) const noexcept
{
    if (metadata.data == nullptr || metadata.numBytes < 2)
        return false;

    const auto status = metadata.data[0];
    if (status >= systemStatus || (status & statusMask) != controllerStatus)
        return false;

    const auto controller = static_cast<int>(metadata.data[1]);
    return controller == 120 || controller == 123; // All Sound Off / All Notes Off
}

void SmartVoicingInstrumentProcessor::clearHeldNotes() noexcept
{
    heldNoteCounts.fill(0);
    heldNoteVelocities.fill(0);
    noteVoiceOwners.fill(-1);
    activeVoiceNotes.fill(-1);
    voiceNoteOnActive.fill(false);
    voiceReleasedUnderSustain.fill(false);
    heldDistinctNoteCount = 0;
    sustainDown = false;
    stableOwnership = false;

    heldNoteCountForUi.store(0, std::memory_order_relaxed);
    sustainDownForUi.store(false, std::memory_order_relaxed);
    stableOwnershipForUi.store(false, std::memory_order_relaxed);

    for (auto& voiceNote : voiceNotesForUi)
        voiceNote.store(-1, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::addOutputEvent(const std::uint8_t* data,
                                                      int numBytes,
                                                      int samplePosition)
{
    routedMidi.addEvent(data, numBytes, samplePosition);
    midiTotalOutputEvents.fetch_add(1, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::recordMidiInputEventForProbe(const juce::MidiMessageMetadata& metadata) noexcept
{
    midiTotalInputEvents.fetch_add(1, std::memory_order_relaxed);

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
    snapshot.totalInputEvents = midiTotalInputEvents.load(std::memory_order_relaxed);
    snapshot.totalOutputEvents = midiTotalOutputEvents.load(std::memory_order_relaxed);
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
    snapshot.heldNoteCount = heldNoteCountForUi.load(std::memory_order_relaxed);
    snapshot.sustainDown = sustainDownForUi.load(std::memory_order_relaxed);
    snapshot.stableOwnership = stableOwnershipForUi.load(std::memory_order_relaxed);

    for (std::size_t i = 0; i < snapshot.voiceNotes.size(); ++i)
        snapshot.voiceNotes[i] = voiceNotesForUi[i].load(std::memory_order_relaxed);

    return snapshot;
}

void SmartVoicingInstrumentProcessor::resetMidiProbeStatistics() noexcept
{
    midiRevision.store(0, std::memory_order_relaxed);
    midiTotalInputEvents.store(0, std::memory_order_relaxed);
    midiTotalOutputEvents.store(0, std::memory_order_relaxed);
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

void SmartVoicingInstrumentProcessor::resetRouterState() noexcept
{
    clearHeldNotes();
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

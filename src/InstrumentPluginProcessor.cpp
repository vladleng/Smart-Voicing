#include "InstrumentPluginProcessor.h"
#include "InstrumentPluginEditor.h"

#include <algorithm>
#include <cstdlib>

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

    for (auto& stack : voiceNoteStacks)
        stack.fill(-1);

    for (auto& voiceNote : voiceNotesForUi)
        voiceNote.store(-1, std::memory_order_relaxed);

    for (auto& depth : voiceStackDepthsForUi)
        depth.store(0, std::memory_order_relaxed);
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
            // Editing an already established interval/chord switches to stable Voice
            // identity. A complete four-voice frame is locked automatically below.
            if (isNoteOffMessage(metadata) && ! stableOwnership && getActiveVoiceCount() >= 2)
            {
                stableOwnership = true;
                stableOwnershipForUi.store(true, std::memory_order_relaxed);
            }

            noteStateChangedAtCurrentSample = updateHeldNoteFromEvent(metadata)
                                            || noteStateChangedAtCurrentSample;
            continue;
        }

        // Preserve event order: apply note/stack changes before controller messages
        // arriving at a later or equal sample position.
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
                    stableOwnership = true;
                    stableOwnershipForUi.store(true, std::memory_order_relaxed);
                }
                else
                {
                    // Remove only stack entries whose physical keys are already up.
                    // Notes still held by the player remain active after pedal-up.
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

    if (note < 0 || note >= midiNoteCount)
        return false;

    const auto isNoteOn = type == noteOnStatus && velocity > 0;
    const auto isNoteOff = type == noteOffStatus || (type == noteOnStatus && velocity == 0);
    const auto noteIndex = static_cast<std::size_t>(note);

    if (isNoteOn)
    {
        auto& count = heldNoteCounts[noteIndex];
        const auto wasPhysicallyUp = count == 0;

        if (wasPhysicallyUp)
            ++heldDistinctNoteCount;

        if (count < 255)
            ++count;

        heldNoteVelocities[noteIndex] = static_cast<std::uint8_t>(velocity);

        // Re-pressing a note that is still present in a Voice stack because of
        // sustain should re-articulate that same Voice/channel, not allocate a new one.
        if (wasPhysicallyUp && noteVoiceOwners[noteIndex] >= 0)
            retriggerPending[noteIndex] = true;

        heldNoteCountForUi.store(heldDistinctNoteCount, std::memory_order_relaxed);
        return true;
    }

    if (isNoteOff)
    {
        auto& count = heldNoteCounts[noteIndex];
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

    // Four voices establish the persistent Voice 1-4 identity used by the live
    // legato/portamento workflow.
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

    for (int note = midiNoteCount - 1; note >= 0 && desiredIndex < voiceCount; --note)
    {
        if (heldNoteCounts[static_cast<std::size_t>(note)] == 0)
            continue;

        desiredNotes[static_cast<std::size_t>(desiredIndex)] = note;
        ++desiredIndex;
    }

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto desired = desiredNotes[static_cast<std::size_t>(voice)];
        const auto current = getVoiceTopNote(voice);
        const auto depth = voiceStackSizes[static_cast<std::size_t>(voice)];

        if (depth == (desired >= 0 ? 1 : 0) && current == desired)
            continue;

        clearVoiceStack(voice, samplePosition);
        if (desired >= 0)
            pushNoteToVoice(voice, desired, samplePosition);
    }
}

void SmartVoicingInstrumentProcessor::reconcileStableAssignments(int samplePosition)
{
    processPendingRetriggers(samplePosition);

    if (! sustainDown)
        removeReleasedNotesFromStacks(samplePosition);

    assignUnownedHeldNotes(samplePosition);
}

void SmartVoicingInstrumentProcessor::processPendingRetriggers(int samplePosition)
{
    for (int note = 0; note < midiNoteCount; ++note)
    {
        const auto noteIndex = static_cast<std::size_t>(note);
        if (! retriggerPending[noteIndex])
            continue;

        retriggerPending[noteIndex] = false;

        const auto voice = noteVoiceOwners[noteIndex];
        if (voice < 0 || heldNoteCounts[noteIndex] == 0)
            continue;

        moveOwnedNoteToTop(voice, note);

        if (routedNoteActive[noteIndex])
            sendRoutedNoteOff(voice, note, samplePosition);

        sendRoutedNoteOn(voice, note, samplePosition);
        refreshVoiceUi(voice);
    }
}

void SmartVoicingInstrumentProcessor::removeReleasedNotesFromStacks(int samplePosition)
{
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        auto& stackSize = voiceStackSizes[static_cast<std::size_t>(voice)];

        // Work backwards because removeNoteFromVoice compacts the fixed array.
        for (int index = stackSize - 1; index >= 0; --index)
        {
            const auto note = voiceNoteStacks[static_cast<std::size_t>(voice)][static_cast<std::size_t>(index)];
            if (note < 0)
                continue;

            if (heldNoteCounts[static_cast<std::size_t>(note)] == 0)
                removeNoteFromVoice(voice, note, samplePosition);
        }
    }
}

void SmartVoicingInstrumentProcessor::assignUnownedHeldNotes(int samplePosition)
{
    std::array<int, midiNoteCount> unownedNotes {};
    unownedNotes.fill(-1);
    int unownedCount = 0;

    for (int note = midiNoteCount - 1; note >= 0; --note)
    {
        const auto noteIndex = static_cast<std::size_t>(note);
        if (heldNoteCounts[noteIndex] == 0 || noteVoiceOwners[noteIndex] >= 0)
            continue;

        unownedNotes[static_cast<std::size_t>(unownedCount++)] = note;
    }

    if (unownedCount == 0)
        return;

    std::array<bool, midiNoteCount> noteAssigned {};
    std::array<bool, voiceCount> voiceUsed {};

    // First use genuinely empty Voice slots. This preserves the 1-4 voice bootstrap
    // while avoiding any unnecessary legato interpretation.
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        if (voiceStackSizes[static_cast<std::size_t>(voice)] != 0)
            continue;

        int notePosition = -1;
        for (int i = 0; i < unownedCount; ++i)
        {
            if (! noteAssigned[static_cast<std::size_t>(i)])
            {
                notePosition = i;
                break;
            }
        }

        if (notePosition < 0)
            break;

        const auto note = unownedNotes[static_cast<std::size_t>(notePosition)];
        if (pushNoteToVoice(voice, note, samplePosition))
        {
            noteAssigned[static_cast<std::size_t>(notePosition)] = true;
            voiceUsed[static_cast<std::size_t>(voice)] = true;
        }
    }

    std::array<int, midiNoteCount> remainingNotes {};
    remainingNotes.fill(-1);
    int remainingCount = 0;

    for (int i = 0; i < unownedCount; ++i)
    {
        if (! noteAssigned[static_cast<std::size_t>(i)])
            remainingNotes[static_cast<std::size_t>(remainingCount++)] = unownedNotes[static_cast<std::size_t>(i)];
    }

    if (remainingCount > 0)
        assignContinuationNotes(remainingNotes, remainingCount, voiceUsed, samplePosition);
}

void SmartVoicingInstrumentProcessor::assignContinuationNotes(const std::array<int, midiNoteCount>& notes,
                                                               int noteCount,
                                                               std::array<bool, voiceCount>& voiceUsed,
                                                               int samplePosition)
{
    std::array<bool, midiNoteCount> assigned {};

    const auto assignPhase = [this, &notes, noteCount, &voiceUsed, &assigned, samplePosition]
                             (const std::array<bool, voiceCount>& allowed)
    {
        int allowedCount = 0;
        int remainingNoteCount = 0;

        for (int voice = 0; voice < voiceCount; ++voice)
        {
            if (allowed[static_cast<std::size_t>(voice)] && ! voiceUsed[static_cast<std::size_t>(voice)])
                ++allowedCount;
        }

        for (int i = 0; i < noteCount; ++i)
        {
            if (! assigned[static_cast<std::size_t>(i)])
                ++remainingNoteCount;
        }

        if (allowedCount == 0 || remainingNoteCount == 0)
            return;

        // A simultaneous chord-sized continuation keeps vertical Voice order:
        // highest new note -> lowest Voice number. This is especially important for
        // changing chords while Sustain is down.
        if (allowedCount > 1 && remainingNoteCount >= allowedCount)
        {
            int notePosition = 0;
            for (int voice = 0; voice < voiceCount; ++voice)
            {
                if (! allowed[static_cast<std::size_t>(voice)] || voiceUsed[static_cast<std::size_t>(voice)])
                    continue;

                while (notePosition < noteCount && assigned[static_cast<std::size_t>(notePosition)])
                    ++notePosition;

                if (notePosition >= noteCount)
                    break;

                const auto note = notes[static_cast<std::size_t>(notePosition)];
                if (pushNoteToVoice(voice, note, samplePosition))
                {
                    assigned[static_cast<std::size_t>(notePosition)] = true;
                    voiceUsed[static_cast<std::size_t>(voice)] = true;
                }
                ++notePosition;
            }
            return;
        }

        // A single/few-note overlap is interpreted as continuation of the nearest
        // existing Voice. Sending Note On on that same channel before the old Note Off
        // gives monophonic physical-model instruments their normal legato/portamento cue.
        for (int i = 0; i < noteCount; ++i)
        {
            if (assigned[static_cast<std::size_t>(i)])
                continue;

            const auto note = notes[static_cast<std::size_t>(i)];
            const auto voice = chooseNearestVoice(note, allowed, voiceUsed);
            if (voice < 0)
                continue;

            if (pushNoteToVoice(voice, note, samplePosition))
            {
                assigned[static_cast<std::size_t>(i)] = true;
                voiceUsed[static_cast<std::size_t>(voice)] = true;
            }
        }
    };

    // Sustain-only Voices are the natural destination for the next chord. Their old
    // notes remain underneath in the Voice stack until pedal-up.
    std::array<bool, voiceCount> sustainOnlyVoices {};
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        sustainOnlyVoices[index] = voiceStackSizes[index] > 0 && ! voiceHasPhysicallyHeldNotes(voice);
    }
    assignPhase(sustainOnlyVoices);

    // Remaining new notes overlap physically held notes. Treat them as legato Voice
    // continuations. At most one new note is assigned to each Voice per sample frame.
    std::array<bool, voiceCount> allOccupiedVoices {};
    for (int voice = 0; voice < voiceCount; ++voice)
        allOccupiedVoices[static_cast<std::size_t>(voice)] = voiceStackSizes[static_cast<std::size_t>(voice)] > 0;

    assignPhase(allOccupiedVoices);
}

bool SmartVoicingInstrumentProcessor::pushNoteToVoice(int voice,
                                                       int note,
                                                       int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return false;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    const auto noteIndex = static_cast<std::size_t>(note);

    if (noteVoiceOwners[noteIndex] >= 0)
        return noteVoiceOwners[noteIndex] == voice;

    auto& stackSize = voiceStackSizes[voiceIndex];
    if (stackSize >= maxVoiceStackDepth)
        return false;

    voiceNoteStacks[voiceIndex][static_cast<std::size_t>(stackSize++)] = note;
    noteVoiceOwners[noteIndex] = voice;
    retriggerPending[noteIndex] = false;

    sendRoutedNoteOn(voice, note, samplePosition);
    refreshVoiceUi(voice);
    return true;
}

void SmartVoicingInstrumentProcessor::removeNoteFromVoice(int voice,
                                                           int note,
                                                           int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    auto& stackSize = voiceStackSizes[voiceIndex];
    int foundIndex = -1;

    for (int i = 0; i < stackSize; ++i)
    {
        if (voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i)] == note)
        {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex < 0)
        return;

    sendRoutedNoteOff(voice, note, samplePosition);

    for (int i = foundIndex; i + 1 < stackSize; ++i)
        voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i)] =
            voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i + 1)];

    --stackSize;
    voiceNoteStacks[voiceIndex][static_cast<std::size_t>(stackSize)] = -1;

    const auto noteIndex = static_cast<std::size_t>(note);
    if (noteVoiceOwners[noteIndex] == voice)
        noteVoiceOwners[noteIndex] = -1;

    retriggerPending[noteIndex] = false;
    if (heldNoteCounts[noteIndex] == 0)
        heldNoteVelocities[noteIndex] = 0;

    refreshVoiceUi(voice);
}

void SmartVoicingInstrumentProcessor::clearVoiceStack(int voice, int samplePosition)
{
    if (voice < 0 || voice >= voiceCount)
        return;

    while (voiceStackSizes[static_cast<std::size_t>(voice)] > 0)
    {
        const auto note = getVoiceTopNote(voice);
        if (note < 0)
            break;
        removeNoteFromVoice(voice, note, samplePosition);
    }
}

void SmartVoicingInstrumentProcessor::moveOwnedNoteToTop(int voice, int note) noexcept
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    const auto stackSize = voiceStackSizes[voiceIndex];
    int foundIndex = -1;

    for (int i = 0; i < stackSize; ++i)
    {
        if (voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i)] == note)
        {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex < 0 || foundIndex == stackSize - 1)
        return;

    for (int i = foundIndex; i + 1 < stackSize; ++i)
        voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i)] =
            voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i + 1)];

    voiceNoteStacks[voiceIndex][static_cast<std::size_t>(stackSize - 1)] = note;
    refreshVoiceUi(voice);
}

bool SmartVoicingInstrumentProcessor::voiceHasPhysicallyHeldNotes(int voice) const noexcept
{
    if (voice < 0 || voice >= voiceCount)
        return false;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    const auto stackSize = voiceStackSizes[voiceIndex];

    for (int i = 0; i < stackSize; ++i)
    {
        const auto note = voiceNoteStacks[voiceIndex][static_cast<std::size_t>(i)];
        if (note >= 0 && heldNoteCounts[static_cast<std::size_t>(note)] > 0)
            return true;
    }

    return false;
}

int SmartVoicingInstrumentProcessor::chooseNearestVoice(int note,
                                                         const std::array<bool, voiceCount>& allowed,
                                                         const std::array<bool, voiceCount>& alreadyUsed) const noexcept
{
    int bestVoice = -1;
    int bestDistance = 1000;

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto voiceIndex = static_cast<std::size_t>(voice);
        if (! allowed[voiceIndex] || alreadyUsed[voiceIndex])
            continue;

        const auto top = getVoiceTopNote(voice);
        if (top < 0)
            continue;

        const auto distance = std::abs(note - top);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestVoice = voice;
        }
    }

    return bestVoice;
}

int SmartVoicingInstrumentProcessor::getVoiceTopNote(int voice) const noexcept
{
    if (voice < 0 || voice >= voiceCount)
        return -1;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    const auto stackSize = voiceStackSizes[voiceIndex];
    if (stackSize <= 0)
        return -1;

    return voiceNoteStacks[voiceIndex][static_cast<std::size_t>(stackSize - 1)];
}

void SmartVoicingInstrumentProcessor::refreshVoiceUi(int voice) noexcept
{
    if (voice < 0 || voice >= voiceCount)
        return;

    const auto index = static_cast<std::size_t>(voice);
    voiceNotesForUi[index].store(getVoiceTopNote(voice), std::memory_order_relaxed);
    voiceStackDepthsForUi[index].store(voiceStackSizes[index], std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::sendRoutedNoteOn(int voice,
                                                        int note,
                                                        int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return;

    const auto noteIndex = static_cast<std::size_t>(note);
    const auto velocity = heldNoteVelocities[noteIndex];
    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOnStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        velocity > 0 ? velocity : static_cast<std::uint8_t>(1)
    };

    addOutputEvent(bytes, 3, samplePosition);
    routedNoteActive[noteIndex] = true;
}

void SmartVoicingInstrumentProcessor::sendRoutedNoteOff(int voice,
                                                         int note,
                                                         int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return;

    const auto noteIndex = static_cast<std::size_t>(note);
    if (! routedNoteActive[noteIndex])
        return;

    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOffStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        0
    };

    addOutputEvent(bytes, 3, samplePosition);
    routedNoteActive[noteIndex] = false;
}

int SmartVoicingInstrumentProcessor::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (const auto stackSize : voiceStackSizes)
    {
        if (stackSize > 0)
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
    routedNoteActive.fill(false);
    retriggerPending.fill(false);

    for (auto& stack : voiceNoteStacks)
        stack.fill(-1);
    voiceStackSizes.fill(0);

    heldDistinctNoteCount = 0;
    sustainDown = false;
    stableOwnership = false;

    heldNoteCountForUi.store(0, std::memory_order_relaxed);
    sustainDownForUi.store(false, std::memory_order_relaxed);
    stableOwnershipForUi.store(false, std::memory_order_relaxed);

    for (int voice = 0; voice < voiceCount; ++voice)
        refreshVoiceUi(voice);
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
    {
        snapshot.voiceNotes[i] = voiceNotesForUi[i].load(std::memory_order_relaxed);
        snapshot.voiceStackDepths[i] = voiceStackDepthsForUi[i].load(std::memory_order_relaxed);
    }

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

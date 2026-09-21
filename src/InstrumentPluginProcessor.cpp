#include "InstrumentPluginProcessor.h"
#include "InstrumentPluginEditor.h"
#include "ChordModel.h"
#include "CloseVoicingHarmonizer.h"
#include "LiveReharmonizer.h"

#include <algorithm>
#include <array>
#include <cmath>
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
constexpr double chordGestureWindowSeconds = 0.045;
constexpr int maxChordBoundariesPerBlock = 16;
constexpr int stateMagic = 0x53564D32; // "SVM2"
constexpr int stateVersion = 4;

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

std::uint8_t voiceBit(int voice) noexcept
{
    return static_cast<std::uint8_t>(1u << static_cast<unsigned int>(voice));
}

smartvoicing::harmony::ClosedVoicingContext buildClosedVoicingContext(
    smartvoicing::harmony::IHarmonicContextProvider& provider,
    const smartvoicing::harmony::HarmonicContext& currentContext,
    const smartvoicing::harmony::NormalizedChord& chord,
    double requestedPpq) noexcept
{
    smartvoicing::harmony::ClosedVoicingContext result;
    result.key = smartvoicing::harmony::normalizeKey(currentContext.key);

    auto analysisPpq = requestedPpq;
    if (analysisPpq < 0.0 && currentContext.positionAvailable)
        analysisPpq = currentContext.ppq;

    if (analysisPpq >= 0.0)
    {
        const auto nextPpq = provider.nextChordStartAfter(analysisPpq);
        if (nextPpq >= 0.0)
        {
            const auto nextContext = provider.contextAt(nextPpq);
            const auto nextChord = smartvoicing::harmony::normalizeChord(nextContext.chord);
            if (nextChord.valid)
            {
                result.harmonic = smartvoicing::harmony::analyzeHarmonicFunction(
                    chord, result.key, nextChord);
                return result;
            }
        }
    }

    result.harmonic = smartvoicing::harmony::analyzeHarmonicFunction(chord, result.key);
    return result;
}
}

SmartVoicingInstrumentProcessor::SmartVoicingInstrumentProcessor()
    : juce::AudioProcessor(BusesProperties()
                              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    for (auto& stack : voiceNoteStacks)
        stack.fill(-1);

    for (auto& voiceNote : voiceNotesForUi)
        voiceNote.store(-1, std::memory_order_relaxed);

    for (auto& depth : voiceStackDepthsForUi)
        depth.store(0, std::memory_order_relaxed);

    activeMelodyVoicing.clear();
}

void SmartVoicingInstrumentProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    chordGestureWindowSamples = (std::max<std::int64_t>)(1,
        static_cast<std::int64_t>(std::llround(currentSampleRate * chordGestureWindowSeconds)));
    processedSampleCounter = 0;
    currentBlockStartSeconds = -1.0;
    currentBlockStartPpq = -1.0;
    currentBlockBpm = -1.0;
    currentBlockNumSamples = 0;

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

    currentBlockStartSeconds = -1.0;
    currentBlockStartPpq = -1.0;
    currentBlockBpm = -1.0;
    currentBlockNumSamples = buffer.getNumSamples();

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            if (const auto seconds = position->getTimeInSeconds())
            {
                currentBlockStartSeconds = *seconds;
                lastPositionSeconds.store(*seconds, std::memory_order_relaxed);
            }

            if (const auto ppq = position->getPpqPosition())
            {
                currentBlockStartPpq = *ppq;
                lastPpqPosition.store(*ppq, std::memory_order_relaxed);
            }

            if (const auto bpm = position->getBpm())
                currentBlockBpm = *bpm;
        }
    }

    routedMidi.clear();
    const auto blockStartSample = processedSampleCounter;

    const auto requestedHarmonyValue = juce::jlimit(0, 1, requestedHarmonyMode.load(std::memory_order_relaxed));
    const auto requestedHarmony = static_cast<HarmonyMode>(requestedHarmonyValue);
    if (requestedHarmony != activeHarmonyMode)
    {
        for (int voice = 0; voice < voiceCount; ++voice)
            clearVoiceStack(voice, 0);

        clearHeldNotes();
        activeHarmonyMode = requestedHarmony;
    }

    const auto requestedTensionValue = juce::jlimit(
        static_cast<int>(smartvoicing::harmony::TensionLevel::clean),
        static_cast<int>(smartvoicing::harmony::TensionLevel::rich),
        requestedTensionLevel.load(std::memory_order_relaxed));
    activeTensionLevel = static_cast<smartvoicing::harmony::TensionLevel>(requestedTensionValue);

    if (activeHarmonyMode == HarmonyMode::melodyHarmonize)
    {
        processMelodyHarmonizeMidi(midiMessages, buffer.getNumSamples());
        midiMessages.swapWith(routedMidi);
        processedSampleCounter += static_cast<std::int64_t>(buffer.getNumSamples());
        buffer.clear();
        return;
    }

    const auto requestedModeValue = juce::jlimit(0, 2, requestedDistributionMode.load(std::memory_order_relaxed));
    const auto requestedMode = static_cast<DistributionMode>(requestedModeValue);
    if (requestedMode != activeDistributionMode)
    {
        for (int voice = 0; voice < voiceCount; ++voice)
            clearVoiceStack(voice, 0);

        chordFrameNotes.fill(-1);
        ignoredChordNotes.fill(false);
        activeDistributionMode = requestedMode;
        stableOwnership = false;
        pendingChordFrame = heldDistinctNoteCount > 0;
        chordGestureStartSample = pendingChordFrame ? blockStartSample : -1;
    }

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
            const auto absoluteSample = blockStartSample + static_cast<std::int64_t>(metadata.samplePosition);
            noteStateChangedAtCurrentSample = updateHeldNoteFromEvent(metadata, absoluteSample)
                                            || noteStateChangedAtCurrentSample;
            continue;
        }

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
                }
                else
                {
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

    processedSampleCounter += static_cast<std::int64_t>(buffer.getNumSamples());
    buffer.clear();
}

void SmartVoicingInstrumentProcessor::processMelodyHarmonizeMidi(juce::MidiBuffer& midiMessages,
                                                                  int blockSamples)
{
    struct ScheduledChordBoundary
    {
        double ppq = -1.0;
        int samplePosition = -1;
    };

    std::array<ScheduledChordBoundary, maxChordBoundariesPerBlock> boundaries {};
    int boundaryCount = 0;

    if (activeMelodyInputNote >= 0)
        refreshMelodyHarmonyAtPpq(currentBlockStartPpq, 0);

    if (currentBlockStartPpq >= 0.0 && blockSamples > 0)
    {
        auto cursorPpq = currentBlockStartPpq;
        while (boundaryCount < maxChordBoundariesPerBlock)
        {
            const auto nextPpq = harmonicContextProvider.nextChordStartAfter(cursorPpq);
            if (nextPpq < 0.0)
                break;

            const auto samplePosition = samplePositionForPpq(nextPpq);
            if (samplePosition < 0 || samplePosition >= blockSamples)
                break;

            boundaries[static_cast<std::size_t>(boundaryCount++)] = { nextPpq, samplePosition };
            cursorPpq = nextPpq;
        }
    }

    int nextBoundary = 0;
    const auto applyBoundariesBefore = [this, &boundaries, boundaryCount, &nextBoundary](int samplePosition)
    {
        while (nextBoundary < boundaryCount
               && boundaries[static_cast<std::size_t>(nextBoundary)].samplePosition < samplePosition)
        {
            const auto& boundary = boundaries[static_cast<std::size_t>(nextBoundary++)];
            if (activeMelodyInputNote >= 0)
                refreshMelodyHarmonyAtPpq(boundary.ppq, boundary.samplePosition);
        }
    };

    for (const auto metadata : midiMessages)
    {
        applyBoundariesBefore(metadata.samplePosition);

        recordMidiInputEventForProbe(metadata);

        if (isNoteMessage(metadata) && metadata.data != nullptr && metadata.numBytes >= 2)
        {
            const auto status = metadata.data[0];
            const auto type = static_cast<std::uint8_t>(status & statusMask);
            const auto note = static_cast<int>(metadata.data[1]);
            const auto velocity = metadata.numBytes > 2 ? static_cast<int>(metadata.data[2]) : 0;
            const auto isNoteOn = type == noteOnStatus && velocity > 0;
            const auto isNoteOff = type == noteOffStatus || (type == noteOnStatus && velocity == 0);

            if (isNoteOn)
            {
                startMelodyVoicing(note, velocity, metadata.samplePosition);
            }
            else if (isNoteOff && note == activeMelodyInputNote)
            {
                const auto decision = melodyGate.endNote(note);
                heldDistinctNoteCount = melodyGate.keyDown() ? 1 : 0;
                heldNoteCountForUi.store(heldDistinctNoteCount, std::memory_order_relaxed);

                if (decision.releaseVoicing)
                    stopMelodyVoicing(metadata.samplePosition);
            }

            continue;
        }

        bool newSustainState = false;
        if (getSustainState(metadata, newSustainState))
        {
            const auto decision = melodyGate.setSustain(newSustainState);
            sustainDown = newSustainState;
            sustainDownForUi.store(sustainDown, std::memory_order_relaxed);

            if (decision.releaseVoicing)
                stopMelodyVoicing(metadata.samplePosition);

            routeNonNoteEvent(metadata);
            continue;
        }

        routeNonNoteEvent(metadata);

        if (shouldClearHeldNotes(metadata))
        {
            stopMelodyVoicing(metadata.samplePosition);
            clearHeldNotes();
        }
    }

    while (nextBoundary < boundaryCount)
    {
        const auto& boundary = boundaries[static_cast<std::size_t>(nextBoundary++)];
        if (activeMelodyInputNote >= 0)
            refreshMelodyHarmonyAtPpq(boundary.ppq, boundary.samplePosition);
    }
}

void SmartVoicingInstrumentProcessor::startMelodyVoicing(int melodyNote,
                                                          int velocity,
                                                          int samplePosition)
{
    if (melodyNote < 0 || melodyNote >= midiNoteCount)
        return;

    stopMelodyVoicing(samplePosition);
    melodyGate.beginNote(melodyNote);

    const auto ppq = ppqForSamplePosition(samplePosition);
    const auto context = ppq >= 0.0
        ? harmonicContextProvider.contextAt(ppq)
        : harmonicContextProvider.currentContext();
    const auto chord = smartvoicing::harmony::normalizeChord(context.chord);
    auto voicingContext = buildClosedVoicingContext(harmonicContextProvider, context, chord, ppq);
    voicingContext.tensionLevel = activeTensionLevel;
    const auto voicing = smartvoicing::harmony::buildClosedVoicing(melodyNote, chord, voicingContext);
    const auto routedVelocity = juce::jlimit(1, 127, velocity);

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto& slot = voicing.voices[static_cast<std::size_t>(voice)];
        if (! slot.active || slot.midiNote < 0 || slot.midiNote >= midiNoteCount)
            continue;

        heldNoteVelocities[static_cast<std::size_t>(slot.midiNote)] =
            static_cast<std::uint8_t>(routedVelocity);
        pushNoteToVoice(voice, slot.midiNote, samplePosition);
    }

    activeMelodyInputNote = melodyNote;
    activeMelodyVelocity = routedVelocity;
    activeMelodyVoicing = voicing;
    heldDistinctNoteCount = 1;
    stableOwnership = getActiveVoiceCount() > 0;
    heldNoteCountForUi.store(1, std::memory_order_relaxed);
    stableOwnershipForUi.store(stableOwnership, std::memory_order_relaxed);
    ignoredExtraNoteCountForUi.store(0, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::refreshMelodyHarmonyAtPpq(double ppq,
                                                                 int samplePosition)
{
    if (activeMelodyInputNote < 0 || activeMelodyInputNote >= midiNoteCount)
        return;

    const auto context = ppq >= 0.0
        ? harmonicContextProvider.contextAt(ppq)
        : harmonicContextProvider.currentContext();
    const auto chord = smartvoicing::harmony::normalizeChord(context.chord);
    auto voicingContext = buildClosedVoicingContext(harmonicContextProvider, context, chord, ppq);
    voicingContext.tensionLevel = activeTensionLevel;
    const auto desired = smartvoicing::harmony::buildClosedVoicing(
        activeMelodyInputNote, chord, voicingContext);
    const auto plan = smartvoicing::harmony::planLowerVoiceReharmonization(activeMelodyVoicing, desired);

    if (! plan.lowerVoicesChanged)
        return;

    for (int voice = 1; voice < voiceCount; ++voice)
    {
        const auto& transition = plan.voices[static_cast<std::size_t>(voice)];
        if (transition.noteOff)
            clearVoiceStack(voice, samplePosition);
    }

    for (int voice = 1; voice < voiceCount; ++voice)
    {
        const auto& transition = plan.voices[static_cast<std::size_t>(voice)];
        if (! transition.noteOn
            || transition.newNote < 0
            || transition.newNote >= midiNoteCount)
            continue;

        heldNoteVelocities[static_cast<std::size_t>(transition.newNote)] =
            static_cast<std::uint8_t>((std::max)(1, activeMelodyVelocity));
        pushNoteToVoice(voice, transition.newNote, samplePosition);
    }

    activeMelodyVoicing = desired;
    stableOwnership = getActiveVoiceCount() > 0;
    stableOwnershipForUi.store(stableOwnership, std::memory_order_relaxed);
    reharmonizationCountForUi.fetch_add(1, std::memory_order_relaxed);
    lastReharmonizationPpqForUi.store(context.positionAvailable ? context.ppq : ppq,
                                      std::memory_order_relaxed);
    midiRevision.fetch_add(1, std::memory_order_release);
}

double SmartVoicingInstrumentProcessor::ppqForSamplePosition(int samplePosition) noexcept
{
    if (samplePosition <= 0 && currentBlockStartPpq >= 0.0)
        return currentBlockStartPpq;

    if (currentBlockStartSeconds >= 0.0 && currentSampleRate > 0.0)
    {
        const auto seconds = currentBlockStartSeconds
                           + static_cast<double>(samplePosition) / currentSampleRate;
        const auto timelinePpq = harmonicContextProvider.ppqAtSeconds(seconds);
        if (timelinePpq >= 0.0)
            return timelinePpq;
    }

    if (currentBlockStartPpq >= 0.0 && currentBlockBpm > 0.0 && currentSampleRate > 0.0)
    {
        const auto seconds = static_cast<double>(samplePosition) / currentSampleRate;
        return currentBlockStartPpq + seconds * currentBlockBpm / 60.0;
    }

    return currentBlockStartPpq;
}

int SmartVoicingInstrumentProcessor::samplePositionForPpq(double ppq) noexcept
{
    if (ppq < 0.0 || currentBlockNumSamples <= 0 || currentSampleRate <= 0.0)
        return -1;

    if (currentBlockStartSeconds >= 0.0)
    {
        const auto eventSeconds = harmonicContextProvider.secondsAtPpq(ppq);
        if (eventSeconds >= 0.0)
        {
            return smartvoicing::harmony::sampleOffsetFromTimelineSeconds(currentBlockStartSeconds,
                                                                          eventSeconds,
                                                                          currentSampleRate,
                                                                          currentBlockNumSamples);
        }
    }

    return smartvoicing::harmony::sampleOffsetFromPpq(currentBlockStartPpq,
                                                       ppq,
                                                       currentBlockBpm,
                                                       currentSampleRate,
                                                       currentBlockNumSamples);
}

void SmartVoicingInstrumentProcessor::stopMelodyVoicing(int samplePosition)
{
    for (int voice = 0; voice < voiceCount; ++voice)
        clearVoiceStack(voice, samplePosition);

    melodyGate.releaseMelody();
    activeMelodyInputNote = -1;
    activeMelodyVelocity = 0;
    activeMelodyVoicing.clear();
    heldDistinctNoteCount = 0;
    stableOwnership = false;
    chordFrameNotes.fill(-1);

    heldNoteCountForUi.store(0, std::memory_order_relaxed);
    stableOwnershipForUi.store(false, std::memory_order_relaxed);
    ignoredExtraNoteCountForUi.store(0, std::memory_order_relaxed);
}

bool SmartVoicingInstrumentProcessor::updateHeldNoteFromEvent(const juce::MidiMessageMetadata& metadata,
                                                               std::int64_t absoluteSample) noexcept
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
        {
            if (heldDistinctNoteCount == 0)
            {
                chordGestureStartSample = absoluteSample;
                pendingChordFrame = true;
                chordFrameNotes.fill(-1);
            }
            else if (chordGestureStartSample >= 0
                     && absoluteSample - chordGestureStartSample <= chordGestureWindowSamples)
            {
                pendingChordFrame = true;
            }
            else
            {
                chordGestureStartSample = -1;
            }

            ++heldDistinctNoteCount;
            ignoredChordNotes[noteIndex] = false;
        }

        if (count < 255)
            ++count;

        heldNoteVelocities[noteIndex] = static_cast<std::uint8_t>(velocity);

        if (wasPhysicallyUp && noteVoiceMasks[noteIndex] != 0)
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
        {
            heldDistinctNoteCount = (std::max)(0, heldDistinctNoteCount - 1);
            ignoredChordNotes[noteIndex] = false;

            if (chordGestureStartSample >= 0
                && absoluteSample - chordGestureStartSample <= chordGestureWindowSamples)
                pendingChordFrame = true;
        }

        heldNoteCountForUi.store(heldDistinctNoteCount, std::memory_order_relaxed);
        return true;
    }

    return false;
}

void SmartVoicingInstrumentProcessor::applyVoiceState(int samplePosition)
{
    if (pendingChordFrame)
    {
        applyChordDistributionFrame(samplePosition);
        pendingChordFrame = false;
        stableOwnership = getActiveVoiceCount() > 0;
    }

    if (stableOwnership)
        reconcileStableAssignments(samplePosition);

    if (! sustainDown && heldDistinctNoteCount == 0 && getActiveVoiceCount() == 0)
    {
        stableOwnership = false;
        chordGestureStartSample = -1;
        chordFrameNotes.fill(-1);
        ignoredChordNotes.fill(false);
    }

    sustainDownForUi.store(sustainDown, std::memory_order_relaxed);
    stableOwnershipForUi.store(stableOwnership, std::memory_order_relaxed);
    updateIgnoredExtraCountForUi();
}

std::array<int, SmartVoicingInstrumentProcessor::voiceCount>
SmartVoicingInstrumentProcessor::buildDistributionFrame() const noexcept
{
    std::array<int, voiceCount> result { -1, -1, -1, -1 };
    std::array<int, midiNoteCount> descending {};
    descending.fill(-1);
    int count = 0;

    for (int note = midiNoteCount - 1; note >= 0; --note)
    {
        if (heldNoteCounts[static_cast<std::size_t>(note)] > 0)
            descending[static_cast<std::size_t>(count++)] = note;
    }

    if (count <= 0)
        return result;

    if (activeDistributionMode == DistributionMode::topDown)
    {
        const auto useCount = (std::min)(count, voiceCount);
        for (int i = 0; i < useCount; ++i)
            result[static_cast<std::size_t>(i)] = descending[static_cast<std::size_t>(i)];
        return result;
    }

    if (activeDistributionMode == DistributionMode::bottomUp)
    {
        const auto useCount = (std::min)(count, voiceCount);
        const auto sourceStart = count - useCount;
        const auto voiceStart = voiceCount - useCount;
        for (int i = 0; i < useCount; ++i)
            result[static_cast<std::size_t>(voiceStart + i)] =
                descending[static_cast<std::size_t>(sourceStart + i)];
        return result;
    }

    if (count == 1)
    {
        result.fill(descending[0]);
    }
    else if (count == 2)
    {
        result[0] = descending[0];
        result[1] = descending[0];
        result[2] = descending[1];
        result[3] = descending[1];
    }
    else if (count == 3)
    {
        result[0] = descending[0];
        result[1] = descending[1];
        result[2] = descending[2];
        result[3] = descending[2];
    }
    else
    {
        for (int i = 0; i < voiceCount; ++i)
            result[static_cast<std::size_t>(i)] = descending[static_cast<std::size_t>(i)];
    }

    return result;
}

void SmartVoicingInstrumentProcessor::applyChordDistributionFrame(int samplePosition)
{
    const auto desired = buildDistributionFrame();

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto oldNote = chordFrameNotes[index];
        const auto newNote = desired[index];

        if (oldNote >= 0 && oldNote != newNote && voiceContainsNote(voice, oldNote))
            removeNoteFromVoice(voice, oldNote, samplePosition);

        chordFrameNotes[index] = -1;
    }

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto note = desired[index];
        if (note < 0)
            continue;

        if (voiceContainsNote(voice, note))
        {
            moveNoteToTop(voice, note);
            refreshVoiceUi(voice);
        }
        else
        {
            pushNoteToVoice(voice, note, samplePosition);
        }

        chordFrameNotes[index] = note;
    }

    std::array<bool, midiNoteCount> represented {};
    for (const auto note : desired)
    {
        if (note >= 0)
            represented[static_cast<std::size_t>(note)] = true;
    }

    for (int note = 0; note < midiNoteCount; ++note)
    {
        const auto index = static_cast<std::size_t>(note);
        if (heldNoteCounts[index] == 0)
        {
            ignoredChordNotes[index] = false;
            continue;
        }

        ignoredChordNotes[index] = ! represented[index];
    }

    processPendingRetriggers(samplePosition);
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
        if (heldNoteCounts[noteIndex] == 0)
            continue;

        const auto mask = noteVoiceMasks[noteIndex];
        for (int voice = 0; voice < voiceCount; ++voice)
        {
            if ((mask & voiceBit(voice)) == 0)
                continue;

            moveNoteToTop(voice, note);
            sendRoutedNoteOff(voice, note, samplePosition);
            sendRoutedNoteOn(voice, note, samplePosition);
            refreshVoiceUi(voice);
        }
    }
}

void SmartVoicingInstrumentProcessor::removeReleasedNotesFromStacks(int samplePosition)
{
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        auto& stackSize = voiceStackSizes[static_cast<std::size_t>(voice)];
        for (int index = stackSize - 1; index >= 0; --index)
        {
            const auto note = voiceNoteStacks[static_cast<std::size_t>(voice)][static_cast<std::size_t>(index)];
            if (note >= 0 && heldNoteCounts[static_cast<std::size_t>(note)] == 0)
                removeNoteFromVoice(voice, note, samplePosition);
        }
    }
}

void SmartVoicingInstrumentProcessor::assignUnownedHeldNotes(int samplePosition)
{
    std::array<bool, voiceCount> voiceUsed {};

    for (int note = midiNoteCount - 1; note >= 0; --note)
    {
        const auto noteIndex = static_cast<std::size_t>(note);
        if (heldNoteCounts[noteIndex] == 0
            || noteVoiceMasks[noteIndex] != 0
            || ignoredChordNotes[noteIndex])
            continue;

        const auto voice = chooseNearestVoice(note, voiceUsed);
        if (voice < 0)
        {
            ignoredChordNotes[noteIndex] = true;
            continue;
        }

        if (pushNoteToVoice(voice, note, samplePosition))
            voiceUsed[static_cast<std::size_t>(voice)] = true;
    }
}

int SmartVoicingInstrumentProcessor::chooseNearestVoice(int note,
                                                         const std::array<bool, voiceCount>& alreadyUsed) const noexcept
{
    int bestVoice = -1;
    int bestDistance = 1000;

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        if (alreadyUsed[index])
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

    if (bestVoice >= 0)
        return bestVoice;

    if (activeDistributionMode == DistributionMode::bottomUp)
    {
        for (int voice = voiceCount - 1; voice >= 0; --voice)
            if (! alreadyUsed[static_cast<std::size_t>(voice)])
                return voice;
    }
    else
    {
        for (int voice = 0; voice < voiceCount; ++voice)
            if (! alreadyUsed[static_cast<std::size_t>(voice)])
                return voice;
    }

    return -1;
}

bool SmartVoicingInstrumentProcessor::pushNoteToVoice(int voice,
                                                       int note,
                                                       int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return false;

    const auto voiceIndex = static_cast<std::size_t>(voice);
    const auto noteIndex = static_cast<std::size_t>(note);
    const auto bit = voiceBit(voice);

    if ((noteVoiceMasks[noteIndex] & bit) != 0)
    {
        moveNoteToTop(voice, note);
        refreshVoiceUi(voice);
        return true;
    }

    auto& stackSize = voiceStackSizes[voiceIndex];
    if (stackSize >= maxVoiceStackDepth)
        return false;

    voiceNoteStacks[voiceIndex][static_cast<std::size_t>(stackSize++)] = note;
    noteVoiceMasks[noteIndex] = static_cast<std::uint8_t>(noteVoiceMasks[noteIndex] | bit);
    ignoredChordNotes[noteIndex] = false;

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
    noteVoiceMasks[noteIndex] = static_cast<std::uint8_t>(noteVoiceMasks[noteIndex] & ~voiceBit(voice));

    if (chordFrameNotes[voiceIndex] == note)
        chordFrameNotes[voiceIndex] = -1;

    if (noteVoiceMasks[noteIndex] == 0)
    {
        retriggerPending[noteIndex] = false;
        if (heldNoteCounts[noteIndex] == 0)
            heldNoteVelocities[noteIndex] = 0;
    }

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

void SmartVoicingInstrumentProcessor::moveNoteToTop(int voice, int note) noexcept
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
}

bool SmartVoicingInstrumentProcessor::voiceContainsNote(int voice, int note) const noexcept
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return false;

    return (noteVoiceMasks[static_cast<std::size_t>(note)] & voiceBit(voice)) != 0;
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
    const auto bit = voiceBit(voice);
    const auto velocity = heldNoteVelocities[noteIndex];
    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOnStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        velocity > 0 ? velocity : static_cast<std::uint8_t>(1)
    };

    addOutputEvent(bytes, 3, samplePosition);
    routedNoteVoiceMasks[noteIndex] = static_cast<std::uint8_t>(routedNoteVoiceMasks[noteIndex] | bit);
}

void SmartVoicingInstrumentProcessor::sendRoutedNoteOff(int voice,
                                                         int note,
                                                         int samplePosition)
{
    if (voice < 0 || voice >= voiceCount || note < 0 || note >= midiNoteCount)
        return;

    const auto noteIndex = static_cast<std::size_t>(note);
    const auto bit = voiceBit(voice);
    if ((routedNoteVoiceMasks[noteIndex] & bit) == 0)
        return;

    const std::uint8_t bytes[3] {
        static_cast<std::uint8_t>(noteOffStatus | ((firstOutputChannel + voice - 1) & channelMask)),
        static_cast<std::uint8_t>(note),
        0
    };

    addOutputEvent(bytes, 3, samplePosition);
    routedNoteVoiceMasks[noteIndex] = static_cast<std::uint8_t>(routedNoteVoiceMasks[noteIndex] & ~bit);
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
    return controller == 120 || controller == 123;
}

void SmartVoicingInstrumentProcessor::clearHeldNotes() noexcept
{
    heldNoteCounts.fill(0);
    heldNoteVelocities.fill(0);
    noteVoiceMasks.fill(0);
    routedNoteVoiceMasks.fill(0);
    retriggerPending.fill(false);
    ignoredChordNotes.fill(false);

    for (auto& stack : voiceNoteStacks)
        stack.fill(-1);
    voiceStackSizes.fill(0);
    chordFrameNotes.fill(-1);

    heldDistinctNoteCount = 0;
    activeMelodyInputNote = -1;
    activeMelodyVelocity = 0;
    activeMelodyVoicing.clear();
    melodyGate.reset();
    sustainDown = false;
    stableOwnership = false;
    pendingChordFrame = false;
    chordGestureStartSample = -1;

    heldNoteCountForUi.store(0, std::memory_order_relaxed);
    ignoredExtraNoteCountForUi.store(0, std::memory_order_relaxed);
    sustainDownForUi.store(false, std::memory_order_relaxed);
    stableOwnershipForUi.store(false, std::memory_order_relaxed);

    for (int voice = 0; voice < voiceCount; ++voice)
        refreshVoiceUi(voice);
}

void SmartVoicingInstrumentProcessor::updateIgnoredExtraCountForUi() noexcept
{
    int count = 0;
    for (int note = 0; note < midiNoteCount; ++note)
    {
        const auto index = static_cast<std::size_t>(note);
        if (ignoredChordNotes[index] && heldNoteCounts[index] > 0)
            ++count;
    }
    ignoredExtraNoteCountForUi.store(count, std::memory_order_relaxed);
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
    snapshot.reharmonizationCount = reharmonizationCountForUi.load(std::memory_order_relaxed);
    snapshot.lastReharmonizationPpq = lastReharmonizationPpqForUi.load(std::memory_order_relaxed);
    snapshot.lastEventType = static_cast<MidiProbeEventType>(lastMidiEventType.load(std::memory_order_relaxed));
    snapshot.lastChannel = lastMidiChannel.load(std::memory_order_relaxed);
    snapshot.lastData1 = lastMidiData1.load(std::memory_order_relaxed);
    snapshot.lastData2 = lastMidiData2.load(std::memory_order_relaxed);
    snapshot.heldNoteCount = heldNoteCountForUi.load(std::memory_order_relaxed);
    snapshot.ignoredExtraNoteCount = ignoredExtraNoteCountForUi.load(std::memory_order_relaxed);
    snapshot.sustainDown = sustainDownForUi.load(std::memory_order_relaxed);
    snapshot.stableOwnership = stableOwnershipForUi.load(std::memory_order_relaxed);
    snapshot.distributionMode = getDistributionMode();
    snapshot.harmonyMode = getHarmonyMode();
    snapshot.tensionLevel = getTensionLevel();

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
    reharmonizationCountForUi.store(0, std::memory_order_relaxed);
    lastReharmonizationPpqForUi.store(-1.0, std::memory_order_relaxed);
    lastMidiEventType.store(static_cast<int>(MidiProbeEventType::none), std::memory_order_relaxed);
    lastMidiChannel.store(0, std::memory_order_relaxed);
    lastMidiData1.store(0, std::memory_order_relaxed);
    lastMidiData2.store(0, std::memory_order_relaxed);
}

void SmartVoicingInstrumentProcessor::setDistributionMode(DistributionMode mode) noexcept
{
    const auto value = juce::jlimit(0, 2, static_cast<int>(mode));
    requestedDistributionMode.store(value, std::memory_order_release);
}

SmartVoicingInstrumentProcessor::DistributionMode SmartVoicingInstrumentProcessor::getDistributionMode() const noexcept
{
    const auto value = juce::jlimit(0, 2, requestedDistributionMode.load(std::memory_order_acquire));
    return static_cast<DistributionMode>(value);
}

void SmartVoicingInstrumentProcessor::setHarmonyMode(HarmonyMode mode) noexcept
{
    const auto value = juce::jlimit(0, 1, static_cast<int>(mode));
    requestedHarmonyMode.store(value, std::memory_order_release);
}

SmartVoicingInstrumentProcessor::HarmonyMode SmartVoicingInstrumentProcessor::getHarmonyMode() const noexcept
{
    const auto value = juce::jlimit(0, 1, requestedHarmonyMode.load(std::memory_order_acquire));
    return static_cast<HarmonyMode>(value);
}

void SmartVoicingInstrumentProcessor::setTensionLevel(smartvoicing::harmony::TensionLevel level) noexcept
{
    const auto value = juce::jlimit(
        static_cast<int>(smartvoicing::harmony::TensionLevel::clean),
        static_cast<int>(smartvoicing::harmony::TensionLevel::rich),
        static_cast<int>(level));
    requestedTensionLevel.store(value, std::memory_order_release);
}

smartvoicing::harmony::TensionLevel SmartVoicingInstrumentProcessor::getTensionLevel() const noexcept
{
    const auto value = juce::jlimit(
        static_cast<int>(smartvoicing::harmony::TensionLevel::clean),
        static_cast<int>(smartvoicing::harmony::TensionLevel::rich),
        requestedTensionLevel.load(std::memory_order_acquire));
    return static_cast<smartvoicing::harmony::TensionLevel>(value);
}

void SmartVoicingInstrumentProcessor::resetRouterState() noexcept
{
    clearHeldNotes();
    activeDistributionMode = getDistributionMode();
    activeHarmonyMode = getHarmonyMode();
    activeTensionLevel = getTensionLevel();
}

juce::AudioProcessorEditor* SmartVoicingInstrumentProcessor::createEditor()
{
    return new SmartVoicingInstrumentEditor(*this);
}

void SmartVoicingInstrumentProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    stream.writeInt(stateMagic);
    stream.writeInt(stateVersion);
    stream.writeInt(static_cast<int>(getDistributionMode()));
    stream.writeInt(static_cast<int>(getHarmonyMode()));
    stream.writeInt(static_cast<int>(getTensionLevel()));
}

void SmartVoicingInstrumentProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes < 12)
        return;

    juce::MemoryInputStream stream(data, static_cast<std::size_t>(sizeInBytes), false);
    if (stream.readInt() != stateMagic)
        return;

    const auto version = stream.readInt();
    if (version < 2)
        return;

    setDistributionMode(static_cast<DistributionMode>(juce::jlimit(0, 2, stream.readInt())));

    if (version >= 3 && sizeInBytes >= 16)
        setHarmonyMode(static_cast<HarmonyMode>(juce::jlimit(0, 1, stream.readInt())));
    else
        setHarmonyMode(HarmonyMode::directRouter);

    if (version >= 4 && sizeInBytes >= 20)
    {
        const auto value = juce::jlimit(
            static_cast<int>(smartvoicing::harmony::TensionLevel::clean),
            static_cast<int>(smartvoicing::harmony::TensionLevel::rich),
            stream.readInt());
        setTensionLevel(static_cast<smartvoicing::harmony::TensionLevel>(value));
    }
    else
    {
        setTensionLevel(smartvoicing::harmony::TensionLevel::clean);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmartVoicingInstrumentProcessor();
}
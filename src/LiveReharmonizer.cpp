#include "LiveReharmonizer.h"

#include <cmath>

namespace smartvoicing::harmony
{
namespace
{
int slotNote(const VoiceSlot& slot) noexcept
{
    return slot.active ? slot.midiNote : -1;
}

int roundedSampleOffset(double exactSamples, int blockSamples) noexcept
{
    if (! std::isfinite(exactSamples) || blockSamples <= 0)
        return -1;

    const auto offset = static_cast<long long>(std::llround(exactSamples));
    if (offset < 0 || offset >= blockSamples)
        return -1;

    return static_cast<int>(offset);
}
}

namespace
{
bool sameChord(const ChordContext& left, const ChordContext& right) noexcept
{
    return left.available == right.available && left.defined == right.defined
        && left.startPpq == right.startPpq && left.root == right.root
        && left.bass == right.bass
        && left.intervals.values == right.intervals.values;
}

bool sameKey(const KeyContext& left, const KeyContext& right) noexcept
{
    return left.available == right.available && left.defined == right.defined
        && left.startPpq == right.startPpq && left.root == right.root
        && left.intervals.values == right.intervals.values;
}
}

void MelodyHarmonyRefreshGuard::reset() noexcept
{
    initialized = false;
    last = {};
    appliedVoicingMode = -1;
    appliedTensionLevel = -1;
}

bool MelodyHarmonyRefreshGuard::needsRefresh(
    const HarmonicContext& context, int voicingMode, int tensionLevel) const noexcept
{
    if (! initialized)
        return true;

    // Intentionally exclude current position in PPQ and elapsed time.
    return context.harmonicRevision != last.harmonicRevision
        || context.transportRevision != last.transportRevision
        || context.providerConnected != last.providerConnected
        || context.positionAvailable != last.positionAvailable
        || ! sameChord(context.chord, last.chord)
        || ! sameKey(context.key, last.key)
        || voicingMode != appliedVoicingMode
        || tensionLevel != appliedTensionLevel;
}

void MelodyHarmonyRefreshGuard::commit(
    const HarmonicContext& context, int voicingMode, int tensionLevel) noexcept
{
    last = context;
    initialized = true;
    appliedVoicingMode = voicingMode;
    appliedTensionLevel = tensionLevel;
}

double chooseChordPpqAtMelodySample(double computedPpq, int samplePosition,
                                   double boundaryPpq, int boundarySample) noexcept
{
    return boundaryPpq >= 0.0 && boundarySample >= 0
               && boundarySample == samplePosition
        ? boundaryPpq : computedPpq;
}

void MelodyGateState::reset() noexcept
{
    currentNote = -1;
    physicalKeyDown = false;
    pedalDown = false;
}

void MelodyGateState::beginNote(int midiNote) noexcept
{
    currentNote = midiNote;
    physicalKeyDown = midiNote >= 0;
}

MelodyGateDecision MelodyGateState::endNote(int midiNote) noexcept
{
    MelodyGateDecision decision;
    if (midiNote != currentNote || currentNote < 0)
        return decision;

    physicalKeyDown = false;
    if (! pedalDown)
    {
        decision.releaseVoicing = true;
        releaseMelody();
    }

    return decision;
}

MelodyGateDecision MelodyGateState::setSustain(bool down) noexcept
{
    MelodyGateDecision decision;
    if (pedalDown == down)
        return decision;

    pedalDown = down;
    if (! pedalDown && currentNote >= 0 && ! physicalKeyDown)
    {
        decision.releaseVoicing = true;
        releaseMelody();
    }

    return decision;
}

void MelodyGateState::releaseMelody() noexcept
{
    currentNote = -1;
    physicalKeyDown = false;
}

ReharmonizationPlan planLowerVoiceReharmonization(const VoiceOutput& current,
                                                  const VoiceOutput& desired) noexcept
{
    ReharmonizationPlan plan;

    // V1 belongs to the played melody and must never be rewritten by a chord
    // transition. Only generated lower voices participate in reharmonization.
    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto oldNote = slotNote(current.voices[index]);
        const auto newNote = slotNote(desired.voices[index]);

        if (oldNote == newNote)
            continue;

        auto& transition = plan.voices[index];
        transition.oldNote = oldNote;
        transition.newNote = newNote;
        transition.noteOff = oldNote >= 0;
        transition.noteOn = newNote >= 0;
        plan.lowerVoicesChanged = true;
    }

    return plan;
}

ReharmonizationPlan planVoicingTransition(const VoiceOutput& current,
                                          const VoiceOutput& desired,
                                          bool retriggerMelody,
                                          bool retriggerMatchingLowerVoices) noexcept
{
    ReharmonizationPlan plan;

    for (int voice = 0; voice < kVoiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto oldNote = slotNote(current.voices[index]);
        const auto newNote = slotNote(desired.voices[index]);
        const auto matchingActiveNote = oldNote >= 0 && oldNote == newNote;
        const auto forceMelodyRetrigger = voice == 0
                                       && retriggerMelody
                                       && matchingActiveNote;
        const auto forceLowerRetrigger = voice > 0
                                      && retriggerMatchingLowerVoices
                                      && matchingActiveNote;

        if (oldNote == newNote && ! forceMelodyRetrigger && ! forceLowerRetrigger)
            continue;

        auto& transition = plan.voices[index];
        transition.oldNote = oldNote;
        transition.newNote = newNote;
        transition.noteOff = oldNote >= 0;
        transition.noteOn = newNote >= 0;

        if (voice > 0)
            plan.lowerVoicesChanged = true;
    }

    return plan;
}

int sampleOffsetFromTimelineSeconds(double blockStartSeconds,
                                    double eventSeconds,
                                    double sampleRate,
                                    int blockSamples) noexcept
{
    if (blockStartSeconds < 0.0 || eventSeconds < 0.0 || sampleRate <= 0.0)
        return -1;

    return roundedSampleOffset((eventSeconds - blockStartSeconds) * sampleRate,
                               blockSamples);
}

int sampleOffsetFromPpq(double blockStartPpq,
                        double eventPpq,
                        double bpm,
                        double sampleRate,
                        int blockSamples) noexcept
{
    if (blockStartPpq < 0.0 || eventPpq < 0.0 || bpm <= 0.0 || sampleRate <= 0.0)
        return -1;

    const auto secondsPerQuarter = 60.0 / bpm;
    return roundedSampleOffset((eventPpq - blockStartPpq) * secondsPerQuarter * sampleRate,
                               blockSamples);
}
}

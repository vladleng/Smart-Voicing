#include "VoiceLeading.h"

namespace smartvoicing::harmony
{
namespace
{
bool validSlot(const VoiceSlot& slot) noexcept
{
    return ! slot.active || (slot.midiNote >= 0 && slot.midiNote <= 127);
}

int noteDistance(int first, int second) noexcept
{
    const auto delta = first - second;
    return delta < 0 ? -delta : delta;
}

int motionCost(int semitones) noexcept
{
    if (semitones == 0)
        return 0;
    if (semitones <= 2)
        return 6 * semitones;
    if (semitones <= 4)
        return 28 + 8 * (semitones - 3);
    if (semitones <= 7)
        return 60 + 14 * (semitones - 5);
    return 100 + 20 * (semitones - 8);
}
}

void VoiceLeadingState::reset() noexcept
{
    last.clear();
    available = false;
}

void VoiceLeadingState::commit(const VoiceOutput& sounding) noexcept
{
    // Do not treat V1-only (no chord) as a four-voice harmonic predecessor.
    if (! sounding.voices[0].active || ! validSlot(sounding.voices[0]))
    {
        reset();
        return;
    }

    bool hasGeneratedVoice = false;
    for (int index = 1; index < kVoiceCount; ++index)
    {
        const auto& slot = sounding.voices[static_cast<std::size_t>(index)];
        if (! validSlot(slot))
        {
            reset();
            return;
        }

        hasGeneratedVoice = hasGeneratedVoice || slot.active;
    }

    if (! hasGeneratedVoice)
    {
        reset();
        return;
    }

    last = sounding;
    available = true;
}

VoiceLeadingScore evaluateVoiceLeading(const VoiceOutput& candidate,
                                      const VoiceLeadingState& state) noexcept
{
    VoiceLeadingScore score;
    if (! candidate.voices[0].active)
    {
        score.valid = false;
        return score;
    }

    for (const auto& slot : candidate.voices)
    {
        if (! validSlot(slot))
        {
            score.valid = false;
            return score;
        }
    }

    // All current Stage 5 harmonic strategies expose sounding slots V1..V4
    // from upper to lower. This is a safety preference, not a replacement for
    // strategy-specific spacing/vertical validation.
    for (int index = 1; index < kVoiceCount; ++index)
    {
        const auto& upper = candidate.voices[static_cast<std::size_t>(index - 1)];
        const auto& lower = candidate.voices[static_cast<std::size_t>(index)];
        if (upper.active && lower.active && upper.midiNote < lower.midiNote)
        {
            ++score.crossings;
            score.cost += 400 + 20 * (lower.midiNote - upper.midiNote);
        }
    }

    if (! state.hasPrevious())
        return score;

    // Preserve abstract identity: one voice slot corresponds to one line.
    // V1 is immutable source material and is NOT scored as an editable line.
    for (int index = 1; index < kVoiceCount; ++index)
    {
        const auto& oldSlot = state.previous().voices[static_cast<std::size_t>(index)];
        const auto& newSlot = candidate.voices[static_cast<std::size_t>(index)];

        if (! oldSlot.active && ! newSlot.active)
            continue;
        if (! oldSlot.active || ! newSlot.active)
        {
            score.cost += 24;
            continue;
        }

        const auto semitones = noteDistance(oldSlot.midiNote, newSlot.midiNote);
        if (semitones == 0)
            ++score.commonTones;
        else if (semitones <= 2)
            ++score.stepwiseMoves;
        else if (semitones >= 5)
            ++score.largeLeaps;

        score.cost += motionCost(semitones);
    }

    return score;
}
int voiceLeadingAdjustment(const VoiceOutput& candidate,
                           const VoiceLeadingState* state) noexcept
{
    if (state == nullptr || ! state->hasPrevious())
        return 0;

    const auto motion = evaluateVoiceLeading(candidate, *state);
    if (! motion.valid)
        return 10; // never offer an invalid candidate any continuity reward

    // Bounded by ten vertical-score points: continuity breaks near-ties,
    // but never becomes a replacement for Stage 4/5 harmonic correctness.
    const auto adjusted = (motion.cost + 3) / 6;
    return adjusted < 10 ? adjusted : 10;
}

int voiceLeadingAdjustment(const std::array<int, kVoiceCount>& notes,
                           const VoiceLeadingState* state) noexcept
{
    if (state == nullptr || ! state->hasPrevious())
        return 0;

    VoiceOutput candidate;
    for (int index = 0; index < kVoiceCount; ++index)
    {
        const auto note = notes[static_cast<std::size_t>(index)];
        if (note >= 0 && note <= 127)
            candidate.voices[static_cast<std::size_t>(index)] = { true, note };
    }
    return voiceLeadingAdjustment(candidate, state);
}

} // namespace smartvoicing::harmony

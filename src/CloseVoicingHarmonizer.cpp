#include "CloseVoicingHarmonizer.h"

#include <array>
#include <climits>

namespace smartvoicing::harmony
{
namespace
{
constexpr int kClosedSearchDepthSemitones = 24;
constexpr int kInvalidScore = INT_MAX / 4;

int midiPitchClass(int note) noexcept
{
    auto value = note % kPitchClassCount;
    if (value < 0)
        value += kPitchClassCount;
    return value;
}

int relativeToChordRoot(int midiNote, const NormalizedChord& chord) noexcept
{
    auto relative = midiPitchClass(midiNote) - chord.rootPitchClass;
    if (relative < 0)
        relative += kPitchClassCount;
    return relative;
}

bool isChordTone(int midiNote, const NormalizedChord& chord) noexcept
{
    if (midiNote < 0 || midiNote > 127 || ! chord.valid)
        return false;

    return chord.hasTone(relativeToChordRoot(midiNote, chord));
}

bool isThirdRole(int relative, const NormalizedChord& chord) noexcept
{
    if (relative < 0 || relative >= kPitchClassCount || ! chord.hasTone(relative))
        return false;

    const auto degree = chord.degrees[static_cast<std::size_t>(relative)];
    if (degree == 3)
        return true;
    if (degree == 9)
        return false; // explicit #9 is colour, not the chord's minor third

    switch (chord.quality)
    {
        case ChordQuality::major:
        case ChordQuality::dominant:
        case ChordQuality::augmented:
            return relative == 4;

        case ChordQuality::minor:
        case ChordQuality::diminished:
        case ChordQuality::halfDiminished:
            return relative == 3;

        default:
            return false;
    }
}

bool isSeventhRole(int relative, const NormalizedChord& chord) noexcept
{
    if (relative < 0 || relative >= kPitchClassCount || ! chord.hasTone(relative))
        return false;

    const auto degree = chord.degrees[static_cast<std::size_t>(relative)];
    if (degree == 7)
        return true;

    return relative == 10 || relative == 11;
}

bool isExplicitColourTone(int relative, const NormalizedChord& chord) noexcept
{
    if (relative < 0 || relative >= kPitchClassCount || ! chord.hasTone(relative))
        return false;

    const auto degree = chord.degrees[static_cast<std::size_t>(relative)];
    return degree == 9 || degree == 11 || degree == 13;
}

bool chordHasThirdRole(const NormalizedChord& chord) noexcept
{
    for (int relative = 0; relative < kPitchClassCount; ++relative)
        if (isThirdRole(relative, chord))
            return true;
    return false;
}

bool chordHasSeventhRole(const NormalizedChord& chord) noexcept
{
    for (int relative = 0; relative < kPitchClassCount; ++relative)
        if (isSeventhRole(relative, chord))
            return true;
    return false;
}

bool allowsInferredTensions(const NormalizedChord& chord) noexcept
{
    // 0.3d deliberately keeps plain triads conservative. Seventh harmony and
    // explicitly extended sonorities are rich enough to admit inferred tension
    // candidates without silently turning every triad into add9/add13.
    return chordHasSeventhRole(chord)
        || chord.hasExtension(ChordExtension::ninth)
        || chord.hasExtension(ChordExtension::eleventh)
        || chord.hasExtension(ChordExtension::thirteenth);
}

bool isGeneratedHarmonyCandidate(int midiNote,
                                 const NormalizedChord& chord,
                                 const ClosedVoicingContext& context) noexcept
{
    if (midiNote < 0 || midiNote > 127 || ! chord.valid)
        return false;

    const auto relative = relativeToChordRoot(midiNote, chord);
    if (chord.hasTone(relative))
        return true;

    if (! context.tension.valid || ! allowsInferredTensions(chord))
        return false;

    return context.tension.isHarmonyCandidate(relative, context.tensionLevel);
}

bool selectedRole(const std::array<int, kVoiceCount>& notes,
                  const NormalizedChord& chord,
                  bool (*predicate)(int, const NormalizedChord&) noexcept) noexcept
{
    for (const auto note : notes)
    {
        if (note < 0)
            continue;
        if (predicate(relativeToChordRoot(note, chord), chord))
            return true;
    }
    return false;
}

bool selectedRoot(const std::array<int, kVoiceCount>& notes,
                  const NormalizedChord& chord) noexcept
{
    for (const auto note : notes)
        if (note >= 0 && relativeToChordRoot(note, chord) == 0)
            return true;
    return false;
}

int upperSpacingPenalty(int semitones) noexcept
{
    // Soft preference only. Around a third is preferred, a fourth is common,
    // while seconds / wider intervals remain legal when the vertical context
    // makes them the better musical choice. A semitone is contextual rather
    // than forbidden: in real four-way-close writing (for example root over
    // maj7) it can be the most compact and correct upper spacing.
    switch (semitones)
    {
        case 3:
        case 4: return 0;
        case 5: return 1;
        case 2: return 4;
        case 1: return 6;
        case 6: return 5;
        case 7: return 7;
        default:
            return semitones > 7 ? 9 + (semitones - 8) * 2 : 0;
    }
}

int innerSpacingPenalty(int semitones) noexcept
{
    // Closed writing accepts compact seconds/thirds/fourths. Wider inner gaps
    // are not forbidden, but they gradually lose the compact-section preference.
    return semitones <= 5 ? 0 : (semitones - 5) * 2;
}

int duplicatePitchClassPenalty(const std::array<int, kVoiceCount>& notes) noexcept
{
    std::array<int, kPitchClassCount> counts {};
    for (const auto note : notes)
        if (note >= 0)
            ++counts[static_cast<std::size_t>(midiPitchClass(note))];

    int penalty = 0;
    for (const auto count : counts)
        if (count > 1)
            penalty += (count - 1) * 12;
    return penalty;
}

int colourToneReward(const std::array<int, kVoiceCount>& notes,
                     const NormalizedChord& chord) noexcept
{
    int reward = 0;
    // V1 is performer-owned. Reward only generated voices for preserving an
    // explicit chord colour when several structurally valid candidates exist.
    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        const auto note = notes[static_cast<std::size_t>(voice)];
        if (note < 0)
            continue;
        if (isExplicitColourTone(relativeToChordRoot(note, chord), chord))
            reward += 3;
    }
    return reward;
}

int tensionRolePenalty(int midiNote,
                       const NormalizedChord& chord,
                       const ClosedVoicingContext& context) noexcept
{
    if (! context.tension.valid || midiNote < 0)
        return 0;

    const auto relative = relativeToChordRoot(midiNote, chord);
    const auto& tone = context.tension.tone(relative);

    switch (tone.role)
    {
        case TensionRole::chordTone:
            return 0;

        case TensionRole::explicitTension:
            return -3; // explicit Chord Track colour is authoritative at every level

        case TensionRole::preferred:
            switch (context.tensionLevel)
            {
                case TensionLevel::clean: return 18;
                case TensionLevel::color: return 1;
                case TensionLevel::rich:  return -1;
            }
            break;

        case TensionRole::available:
            switch (context.tensionLevel)
            {
                case TensionLevel::clean: return 20;
                case TensionLevel::color: return 3;
                case TensionLevel::rich:  return 1;
            }
            break;

        case TensionRole::contextual:
            if (context.tensionLevel == TensionLevel::rich)
                return tone.alteredCandidate ? 5 : 3;
            return 24;

        case TensionRole::avoidAsHarmony:
        case TensionRole::unavailable:
            return 24;
    }

    return 0;
}

int inferredColourDensityPenalty(const std::array<int, kVoiceCount>& notes,
                                 const NormalizedChord& chord,
                                 const ClosedVoicingContext& context) noexcept
{
    if (! context.tension.valid || context.tensionLevel == TensionLevel::clean)
        return 0;

    int inferredCount = 0;
    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        const auto note = notes[static_cast<std::size_t>(voice)];
        if (note < 0)
            continue;

        const auto relative = relativeToChordRoot(note, chord);
        if (chord.hasTone(relative))
            continue;

        const auto& tone = context.tension.tone(relative);
        if (tone.role == TensionRole::explicitTension)
            continue;

        if (tone.role == TensionRole::preferred
            || tone.role == TensionRole::available
            || tone.role == TensionRole::contextual)
            ++inferredCount;
    }

    if (inferredCount <= 1)
        return 0;

    // A higher level expands choice; it does not make "more tensions" a goal.
    // Color is deliberately conservative until Stage 6 can justify colour with
    // previous-voice continuity. Rich permits denser colour but still resists
    // gratuitously replacing several structural notes at once.
    const auto extra = inferredCount - 1;
    return context.tensionLevel == TensionLevel::color ? extra * 6 : extra * 2;
}

bool isExplicitTensionNote(int midiNote,
                           const NormalizedChord& chord,
                           const ClosedVoicingContext& context) noexcept
{
    if (! context.tension.valid || midiNote < 0)
        return false;

    const auto relative = relativeToChordRoot(midiNote, chord);
    return context.tension.tone(relative).role == TensionRole::explicitTension;
}

int minorNinthPenalty(const std::array<int, kVoiceCount>& notes,
                      const NormalizedChord& chord,
                      const ClosedVoicingContext& context) noexcept
{
    int penalty = 0;
    for (int upper = 0; upper < kVoiceCount; ++upper)
    {
        for (int lower = upper + 1; lower < kVoiceCount; ++lower)
        {
            const auto upperNote = notes[static_cast<std::size_t>(upper)];
            const auto lowerNote = notes[static_cast<std::size_t>(lower)];
            if (upperNote < 0 || lowerNote < 0)
                continue;

            const auto distance = upperNote - lowerNote;
            if (distance < 13 || (distance % 12) != 1)
                continue;

            // Explicit altered tensions such as b9/b13 are intentional evidence
            // from Chord Track and bypass the generic minor-ninth penalty.
            if (isExplicitTensionNote(upperNote, chord, context)
                || isExplicitTensionNote(lowerNote, chord, context))
                continue;

            penalty += 10;
        }
    }
    return penalty;
}

int scoreClosedCandidate(const std::array<int, kVoiceCount>& notes,
                         const NormalizedChord& chord,
                         const ClosedVoicingContext& context) noexcept
{
    const auto melody = notes[0];
    const auto v2 = notes[1];
    const auto v3 = notes[2];
    const auto v4 = notes[3];

    if (melody < 0 || v2 < 0 || v3 < 0 || v4 < 0
        || !(melody > v2 && v2 > v3 && v3 > v4))
        return kInvalidScore;

    if (! isGeneratedHarmonyCandidate(v2, chord, context)
        || ! isGeneratedHarmonyCandidate(v3, chord, context))
        return kInvalidScore;

    if (chord.slashBass)
    {
        if (midiPitchClass(v4) != chord.bassPitchClass)
            return kInvalidScore;
    }
    else if (! isGeneratedHarmonyCandidate(v4, chord, context))
    {
        return kInvalidScore;
    }

    int score = 0;

    score += upperSpacingPenalty(melody - v2);
    score += innerSpacingPenalty(v2 - v3);
    score += innerSpacingPenalty(v3 - v4);

    const auto totalSpan = melody - v4;
    if (totalSpan > 12)
        score += (totalSpan - 12) * 7;

    score += duplicatePitchClassPenalty(notes);
    score += minorNinthPenalty(notes, chord, context);
    score += inferredColourDensityPenalty(notes, chord, context);

    for (int voice = 1; voice < kVoiceCount; ++voice)
        score += tensionRolePenalty(notes[static_cast<std::size_t>(voice)], chord, context);

    const auto hasThird = chordHasThirdRole(chord);
    const auto hasSeventh = chordHasSeventhRole(chord);
    const auto selectedThird = selectedRole(notes, chord, isThirdRole);
    const auto selectedSeventh = selectedRole(notes, chord, isSeventhRole);

    // Guide tones are especially important on dominant-function chords. Tension
    // Policy may colour the vertical, but it must not casually replace 3/7.
    const auto dominantFunction = context.harmonic.valid
                               && context.harmonic.effectiveFunction == HarmonicFunction::dominant;
    const auto guidePenalty = dominantFunction ? 32 : 22;

    if (hasThird && ! selectedThird)
        score += guidePenalty;
    if (hasSeventh && ! selectedSeventh)
        score += guidePenalty;

    // Root omission is normal for seventh/extended harmony. For a plain triad,
    // however, retaining the root helps preserve identity.
    if (! hasSeventh && ! selectedRoot(notes, chord))
        score += 16;

    // The fifth intentionally has no required-presence penalty. It is the first
    // expendable structural tone when guide tones / explicit colours need room.
    score -= colourToneReward(notes, chord);

    return score;
}

int nearestChordToneBelow(int upperExclusive,
                          const NormalizedChord& chord) noexcept
{
    for (int note = upperExclusive - 1; note >= 0; --note)
        if (isChordTone(note, chord))
            return note;
    return -1;
}

int nearestPitchClassBelow(int upperExclusive, int pitchClass) noexcept
{
    for (int note = upperExclusive - 1; note >= 0; --note)
        if (midiPitchClass(note) == pitchClass)
            return note;
    return -1;
}

VoiceOutput fallbackTopDown(int melodyNote, const NormalizedChord& chord) noexcept
{
    VoiceOutput output;
    output.clear();
    output.voices[0] = { true, melodyNote };

    auto upperExclusive = melodyNote;
    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        auto note = nearestChordToneBelow(upperExclusive, chord);
        if (note < 0)
            break;

        output.voices[static_cast<std::size_t>(voice)] = { true, note };
        upperExclusive = note;
    }

    if (chord.slashBass && output.voices[2].active)
    {
        const auto bassNote = nearestPitchClassBelow(output.voices[2].midiNote,
                                                     chord.bassPitchClass);
        if (bassNote >= 0)
            output.voices[3] = { true, bassNote };
    }

    return output;
}
}

VoiceOutput buildClosedVoicing(int melodyNote,
                               const NormalizedChord& chord,
                               const ClosedVoicingContext& context) noexcept
{
    VoiceOutput output;
    output.clear();

    if (melodyNote < 0 || melodyNote > 127)
        return output;

    output.voices[0] = { true, melodyNote };

    // Same safe fallback as Stage 3: never invent harmony from Key alone.
    if (! chord.valid)
        return output;

    auto resolvedContext = context;
    if (! resolvedContext.tension.valid)
    {
        resolvedContext.tension = buildTensionPolicy(chord,
                                                     resolvedContext.key,
                                                     resolvedContext.harmonic,
                                                     melodyNote);
    }

    const auto lowerLimit = (melodyNote - kClosedSearchDepthSemitones > 0)
        ? melodyNote - kClosedSearchDepthSemitones
        : 0;

    int bestScore = kInvalidScore;
    std::array<int, kVoiceCount> bestNotes { melodyNote, -1, -1, -1 };

    for (int v2 = melodyNote - 1; v2 >= lowerLimit; --v2)
    {
        if (! isGeneratedHarmonyCandidate(v2, chord, resolvedContext))
            continue;

        for (int v3 = v2 - 1; v3 >= lowerLimit; --v3)
        {
            if (! isGeneratedHarmonyCandidate(v3, chord, resolvedContext))
                continue;

            for (int v4 = v3 - 1; v4 >= lowerLimit; --v4)
            {
                if (chord.slashBass)
                {
                    if (midiPitchClass(v4) != chord.bassPitchClass)
                        continue;
                }
                else if (! isGeneratedHarmonyCandidate(v4, chord, resolvedContext))
                {
                    continue;
                }

                const std::array<int, kVoiceCount> candidate { melodyNote, v2, v3, v4 };
                const auto score = scoreClosedCandidate(candidate, chord, resolvedContext);
                if (score < bestScore)
                {
                    bestScore = score;
                    bestNotes = candidate;
                }
            }
        }
    }

    if (bestScore == kInvalidScore)
        return fallbackTopDown(melodyNote, chord);

    for (int voice = 0; voice < kVoiceCount; ++voice)
    {
        output.voices[static_cast<std::size_t>(voice)].active = true;
        output.voices[static_cast<std::size_t>(voice)].midiNote =
            bestNotes[static_cast<std::size_t>(voice)];
    }

    return output;
}

VoiceOutput buildCloseVoicing(int melodyNote, const NormalizedChord& chord) noexcept
{
    ClosedVoicingContext context;
    return buildClosedVoicing(melodyNote, chord, context);
}
}

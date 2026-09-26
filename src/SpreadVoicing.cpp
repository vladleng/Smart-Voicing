#include "SpreadVoicing.h"

#include <array>
#include <climits>

namespace smartvoicing::harmony
{
namespace
{
int pitchClass(int note) noexcept
{
    return (note % 12 + 12) % 12;
}

int relative(int note, const NormalizedChord& chord) noexcept
{
    return (pitchClass(note) - chord.rootPitchClass + 12) % 12;
}

bool third(int interval, const NormalizedChord& chord) noexcept
{
    if (! chord.hasTone(interval) || chord.degrees[static_cast<std::size_t>(interval)] == 9)
        return false;
    if (chord.degrees[static_cast<std::size_t>(interval)] == 3)
        return true;
    switch (chord.quality)
    {
        case ChordQuality::major:
        case ChordQuality::dominant:
        case ChordQuality::augmented: return interval == 4;
        case ChordQuality::minor:
        case ChordQuality::diminished:
        case ChordQuality::halfDiminished: return interval == 3;
        default: return false;
    }
}

bool seventh(int interval, const NormalizedChord& chord) noexcept
{
    return chord.hasTone(interval)
        && (chord.degrees[static_cast<std::size_t>(interval)] == 7
            || interval == 10 || interval == 11);
}

bool characteristic(int interval, const NormalizedChord& chord) noexcept
{
    return (chord.quality == ChordQuality::halfDiminished && interval == 6)
        || (chord.quality == ChordQuality::augmented && interval == 8)
        || (chord.quality == ChordQuality::suspended2 && interval == 2)
        || (chord.quality == ChordQuality::suspended4 && interval == 5)
        || (chord.hasAlteration(ChordAlteration::flatFifth) && interval == 6)
        || (chord.hasAlteration(ChordAlteration::sharpFifth) && interval == 8);
}

bool extendedHarmony(const NormalizedChord& chord) noexcept
{
    for (int interval = 0; interval < 12; ++interval)
        if (seventh(interval, chord))
            return true;
    return chord.hasExtension(ChordExtension::ninth)
        || chord.hasExtension(ChordExtension::eleventh)
        || chord.hasExtension(ChordExtension::thirteenth);
}

bool candidate(int note, const VoicingContext& context) noexcept
{
    const auto& chord = context.chord;
    if (note < 0 || note > 127 || ! chord.valid)
        return false;
    const auto interval = relative(note, chord);
    return chord.hasTone(interval)
        || (extendedHarmony(chord) && context.tension.valid
            && context.tension.isHarmonyCandidate(interval, context.tensionLevel));
}

bool containsRole(const std::array<int, 4>& notes,
                  const NormalizedChord& chord,
                  bool (*role)(int, const NormalizedChord&) noexcept) noexcept
{
    for (const auto note : notes)
        if (role(relative(note, chord), chord))
            return true;
    return false;
}

bool chordHasRole(const NormalizedChord& chord,
                  bool (*role)(int, const NormalizedChord&) noexcept) noexcept
{
    for (int interval = 0; interval < 12; ++interval)
        if (role(interval, chord))
            return true;
    return false;
}

int spacingPenalty(int distance, int preferredMin, int preferredMax) noexcept
{
    if (distance < preferredMin)
        return (preferredMin - distance) * 3;
    if (distance > preferredMax)
        return (distance - preferredMax) * 2;
    return 0;
}

int score(const std::array<int, 4>& notes, const VoicingContext& context) noexcept
{
    const auto& chord = context.chord;
    int value = spacingPenalty(notes[0] - notes[1], 3, 10)
        + spacingPenalty(notes[1] - notes[2], 3, 10)
        + spacingPenalty(notes[2] - notes[3], 5, 12)
        + spacingPenalty(notes[0] - notes[3], 17, 29);

    // Keep the structural guide tones and characteristic chord identity before
    // spending an inner voice on colour. The melody can itself supply a role.
    if (chordHasRole(chord, third) && ! containsRole(notes, chord, third))
        value += 40;
    if (chordHasRole(chord, seventh) && ! containsRole(notes, chord, seventh))
        value += 40;
    if (chordHasRole(chord, characteristic) && ! containsRole(notes, chord, characteristic))
        value += 48;

    for (int voice = 1; voice < 3; ++voice)
    {
        const auto interval = relative(notes[static_cast<std::size_t>(voice)], chord);
        if (interval == 0)
            value += 8; // root is already the bass anchor
        if (pitchClass(notes[static_cast<std::size_t>(voice)]) == pitchClass(notes[0]))
            value += 8;
        if (context.tension.valid && ! chord.hasTone(interval))
        {
            const auto& tone = context.tension.tone(interval);
            if (tone.role == TensionRole::preferred && tone.functionallyDirected
                && context.tension.resolutionConfirmed)
                value -= 7;
            else if (tone.role == TensionRole::contextual)
                value += 7;
            else if (tone.role == TensionRole::available)
                value += 3;
        }
        else if (chord.degrees[static_cast<std::size_t>(interval)] == 9
                 || chord.degrees[static_cast<std::size_t>(interval)] == 11
                 || chord.degrees[static_cast<std::size_t>(interval)] == 13)
        {
            value -= 2; // explicit chord colours remain eligible at Clean
        }
    }

    // Minor ninth is a strong soft negative. Explicit/directed colour is a
    // context-specific exception, following the Stage 4 candidate metadata.
    for (int upper = 0; upper < 3; ++upper)
    {
        for (int lower = upper + 1; lower < 4; ++lower)
        {
            const auto distance = notes[static_cast<std::size_t>(upper)]
                                - notes[static_cast<std::size_t>(lower)];
            if (distance < 13 || distance % 12 != 1)
                continue;
            bool exception = false;
            for (const auto index : { upper, lower })
            {
                if (index == 0) // performer melody never gets reclassified
                    continue;
                const auto interval = relative(notes[static_cast<std::size_t>(index)], chord);
                if (context.tension.valid)
                {
                    const auto& tone = context.tension.tone(interval);
                    exception = exception || tone.role == TensionRole::explicitTension
                        || (context.tensionLevel == TensionLevel::rich
                            && context.tension.resolutionConfirmed && tone.functionallyDirected);
                }
            }
            if (! exception)
                value += 14;
        }
    }
    return value;
}
}

VoiceOutput buildSpreadVoicing(int melodyNote, const VoicingContext& context) noexcept
{
    VoiceOutput result;
    result.clear();
    if (melodyNote < 0 || melodyNote > 127)
        return result;
    result.voices[0] = { true, melodyNote };
    if (! context.chord.valid)
        return result;

    // Keep a deliberate open anchor below V1, yet never change the melody or
    // manufacture notes outside MIDI. Slash bass is authoritative even if it
    // lies outside the Chord Track's ordinary chord-tone pool.
    const auto anchorClass = context.chord.slashBass
        ? context.chord.bassPitchClass : context.chord.rootPitchClass;
    int bass = -1;
    for (int note = melodyNote - 17; note >= 0 && note >= melodyNote - 29; --note)
    {
        if (pitchClass(note) == anchorClass)
        {
            bass = note;
            break;
        }
    }
    if (bass < 0)
        return result; // no safe structural spread in this register

    int best = INT_MAX;
    std::array<int, 4> chosen {};
    // Bottom-up: fix structural bass, place V3 above it, then V2 below the
    // performer melody. Fixed bounded search; no allocation/locks/audio I/O.
    for (int v3 = bass + 1; v3 < melodyNote - 1; ++v3)
    {
        if (! candidate(v3, context))
            continue;
        for (int v2 = v3 + 1; v2 < melodyNote; ++v2)
        {
            if (! candidate(v2, context))
                continue;
            const std::array<int, 4> notes { melodyNote, v2, v3, bass };
            const auto current = score(notes, context);
            if (current < best) // first ascending candidate wins deterministic ties
            {
                best = current;
                chosen = notes;
            }
        }
    }

    if (best == INT_MAX)
        return result;
    for (std::size_t voice = 1; voice < chosen.size(); ++voice)
        result.voices[voice] = { true, chosen[voice] };
    return result;
}
}

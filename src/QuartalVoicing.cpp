#include "QuartalVoicing.h"

#include <array>
#include <climits>

namespace smartvoicing::harmony
{
namespace
{
int pitchClass(int note) noexcept { return (note % 12 + 12) % 12; }
int relative(int note, const NormalizedChord& chord) noexcept
{
    return (pitchClass(note) - chord.rootPitchClass + 12) % 12;
}

bool candidate(int note, const VoicingContext& context) noexcept
{
    if (note < 0 || note > 127 || ! context.chord.valid)
        return false;
    const auto degree = relative(note, context.chord);
    bool extended = false;
    for (int i = 0; i < 12; ++i)
        extended = extended || (context.chord.hasTone(i)
            && context.chord.degrees[static_cast<std::size_t>(i)] == 7);
    extended = extended || context.chord.hasExtension(ChordExtension::ninth)
        || context.chord.hasExtension(ChordExtension::eleventh)
        || context.chord.hasExtension(ChordExtension::thirteenth);
    return context.chord.hasTone(degree)
        || (extended && context.tension.valid
            && context.tension.isHarmonyCandidate(degree, context.tensionLevel));
}

bool hasThird(const NormalizedChord& chord, int degree) noexcept
{
    return chord.hasTone(degree)
        && (chord.degrees[static_cast<std::size_t>(degree)] == 3);
}

bool hasSeventh(const NormalizedChord& chord, int degree) noexcept
{
    return chord.hasTone(degree)
        && (chord.degrees[static_cast<std::size_t>(degree)] == 7);
}

bool characteristic(const NormalizedChord& chord, int degree) noexcept
{
    return (chord.quality == ChordQuality::halfDiminished && degree == 6)
        || (chord.quality == ChordQuality::augmented && degree == 8)
        || (chord.quality == ChordQuality::suspended2 && degree == 2)
        || (chord.quality == ChordQuality::suspended4 && degree == 5)
        || (chord.hasAlteration(ChordAlteration::flatFifth) && degree == 6)
        || (chord.hasAlteration(ChordAlteration::sharpFifth) && degree == 8);
}

bool protectedAlteration(const NormalizedChord& chord, int degree) noexcept
{
    return (degree == 1 && chord.hasAlteration(ChordAlteration::flatNinth))
        || (degree == 3 && chord.hasAlteration(ChordAlteration::sharpNinth))
        || (degree == 6 && chord.hasAlteration(ChordAlteration::sharpEleventh))
        || (degree == 8 && chord.hasAlteration(ChordAlteration::flatThirteenth));
}

int intervalCost(int distance, bool top) noexcept
{
    if (distance == 5) return 0; // perfect fourth
    if (distance == 6) return 2; // augmented fourth when the pool permits it
    if (top && distance == 4) return 3; // contextual top major third
    if (distance == 4 || distance == 7) return 8;
    if (distance == 3 || distance == 8) return 12;
    if (distance == 2 || distance == 9) return 16;
    return 18 + (distance > 9 ? distance - 9 : 0);
}

int score(const std::array<int, 4>& notes, const VoicingContext& context) noexcept
{
    const auto& chord = context.chord;
    int value = 0;
    for (int i = 0; i < 3; ++i)
        value += intervalCost(notes[static_cast<std::size_t>(i)]
                                  - notes[static_cast<std::size_t>(i + 1)], i == 0);

    const int span = notes[0] - notes[3];
    if (span < 12) value += (12 - span) * 3;
    if (span > 27) value += (span - 27) * 3;

    bool chordThird = false, chordSeventh = false, retainedThird = false;
    bool retainedSeventh = false, chordCharacteristic = false, retainedCharacteristic = false;
    for (int degree = 0; degree < 12; ++degree)
    {
        chordThird = chordThird || hasThird(chord, degree);
        chordSeventh = chordSeventh || hasSeventh(chord, degree);
        chordCharacteristic = chordCharacteristic || characteristic(chord, degree);
    }
    for (const auto note : notes)
    {
        const auto degree = relative(note, chord);
        retainedThird = retainedThird || hasThird(chord, degree);
        retainedSeventh = retainedSeventh || hasSeventh(chord, degree);
        retainedCharacteristic = retainedCharacteristic || characteristic(chord, degree);
    }
    // Incomplete harmony is possible, but a fourth stack cannot obscure the
    // explicit chord identity or discard protected altered/characteristic tones.
    if (chordThird && ! retainedThird) value += 24;
    if (chordSeventh && ! retainedSeventh) value += 24;
    if (chordThird && chordSeventh && ! retainedThird && ! retainedSeventh) value += 40;
    if (chordCharacteristic && ! retainedCharacteristic) value += 60;
    for (int degree = 0; degree < 12; ++degree)
    {
        if (! protectedAlteration(chord, degree)) continue;
        bool retained = false;
        for (const auto note : notes)
            retained = retained || relative(note, chord) == degree;
        if (! retained) value += 60;
    }

    for (int i = 1; i < 4; ++i)
    {
        const auto degree = relative(notes[static_cast<std::size_t>(i)], chord);
        if (degree == 0) value += 2; // rootless is allowed, not required
        for (int j = 0; j < i; ++j)
            if (pitchClass(notes[static_cast<std::size_t>(i)])
                == pitchClass(notes[static_cast<std::size_t>(j)]))
                value += 7;
        if (context.tension.valid && ! chord.hasTone(degree))
        {
            const auto& tone = context.tension.tone(degree);
            if (tone.role == TensionRole::preferred && tone.functionallyDirected
                && context.tension.resolutionConfirmed)
                value -= 5;
            else if (tone.role == TensionRole::contextual)
                value += 5;
        }
    }
    for (int upper = 0; upper < 3; ++upper)
        for (int lower = upper + 1; lower < 4; ++lower)
        {
            const int distance = notes[static_cast<std::size_t>(upper)]
                               - notes[static_cast<std::size_t>(lower)];
            if (distance >= 13 && distance % 12 == 1)
            {
                bool exception = false;
                for (const int index : { upper, lower })
                {
                    if (index == 0 || ! context.tension.valid) continue;
                    const int degree = relative(notes[static_cast<std::size_t>(index)], chord);
                    const auto& tone = context.tension.tone(degree);
                    exception = exception || tone.role == TensionRole::explicitTension
                        || (context.tensionLevel == TensionLevel::rich
                            && context.tension.resolutionConfirmed
                            && tone.functionallyDirected);
                }
                if (! exception) value += 14; // strong soft penalty
            }
        }
    return value;
}
}

VoiceOutput buildQuartalVoicing(int melodyNote, const VoicingContext& context) noexcept
{
    VoiceOutput result;
    result.clear();
    if (melodyNote < 0 || melodyNote > 127)
        return result;
    result.voices[0] = { true, melodyNote };
    if (! context.chord.valid)
        return result;

    // The explicit slash bass owns V4. An ordinary quartal may be rootless.
    // Bound the search to a practical vertical; never fold out-of-range notes.
    const int floor = melodyNote > 30 ? melodyNote - 30 : 0;
    int best = INT_MAX;
    std::array<int, 4> chosen {};
    for (int v4 = floor; v4 < melodyNote - 2; ++v4)
    {
        if (context.chord.slashBass
            ? pitchClass(v4) != context.chord.bassPitchClass
            : ! candidate(v4, context))
            continue;
        for (int v3 = v4 + 1; v3 < melodyNote - 1; ++v3)
        {
            if (! candidate(v3, context)) continue;
            for (int v2 = v3 + 1; v2 < melodyNote; ++v2)
            {
                if (! candidate(v2, context)) continue;
                const std::array<int, 4> notes { melodyNote, v2, v3, v4 };
                const int cost = score(notes, context);
                if (cost < best)
                {
                    best = cost;
                    chosen = notes;
                }
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

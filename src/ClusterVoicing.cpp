#include "ClusterVoicing.h"

#include <array>
#include <climits>

namespace smartvoicing::harmony
{
namespace
{
int pc(int note) noexcept { return (note % 12 + 12) % 12; }
int degree(int note, const NormalizedChord& chord) noexcept
{
    return (pc(note) - chord.rootPitchClass + 12) % 12;
}
bool extended(const NormalizedChord& chord) noexcept
{
    for (int i = 0; i < 12; ++i)
        if (chord.hasTone(i) && chord.degrees[static_cast<std::size_t>(i)] == 7)
            return true;
    return chord.hasExtension(ChordExtension::ninth)
        || chord.hasExtension(ChordExtension::eleventh)
        || chord.hasExtension(ChordExtension::thirteenth);
}
bool guide(int d, const NormalizedChord& chord) noexcept
{
    return chord.hasTone(d) && (chord.degrees[static_cast<std::size_t>(d)] == 3
                                 || chord.degrees[static_cast<std::size_t>(d)] == 7);
}
bool protectedTone(int d, const NormalizedChord& chord) noexcept
{
    return (d == 6 && (chord.quality == ChordQuality::halfDiminished
                        || chord.hasAlteration(ChordAlteration::flatFifth)
                        || chord.hasAlteration(ChordAlteration::sharpEleventh)))
        || (d == 8 && (chord.quality == ChordQuality::augmented
                       || chord.hasAlteration(ChordAlteration::sharpFifth)
                       || chord.hasAlteration(ChordAlteration::flatThirteenth)))
        || (d == 1 && chord.hasAlteration(ChordAlteration::flatNinth))
        || (d == 3 && chord.hasAlteration(ChordAlteration::sharpNinth))
        || (d == 2 && chord.quality == ChordQuality::suspended2)
        || (d == 5 && chord.quality == ChordQuality::suspended4);
}
int intervalCost(int distance, bool top) noexcept
{
    if (distance == 1 || distance == 2) return 0;
    if (top && (distance == 3 || distance == 4)) return 3;
    if (distance == 3 || distance == 4) return 7;
    return 13 + (distance - 5) * 3;
}
int score(const std::array<int, 4>& notes, const VoicingContext& context) noexcept
{
    const auto& chord = context.chord;
    int cost = intervalCost(notes[0] - notes[1], true)
             + intervalCost(notes[1] - notes[2], false);
    // V4 may sit below the dense upper group as its structural support.
    const int gap = notes[2] - notes[3];
    if (gap < 3) cost += (3 - gap) * 3;
    if (gap > 13) cost += (gap - 13) * 3;
    const int span = notes[0] - notes[3];
    if (span > 23) cost += (span - 23) * 3;
    if (span < 7) cost += (7 - span) * 3;

    bool hasThird = false, hasSeventh = false, keptThird = false, keptSeventh = false;
    for (int d = 0; d < 12; ++d)
    {
        if (!chord.hasTone(d)) continue;
        if (chord.degrees[static_cast<std::size_t>(d)] == 3) hasThird = true;
        if (chord.degrees[static_cast<std::size_t>(d)] == 7) hasSeventh = true;
    }
    for (const auto note : notes)
    {
        const int d = degree(note, chord);
        if (guide(d, chord))
        {
            keptThird = keptThird || chord.degrees[static_cast<std::size_t>(d)] == 3;
            keptSeventh = keptSeventh || chord.degrees[static_cast<std::size_t>(d)] == 7;
        }
    }
    if (hasThird && !keptThird) cost += 25;
    if (hasSeventh && !keptSeventh) cost += 25;
    if (hasThird && hasSeventh && !keptThird && !keptSeventh) cost += 45;
    for (int d = 0; d < 12; ++d)
    {
        if (!protectedTone(d, chord)) continue;
        bool retained = false;
        for (const auto note : notes) retained = retained || degree(note, chord) == d;
        if (!retained) cost += 65;
    }
    const int support = degree(notes[3], chord);
    if (support != 0 && !guide(support, chord)) cost += 9;
    for (int i = 1; i < 4; ++i)
    {
        const int d = degree(notes[static_cast<std::size_t>(i)], chord);
        if (!chord.hasTone(d) && context.tension.valid)
        {
            const auto& tone = context.tension.tone(d);
            if (tone.role == TensionRole::contextual) cost += 4;
        }
        for (int j = 0; j < i; ++j)
            if (pc(notes[static_cast<std::size_t>(j)]) == pc(notes[static_cast<std::size_t>(i)]))
                cost += 9;
    }
    // A compound minor ninth remains a strong soft penalty, with Stage 4
    // explicit/directed exceptions. Small seconds are the intended texture.
    for (int upper = 0; upper < 3; ++upper)
        for (int lower = upper + 1; lower < 4; ++lower)
        {
            const int distance = notes[static_cast<std::size_t>(upper)]
                               - notes[static_cast<std::size_t>(lower)];
            if (distance < 13 || distance % 12 != 1) continue;
            bool exception = false;
            for (const int i : {upper, lower})
            {
                if (i == 0 || !context.tension.valid) continue;
                const auto& tone = context.tension.tone(
                    degree(notes[static_cast<std::size_t>(i)], chord));
                exception = exception || tone.role == TensionRole::explicitTension
                    || (context.tensionLevel == TensionLevel::rich
                        && context.tension.resolutionConfirmed && tone.functionallyDirected);
            }
            if (!exception) cost += 14;
        }
    return cost;
}
}

VoiceOutput buildClusterVoicing(int melodyNote, const VoicingContext& context) noexcept
{
    VoiceOutput result;
    result.clear();
    if (melodyNote < 0 || melodyNote > 127) return result;
    result.voices[0] = {true, melodyNote};
    if (!context.chord.valid) return result;

    std::array<bool, 12> legal {};
    const bool useTensions = extended(context.chord) && context.tension.valid;
    for (int pitch = 0; pitch < 12; ++pitch)
    {
        const int d = degree(pitch, context.chord);
        legal[static_cast<std::size_t>(pitch)] = context.chord.hasTone(d)
            || (useTensions && context.tension.isHarmonyCandidate(d, context.tensionLevel));
    }
    const int floor = melodyNote > 28 ? melodyNote - 28 : 0;
    int best = INT_MAX;
    std::array<int, 4> chosen {};
    for (int v2 = melodyNote - 1; v2 >= floor + 2; --v2)
    {
        if (!legal[static_cast<std::size_t>(pc(v2))]) continue;
        for (int v3 = v2 - 1; v3 >= floor + 1; --v3)
        {
            if (!legal[static_cast<std::size_t>(pc(v3))]) continue;
            for (int v4 = v3 - 1; v4 >= floor; --v4)
            {
                if (context.chord.slashBass)
                {
                    if (pc(v4) != context.chord.bassPitchClass) continue;
                }
                else if (!context.chord.hasTone(degree(v4, context.chord)))
                    continue;
                const std::array<int, 4> notes {melodyNote, v2, v3, v4};
                const int cost = score(notes, context);
                if (cost < best) { best = cost; chosen = notes; }
            }
        }
    }
    if (best == INT_MAX) return result;
    for (std::size_t i = 1; i < chosen.size(); ++i)
        result.voices[i] = {true, chosen[i]};
    return result;
}
}

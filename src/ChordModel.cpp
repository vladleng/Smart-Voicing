#include "ChordModel.h"

namespace smartvoicing::harmony
{
namespace
{
constexpr std::uint8_t genericUsedValue = 0xFFu;

bool explicitDegreeAt(const NormalizedChord& chord, int semitones, int degree) noexcept
{
    return semitones >= 0
        && semitones < kPitchClassCount
        && chord.degrees[static_cast<std::size_t>(semitones)] == degree;
}

void addExtension(NormalizedChord& chord, ChordExtension extension) noexcept
{
    chord.extensions |= flag(extension);
}

void addAlteration(NormalizedChord& chord, ChordAlteration alteration) noexcept
{
    chord.alterations |= flag(alteration);
}
}

bool NormalizedChord::hasTone(int semitones) const noexcept
{
    return semitones >= 0
        && semitones < kPitchClassCount
        && tones[static_cast<std::size_t>(semitones)];
}

bool NormalizedChord::hasDegree(int degree) const noexcept
{
    if (degree <= 0 || degree > 13)
        return false;

    for (const auto value : degrees)
        if (value == degree)
            return true;

    return false;
}

bool NormalizedChord::hasExtension(ChordExtension extension) const noexcept
{
    return (extensions & flag(extension)) != 0;
}

bool NormalizedChord::hasAlteration(ChordAlteration alteration) const noexcept
{
    return (alterations & flag(alteration)) != 0;
}

int circleOfFifthsToPitchClass(std::int32_t fifths) noexcept
{
    // ARA represents roots/basses as circle-of-fifths distance from C:
    // C=0, G=1, D=2, F=-1, Db=-5, C#=7, etc. Moving one fifth is
    // +7 semitones, then octave-wrap to the canonical pitch class 0..11.
    auto pitchClass = static_cast<int>((fifths * 7) % kPitchClassCount);
    if (pitchClass < 0)
        pitchClass += kPitchClassCount;
    return pitchClass;
}

NormalizedChord normalizeChord(const ChordContext& source) noexcept
{
    NormalizedChord result;
    result.rootFifths = source.root;
    result.bassFifths = source.bass;
    result.rootPitchClass = circleOfFifthsToPitchClass(source.root);
    result.bassPitchClass = circleOfFifthsToPitchClass(source.bass);
    result.slashBass = source.bass != source.root;

    if (! source.available || ! source.defined || ! source.intervals.any())
        return result;

    result.valid = true;

    for (int semitones = 0; semitones < kPitchClassCount; ++semitones)
    {
        const auto usage = source.intervals.values[static_cast<std::size_t>(semitones)];
        if (usage == 0)
            continue;

        result.tones[static_cast<std::size_t>(semitones)] = true;

        // ARA may either provide a generic "used" marker (0xFF) or encode the
        // diatonic degree directly (1..13). Keep the latter because it resolves
        // enharmonic harmonic functions such as b5 vs #11 and #5 vs b13.
        if (usage != genericUsedValue && usage >= 1 && usage <= 13)
            result.degrees[static_cast<std::size_t>(semitones)] = usage;
    }

    const auto minorThird = result.hasTone(3)
                         && ! explicitDegreeAt(result, 3, 9);
    const auto majorThird = result.hasTone(4);
    const auto perfectFifth = result.hasTone(7);

    const auto explicitFlatFifth = explicitDegreeAt(result, 6, 5);
    const auto explicitSharpEleventh = explicitDegreeAt(result, 6, 11);
    const auto structuralFlatFifth = result.hasTone(6)
                                  && ! perfectFifth
                                  && ! explicitSharpEleventh;

    const auto explicitSharpFifth = explicitDegreeAt(result, 8, 5);
    const auto explicitFlatThirteenth = explicitDegreeAt(result, 8, 13);
    const auto structuralSharpFifth = result.hasTone(8)
                                   && ! perfectFifth
                                   && majorThird
                                   && ! explicitFlatThirteenth;

    const auto minorSeventh = result.hasTone(10);
    const auto majorSeventh = result.hasTone(11);
    const auto diminishedSeventh = explicitDegreeAt(result, 9, 7);

    if (minorSeventh)
        addExtension(result, ChordExtension::minorSeventh);
    if (majorSeventh)
        addExtension(result, ChordExtension::majorSeventh);
    if (result.hasDegree(6))
        addExtension(result, ChordExtension::sixth);
    if (result.hasDegree(9))
        addExtension(result, ChordExtension::ninth);
    if (result.hasDegree(11))
        addExtension(result, ChordExtension::eleventh);
    if (result.hasDegree(13))
        addExtension(result, ChordExtension::thirteenth);

    if (explicitFlatFifth || structuralFlatFifth)
        addAlteration(result, ChordAlteration::flatFifth);
    if (explicitSharpFifth || structuralSharpFifth)
        addAlteration(result, ChordAlteration::sharpFifth);
    if (explicitDegreeAt(result, 1, 9))
        addAlteration(result, ChordAlteration::flatNinth);
    if (explicitDegreeAt(result, 3, 9))
        addAlteration(result, ChordAlteration::sharpNinth);
    if (explicitSharpEleventh)
        addAlteration(result, ChordAlteration::sharpEleventh);
    if (explicitFlatThirteenth)
        addAlteration(result, ChordAlteration::flatThirteenth);

    const auto flatFifth = result.hasAlteration(ChordAlteration::flatFifth);
    const auto sharpFifth = result.hasAlteration(ChordAlteration::sharpFifth);

    // Quality describes the stable harmonic family. Extensions/alterations are
    // orthogonal flags so C7, C9, C7b9 and C13 all remain "dominant".
    if (minorThird && flatFifth && minorSeventh)
        result.quality = ChordQuality::halfDiminished;
    else if (minorThird && flatFifth)
        result.quality = ChordQuality::diminished;
    else if (majorThird && minorSeventh)
        result.quality = ChordQuality::dominant;
    else if (majorThird && sharpFifth)
        result.quality = ChordQuality::augmented;
    else if (majorThird)
        result.quality = ChordQuality::major;
    else if (minorThird)
        result.quality = ChordQuality::minor;
    else if (result.hasTone(2) && perfectFifth)
        result.quality = ChordQuality::suspended2;
    else if (result.hasTone(5) && perfectFifth)
        result.quality = ChordQuality::suspended4;
    else if (perfectFifth)
        result.quality = ChordQuality::power;
    else if (diminishedSeventh || result.hasTone(6) || result.hasTone(8))
        result.quality = ChordQuality::noThird;
    else
        result.quality = ChordQuality::unknown;

    return result;
}

const char* chordQualityName(ChordQuality quality) noexcept
{
    switch (quality)
    {
        case ChordQuality::undefined:      return "undefined";
        case ChordQuality::major:          return "major";
        case ChordQuality::minor:          return "minor";
        case ChordQuality::dominant:       return "dominant";
        case ChordQuality::diminished:     return "diminished";
        case ChordQuality::halfDiminished: return "half-diminished";
        case ChordQuality::augmented:      return "augmented";
        case ChordQuality::suspended2:     return "sus2";
        case ChordQuality::suspended4:     return "sus4";
        case ChordQuality::power:          return "power";
        case ChordQuality::noThird:        return "no-third";
        case ChordQuality::unknown:        return "unknown";
    }

    return "unknown";
}
}

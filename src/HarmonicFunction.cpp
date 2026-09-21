#include "HarmonicFunction.h"

namespace smartvoicing::harmony
{
namespace
{
constexpr int wrap12(int value) noexcept
{
    value %= kPitchClassCount;
    return value < 0 ? value + kPitchClassCount : value;
}

HarmonicFunction functionForDegree(int degree) noexcept
{
    switch (degree)
    {
        case 1:
        case 3:
        case 6:
            return HarmonicFunction::tonic;

        case 2:
        case 4:
            return HarmonicFunction::predominant;

        case 5:
        case 7:
            return HarmonicFunction::dominant;

        default:
            return HarmonicFunction::other;
    }
}

bool allChordTonesBelongToKey(const NormalizedChord& chord,
                              const NormalizedKey& key) noexcept
{
    if (! chord.valid || ! key.valid)
        return false;

    bool sawTone = false;
    for (int relative = 0; relative < kPitchClassCount; ++relative)
    {
        if (! chord.tones[static_cast<std::size_t>(relative)])
            continue;

        sawTone = true;
        const auto absolute = wrap12(chord.rootPitchClass + relative);
        if (! key.hasPitchClass(absolute))
            return false;
    }

    return sawTone;
}
}

HarmonicAnalysis analyzeHarmonicFunction(const NormalizedChord& chord,
                                         const NormalizedKey& key) noexcept
{
    HarmonicAnalysis result;
    if (! chord.valid || ! key.valid)
        return result;

    result.valid = true;
    result.rootScaleDegree = scaleDegreeForPitchClass(key, chord.rootPitchClass);
    result.rootFunction = functionForDegree(result.rootScaleDegree);
    result.effectiveFunction = result.rootFunction;
    result.chordTonesDiatonic = allChordTonesBelongToKey(chord, key);
    result.relation = result.chordTonesDiatonic
        ? HarmonicRelation::diatonic
        : HarmonicRelation::chromatic;

    // A dominant seventh resolves down a fifth / up a fourth: target = root + 5 semitones.
    // If that target belongs to the active Key, expose it as an applied-dominant candidate.
    // Primary V -> I is not labelled as "applied"; it remains the ordinary dominant.
    if (chord.quality == ChordQuality::dominant)
    {
        const auto targetPitchClass = wrap12(chord.rootPitchClass + 5);
        const auto targetDegree = scaleDegreeForPitchClass(key, targetPitchClass);
        const auto isPrimaryDominant = result.rootScaleDegree == 5 && targetDegree == 1;

        if (targetDegree > 0 && ! isPrimaryDominant)
        {
            result.appliedDominantCandidate = true;
            result.appliedTargetPitchClass = targetPitchClass;
            result.appliedTargetScaleDegree = targetDegree;
            result.effectiveFunction = HarmonicFunction::dominant;
        }
    }

    return result;
}

const char* harmonicFunctionName(HarmonicFunction function) noexcept
{
    switch (function)
    {
        case HarmonicFunction::tonic: return "Tonic";
        case HarmonicFunction::predominant: return "Predominant";
        case HarmonicFunction::dominant: return "Dominant";
        case HarmonicFunction::other: return "Other";
        case HarmonicFunction::undefined:
        default: return "Undefined";
    }
}

const char* harmonicRelationName(HarmonicRelation relation) noexcept
{
    switch (relation)
    {
        case HarmonicRelation::diatonic: return "Diatonic";
        case HarmonicRelation::chromatic: return "Chromatic";
        case HarmonicRelation::undefined:
        default: return "Undefined";
    }
}

const char* scaleDegreeName(int degree) noexcept
{
    switch (degree)
    {
        case 1: return "I";
        case 2: return "II";
        case 3: return "III";
        case 4: return "IV";
        case 5: return "V";
        case 6: return "VI";
        case 7: return "VII";
        default: return "-";
    }
}
}

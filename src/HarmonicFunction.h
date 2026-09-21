#pragma once

#include "ChordModel.h"
#include "KeyModel.h"

#include <cstdint>

namespace smartvoicing::harmony
{
enum class HarmonicFunction : std::uint8_t
{
    undefined = 0,
    tonic,
    predominant,
    dominant,
    other
};

enum class HarmonicRelation : std::uint8_t
{
    undefined = 0,
    diatonic,
    chromatic
};

struct HarmonicAnalysis
{
    bool valid = false;

    int rootScaleDegree = 0;
    HarmonicFunction rootFunction = HarmonicFunction::undefined;
    HarmonicFunction effectiveFunction = HarmonicFunction::undefined;
    HarmonicRelation relation = HarmonicRelation::undefined;
    bool chordTonesDiatonic = false;

    // 0.3a deliberately calls this a candidate. From current Chord + Key alone
    // D7 in C can strongly suggest V/V, but a resolution-aware stage is needed
    // before claiming that every dominant-form chord actually functions that way.
    bool appliedDominantCandidate = false;
    int appliedTargetPitchClass = -1;
    int appliedTargetScaleDegree = 0;
};

HarmonicAnalysis analyzeHarmonicFunction(const NormalizedChord& chord,
                                         const NormalizedKey& key) noexcept;

const char* harmonicFunctionName(HarmonicFunction function) noexcept;
const char* harmonicRelationName(HarmonicRelation relation) noexcept;
const char* scaleDegreeName(int degree) noexcept;
}

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

    // Dominant-form chord interpreted from the current Chord + Key.
    // Candidate remains true even when the following chord is unavailable or
    // does not confirm the expected target; confirmation is separate evidence.
    bool appliedDominantCandidate = false;
    int appliedTargetPitchClass = -1;
    int appliedTargetScaleDegree = 0;

    // 0.3c timeline evidence. A valid next chord can confirm the predicted
    // applied-dominant target without turning a non-confirming next chord into
    // an automatic rejection: delayed/deceptive resolutions remain possible.
    bool nextChordAvailable = false;
    int nextChordRootPitchClass = -1;
    ChordQuality nextChordQuality = ChordQuality::undefined;
    bool appliedDominantConfirmed = false;

    // 0.3e generic dominant-resolution evidence. Unlike
    // appliedDominantConfirmed this also covers the primary V -> I case. The
    // target quality is preserved because dominant colour depends on whether the
    // actual resolution target is major-like or minor.
    bool dominantResolutionConfirmed = false;
    int dominantTargetPitchClass = -1;
    ChordQuality dominantTargetQuality = ChordQuality::undefined;

    // Parallel-mode borrowing MVP. This is deliberately a candidate layer:
    // if a chromatic chord's complete pitch set belongs to the parallel major
    // or minor collection, expose that as modal-interchange evidence without
    // rewriting the explicit Chord Track.
    bool modalInterchangeCandidate = false;
    KeyMode modalInterchangeSource = KeyMode::undefined;
};

// 0.3a-compatible static analysis from the currently active Chord + Key only.
HarmonicAnalysis analyzeHarmonicFunction(const NormalizedChord& chord,
                                         const NormalizedKey& key) noexcept;

// 0.3c/0.3e resolution-aware overload. The following Chord Track event is
// evidence used both to confirm applied/secondary dominant candidates and to
// expose the actual target quality for functional tension profiles.
HarmonicAnalysis analyzeHarmonicFunction(const NormalizedChord& chord,
                                         const NormalizedKey& key,
                                         const NormalizedChord& nextChord) noexcept;

const char* harmonicFunctionName(HarmonicFunction function) noexcept;
const char* harmonicRelationName(HarmonicRelation relation) noexcept;
const char* scaleDegreeName(int degree) noexcept;
}

#pragma once

#include "ChordModel.h"
#include "HarmonicContext.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"

namespace smartvoicing::harmony
{
// Context available to the 0.3b Closed Voicing engine. The musical priority is
// deliberately one-way: Melody > explicit Chord > Key / Function > voicing
// preferences. Key/function may influence ranking, but never rewrite the chord
// or the performer-owned melody.
struct ClosedVoicingContext
{
    NormalizedKey key;
    HarmonicAnalysis harmonic;
};

// Smart Voicing 0.3b Closed Voicing MVP:
// - V1 is always the played melody note, including non-chord / tension notes;
// - V2..V4 are selected as one compact vertical candidate below the melody;
// - guide tones / characteristic chord tones outrank mechanical chord stacking;
// - root and fifth may be omitted when harmonic identity remains clear;
// - V1-V2 spacing uses a soft preference (3rd preferred, 4th common), never a
//   hard interval rule;
// - explicit slash bass remains authoritative in V4 when feasible;
// - no valid chord => V1-only fallback;
// - no dynamic allocation, file I/O or locking is used in the realtime path.
VoiceOutput buildClosedVoicing(int melodyNote,
                               const NormalizedChord& chord,
                               const ClosedVoicingContext& context) noexcept;

// Compatibility entry point used by existing Stage 3 tests / code paths while
// 0.3b integration is being completed. It uses the same Closed engine with no
// Key/Function preference information.
VoiceOutput buildCloseVoicing(int melodyNote, const NormalizedChord& chord) noexcept;
}

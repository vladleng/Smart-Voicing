#pragma once

#include "ChordModel.h"
#include "HarmonicContext.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"
#include "TensionPolicy.h"

namespace smartvoicing::harmony
{
// Context available to the Closed Voicing engine. The musical priority remains
// deliberately one-way: Melody > explicit Chord > Key / Function > Tension
// Policy > voicing preferences. Key/function/policy may influence ranking, but
// never rewrite the chord or the performer-owned melody.
struct ClosedVoicingContext
{
    NormalizedKey key;
    HarmonicAnalysis harmonic;
    TensionPolicy tension;
};

// Smart Voicing Closed Voicing engine:
// - V1 is always the played melody note, including non-chord / tension notes;
// - V2..V4 are selected as one compact vertical candidate below the melody;
// - guide tones / characteristic chord tones outrank mechanical chord stacking;
// - root and fifth may be omitted when harmonic identity remains clear;
// - 0.3d may use inferred Preferred / Available / Contextual tensions in V2..V4
//   when the chord family is rich enough, while Avoid-as-harmony stays excluded;
// - explicit Chord Track tensions remain authoritative candidates;
// - V1-V2 spacing uses a soft preference (3rd preferred, 4th common), never a
//   hard interval rule;
// - explicit slash bass remains authoritative in V4 when feasible;
// - no valid chord => V1-only fallback;
// - no dynamic allocation, file I/O or locking is used in the realtime path.
VoiceOutput buildClosedVoicing(int melodyNote,
                               const NormalizedChord& chord,
                               const ClosedVoicingContext& context) noexcept;

// Compatibility entry point used by older Stage 3 tests / code paths. It uses
// the same Closed engine with no Key/Function information; therefore no inferred
// tensions are invented and only explicit Chord Track material is available.
VoiceOutput buildCloseVoicing(int melodyNote, const NormalizedChord& chord) noexcept;
}

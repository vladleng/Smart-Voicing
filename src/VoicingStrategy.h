#pragma once

#include "CloseVoicingHarmonizer.h"

#include <cstdint>

namespace smartvoicing::harmony
{
// Public Stage 5 selector.
//
// Harmonic strategies (Closed / Drop family) organize already interpreted
// Stage 4 harmonic material. Melodic orchestration strategies such as Unison,
// Octaves and Doubling organize performer-owned melody directly and do not
// invent harmony merely to make Tension Level audible.
enum class VoicingType : std::uint8_t
{
    closed = 0,
    drop2 = 1,
    unison = 2,
    octaves = 3,
    doubling = 4,
    // Appended in 0.4d. Existing 0..4 numeric meanings are intentionally
    // frozen for saved-project compatibility.
    drop3 = 5
};

// Stage 4 hands Stage 5 an already interpreted harmonic context. A strategy
// may organize this material vertically, but must not re-run harmonic-function
// or tension-vocabulary discovery on its own.
struct VoicingContext
{
    NormalizedChord chord;
    NormalizedKey key;
    HarmonicAnalysis harmonic;
    TensionPolicy tension;
    TensionLevel tensionLevel = TensionLevel::clean;
};

// Pure Drop 2 family transform used by the dispatcher and regression tests.
// The second voice from the selected Closed vertical is lowered one octave,
// then V2..V4 are assigned in sounding top-down order. V1/melody is unchanged.
// If a complete four-voice transform is not safe, or an explicit slash bass
// would cease to be the lowest authoritative voice, the accepted Closed
// vertical is returned unchanged rather than violating a higher-priority rule.
VoiceOutput transformClosedToDrop2(const VoiceOutput& closed,
                                   const NormalizedChord& chord) noexcept;

// 0.4d Drop 3 family transform. The third voice from the selected Closed
// vertical is lowered one octave, then V2..V4 are assigned in sounding
// top-down order. V1/melody and the exact Closed pitch-class material remain
// unchanged. Slash-bass / incomplete / underflow cases use the same safe
// Closed fallback policy as Drop 2 for this MVP.
VoiceOutput transformClosedToDrop3(const VoiceOutput& closed,
                                   const NormalizedChord& chord) noexcept;

// 0.4c1 melodic-section texture: every output voice receives exactly the
// performer-owned melody pitch. The voices remain independent output slots / MIDI
// channels even when their note number is identical. No chord/function/tension
// material is generated in this strategy.
VoiceOutput buildUnisonVoicing(int melodyNote) noexcept;

// 0.4c2 octave-section texture. Default Stage 5 layout is explicitly:
// V1 = melody, V2 = melody-12, V3 = melody-12, V4 = melody-24.
// This is a project-defined orchestration layout informed by common section
// octave-doubling practice; instrument-specific comfortable ranges remain Stage 7.
// Any target below the MIDI domain is left inactive rather than wrapping or
// changing the performer-owned V1 melody.
VoiceOutput buildOctaveVoicing(int melodyNote) noexcept;

// 0.4c3 paired doubling texture. The first pair doubles the melody in unison,
// while the second pair doubles the same pitch class one octave below:
// V1 = melody, V2 = melody, V3 = melody-12, V4 = melody-12.
// This remains a deterministic Stage 5 melodic-layout decision; instrument
// register adaptation is intentionally deferred to Stage 7.
VoiceOutput buildDoublingVoicing(int melodyNote) noexcept;

// Stage 5 dispatcher. Drop-family strategies transform the exact Closed material;
// melodic-section strategies may intentionally bypass harmonic generation.
VoiceOutput buildVoicing(int melodyNote,
                         VoicingType type,
                         const VoicingContext& context) noexcept;

const char* voicingTypeName(VoicingType type) noexcept;
}

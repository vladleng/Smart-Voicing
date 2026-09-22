#pragma once

#include "CloseVoicingHarmonizer.h"

#include <cstdint>

namespace smartvoicing::harmony
{
// Public Stage 5 selector. Closed remains the source material for the Drop
// family: Drop strategies transform an already selected Closed vertical and
// must not re-run harmonic-function or tension-vocabulary discovery.
enum class VoicingType : std::uint8_t
{
    closed = 0,
    drop2 = 1
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

// Stage 5 dispatcher. 0.4b adds Drop 2 as a transformation of the exact Closed
// material selected from the Stage 4 context.
VoiceOutput buildVoicing(int melodyNote,
                         VoicingType type,
                         const VoicingContext& context) noexcept;

const char* voicingTypeName(VoicingType type) noexcept;
}

#pragma once

#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"

#include <array>
#include <cstdint>

namespace smartvoicing::harmony
{
// 0.3d keeps melodic authority separate from harmonic suitability. A pitch can
// be avoid-as-harmony or even outside the inferred pool and still remain a
// valid performer-owned melody note.
enum class TensionRole : std::uint8_t
{
    unavailable = 0,
    chordTone,
    explicitTension,
    preferred,
    available,
    contextual,
    avoidAsHarmony
};

struct TensionTonePolicy
{
    TensionRole role = TensionRole::unavailable;
    bool melodyImposed = false;
    bool explicitFromChord = false;
    bool fromActiveKey = false;
    bool fromFunctionScale = false;
};

struct TensionPolicy
{
    bool valid = false;
    std::array<TensionTonePolicy, kPitchClassCount> tones {};

    const TensionTonePolicy& tone(int relativeSemitones) const noexcept;
    bool isHarmonyCandidate(int relativeSemitones) const noexcept;
};

// Build a conservative, allocation-free harmonic candidate policy.
// Priority is one-way:
//   performer melody > explicit Chord Track > Key/Mode + Function inference.
// Explicit 9/11/13/alterations are never downgraded by inferred avoid rules.
TensionPolicy buildTensionPolicy(const NormalizedChord& chord,
                                 const NormalizedKey& key,
                                 const HarmonicAnalysis& harmonic,
                                 int melodyMidiNote = -1) noexcept;

const char* tensionRoleName(TensionRole role) noexcept;
}

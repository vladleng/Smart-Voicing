#pragma once

#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"

#include <array>
#include <cstdint>

namespace smartvoicing::harmony
{
// User-facing degree of harmonic colour. This is intentionally independent
// from TensionRole: role describes what a pitch means in the current context,
// while level controls how freely inferred colour may enter generated voices.
enum class TensionLevel : std::uint8_t
{
    clean = 1,
    color = 2,
    rich = 3
};

// 0.3e adds target-aware functional profiles. The profile is not a voicing type
// and does not rewrite the explicit chord; it only changes how inferred colour
// is classified and ranked.
enum class FunctionalTensionProfile : std::uint8_t
{
    neutral = 0,
    dominantUnresolved,
    dominantMajorTarget,
    dominantMinorTarget
};

// Melodic authority remains separate from harmonic suitability. A pitch can be
// avoid-as-harmony or even outside the inferred pool and still remain a valid
// performer-owned melody note.
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
    bool alteredCandidate = false;

    // True when the pitch is not merely allowed, but is specifically supported
    // by the current function + target relationship. Stage 6 will later use the
    // same evidence together with previous Voice state / tendency resolution.
    bool functionallyDirected = false;
};

struct TensionPolicy
{
    bool valid = false;
    FunctionalTensionProfile functionalProfile = FunctionalTensionProfile::neutral;
    bool resolutionConfirmed = false;
    std::array<TensionTonePolicy, kPitchClassCount> tones {};

    const TensionTonePolicy& tone(int relativeSemitones) const noexcept;

    // Tension Level is a gate/weighting policy, not a fixed list of extensions.
    // Explicit Chord Track material remains available at every level.
    bool isHarmonyCandidate(int relativeSemitones, TensionLevel level) const noexcept;

    // Compatibility with the first 0.3d foundation implementation. Rich is the
    // closest equivalent to the previous broad candidate pool.
    bool isHarmonyCandidate(int relativeSemitones) const noexcept
    {
        return isHarmonyCandidate(relativeSemitones, TensionLevel::rich);
    }
};

// Build a conservative, allocation-free harmonic candidate policy.
// Priority is one-way:
//   performer melody > explicit Chord Track > Key/Mode + Function/Target inference.
// Explicit 9/11/13/alterations are never downgraded by inferred avoid rules.
TensionPolicy buildTensionPolicy(const NormalizedChord& chord,
                                 const NormalizedKey& key,
                                 const HarmonicAnalysis& harmonic,
                                 int melodyMidiNote = -1) noexcept;

const char* tensionRoleName(TensionRole role) noexcept;
const char* tensionLevelName(TensionLevel level) noexcept;
const char* functionalTensionProfileName(FunctionalTensionProfile profile) noexcept;
}

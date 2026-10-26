#pragma once
#include "HarmonicContext.h"
#include <array>

namespace smartvoicing::harmony
{
// Realtime-owned musical continuity state; not Router ownership or saved state.
class VoiceLeadingState
{
public:
    void reset() noexcept;
    void commit(const VoiceOutput& sounding) noexcept;
    bool hasPrevious() const noexcept { return available; }
    const VoiceOutput& previous() const noexcept { return last; }
private:
    VoiceOutput last {};
    bool available = false;
};

struct VoiceLeadingScore
{
    int cost = 0;
    int commonTones = 0;
    int stepwiseMoves = 0;
    int largeLeaps = 0;
    int crossings = 0;
    bool valid = true;
};

// Call only after harmonic legality and VoicingStrategy constraints are met.
// Does not rewrite V1 or any notes. Fixed-size, allocation-free, deterministic.
VoiceLeadingScore evaluateVoiceLeading(const VoiceOutput& candidate,
                                      const VoiceLeadingState& state) noexcept;

// Small bounded additive tie-break (0..10) for already legal Stage 5 verticals.
// A zero score means same V2..V4; no history preserves exact 0.5 behavior.
// 0.5a deliberately makes the musical vertical score dominant over smoothness.
int voiceLeadingAdjustment(const VoiceOutput& candidate,
                           const VoiceLeadingState* state) noexcept;
int voiceLeadingAdjustment(const std::array<int, kVoiceCount>& notes,
                           const VoiceLeadingState* state) noexcept;

// 0.5b: unsaturated motion rank, used only inside a harmonic safety window.
// Differentiates distinct leaps; Drop uses softer octave-displacement weight.
int voiceLeadingMotionRank(const VoiceOutput& candidate,
                           const VoiceLeadingState* state,
                           bool dropFamily) noexcept;
}

#pragma once
#include "HarmonicContext.h"

namespace smartvoicing::harmony
{
// Realtime-owned musical continuity state; not Router ownership or saved state.
class VoiceLeadingState
{
public:
    void reset() noexcept;
    void commit(const VoiceOutput& sounding) noexcept;
    bool hasPrevious() const noexcept { return valid; }
    const VoiceOutput& previous() const noexcept { return last; }
private:
    VoiceOutput last {};
    bool valid = false;
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
}

#pragma once

#include "HarmonicContext.h"

#include <array>

namespace smartvoicing::harmony
{
struct VoiceTransition
{
    bool noteOff = false;
    int oldNote = -1;
    bool noteOn = false;
    int newNote = -1;
};

struct ReharmonizationPlan
{
    std::array<VoiceTransition, kVoiceCount> voices {};
    bool lowerVoicesChanged = false;
};

// Build a transition from the currently sounding Melody Harmonize voicing to
// the desired one. V1 is deliberately immutable here: the played melody owns
// V1, while only generated V2..V4 may be replaced by a Chord Track change.
ReharmonizationPlan planLowerVoiceReharmonization(const VoiceOutput& current,
                                                  const VoiceOutput& desired) noexcept;
}

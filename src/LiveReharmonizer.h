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

// Timeline helpers for scheduling a known future Chord Track boundary inside
// the current audio block. Values outside the block return -1 instead of being
// clamped, so the next block owns that event. They do not add lookahead/latency.
int sampleOffsetFromTimelineSeconds(double blockStartSeconds,
                                    double eventSeconds,
                                    double sampleRate,
                                    int blockSamples) noexcept;

int sampleOffsetFromPpq(double blockStartPpq,
                        double eventPpq,
                        double bpm,
                        double sampleRate,
                        int blockSamples) noexcept;
}

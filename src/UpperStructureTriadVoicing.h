#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// Independent Stage 5 strategy: a major/minor triad in the upper voices
// above a structural chord-tone support voice. V1 remains performer-owned.
VoiceOutput buildUpperStructureTriadVoicing(int melodyNote,
                                            const VoicingContext& context) noexcept;
}

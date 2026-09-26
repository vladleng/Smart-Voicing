#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// Dense second-based vertical drawn from the existing Stage 4 vocabulary.
// The performed melody owns V1; an explicit slash bass owns V4.
VoiceOutput buildClusterVoicing(int melodyNote, const VoicingContext& context) noexcept;
}

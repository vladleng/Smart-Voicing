#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// Independent Stage 5 vertical objective. V1 is the played melody; the lower
// voices are selected from the existing Stage 4 candidate vocabulary with a
// preference for fourth structures. No previous-voice state is consulted.
VoiceOutput buildQuartalVoicing(int melodyNote, const VoicingContext& context) noexcept;
}

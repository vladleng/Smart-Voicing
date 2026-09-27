#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// Independent bottom-up Stage 5 strategy. The bass is a root (or explicit
// slash-bass) anchor; V3/V2 are selected from the already interpreted Stage 4
// candidate pool. V1 is always the untouched performer melody. This is not a
// transformation of a Closed vertical and has no previous-voice state.
VoiceOutput buildSpreadVoicing(int melodyNote,
                               const VoicingContext& context) noexcept;
}

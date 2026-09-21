#pragma once

#include "ChordModel.h"
#include "HarmonicContext.h"

namespace smartvoicing::harmony
{
// Smart Voicing 0.2c MVP:
// - V1 is always the played melody note;
// - V2..V4 are the closest chord tones below it;
// - distinct chord pitch classes are preferred before doubling;
// - slash-bass is respected in V4 when it can be placed below V3;
// - if no valid chord exists, only V1 is returned.
VoiceOutput buildCloseVoicing(int melodyNote, const NormalizedChord& chord) noexcept;
}

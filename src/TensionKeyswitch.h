#pragma once

#include "TensionPolicy.h"

namespace smartvoicing::harmony
{
// 0.4a realtime selector for the existing shared TensionLevel state.
//
// Studio Pro labels middle C (MIDI 60) as C3, therefore the agreed physical
// keys G1 / G#1 / A1 correspond to MIDI notes 43 / 44 / 45. Other DAWs may
// display different octave numbers for the same MIDI notes; the note numbers
// are the stable contract.
constexpr int kCleanTensionKeyswitchNote = 43;
constexpr int kColorTensionKeyswitchNote = 44;
constexpr int kRichTensionKeyswitchNote = 45;

bool isTensionLevelKeyswitch(int midiNote) noexcept;

// Returns true when midiNote is one of the three tension keyswitches and writes
// the requested value to level. This function does not create a second state:
// callers must apply the result to the same TensionLevel used by UI/project
// state. Both note-on and note-off swallowing is handled by the MIDI processor.
bool tensionLevelFromKeyswitch(int midiNote, TensionLevel& level) noexcept;
}

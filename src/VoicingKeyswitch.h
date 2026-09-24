#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// 0.4c4 realtime selector for the existing shared VoicingType state.
//
// Stable contract uses canonical MIDI note numbers, never DAW-specific octave
// labels. The whole 32..42 block is reserved in Melody Harmonize so future
// voicing families can be added without moving already learned keyswitches.
//
// Harmonic / modern family:
//   32 Closed
//   33 Drop 2
//   34 Drop 3      (reserved)
//   35 Drop 2+4    (reserved)
//   36 Spread      (reserved)
//   37 Quartal     (reserved)
//   38 Cluster     (reserved)
//   39 UST         (reserved)
// Melodic textures:
//   40 Unison
//   41 Octaves
//   42 Doubling
// Existing Tension keyswitches remain immediately above this block: 43/44/45.
constexpr int kFirstVoicingKeyswitchNote = 32;
constexpr int kClosedVoicingKeyswitchNote = 32;
constexpr int kDrop2VoicingKeyswitchNote = 33;
constexpr int kDrop3ReservedVoicingKeyswitchNote = 34;
constexpr int kDrop24ReservedVoicingKeyswitchNote = 35;
constexpr int kSpreadReservedVoicingKeyswitchNote = 36;
constexpr int kQuartalReservedVoicingKeyswitchNote = 37;
constexpr int kClusterReservedVoicingKeyswitchNote = 38;
constexpr int kUstReservedVoicingKeyswitchNote = 39;
constexpr int kUnisonVoicingKeyswitchNote = 40;
constexpr int kOctavesVoicingKeyswitchNote = 41;
constexpr int kDoublingVoicingKeyswitchNote = 42;
constexpr int kLastVoicingKeyswitchNote = 42;

// Returns true for the entire reserved 32..42 control block. In Melody
// Harmonize both note-on and note-off in this block are swallowed, including
// not-yet-implemented reserved slots.
bool isVoicingTypeKeyswitch(int midiNote) noexcept;

// Returns true only for currently implemented Voicing Types and writes the
// requested value to type. Reserved future notes return false while remaining
// part of the swallowed control block.
bool voicingTypeFromKeyswitch(int midiNote, VoicingType& type) noexcept;

bool isReservedVoicingTypeKeyswitch(int midiNote) noexcept;
}

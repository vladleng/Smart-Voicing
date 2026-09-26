#pragma once

#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
// 0.4f realtime selector for the shared VoicingType state.
//
// Stable contract uses canonical MIDI note numbers. Studio Pro octave labels
// below are convenience labels for the current host convention only.
// The whole 32..42 block is reserved in Melody Harmonize so future voicing
// families can be added without colliding with the stable Tension block 43..45.
//
// Lower reserved modern-family slots:
//   32 G#0 UST         (reserved)
//   33 A0  Cluster     (reserved)
//   34 A#0 Quartal     (reserved)
//   35 B0  Spread
// Frequently used block starts at C1:
//   36 C1  Closed
//   37 C#1 Drop 2
//   38 D1  Drop 3
//   39 D#1 Drop 2+4
//   40 E1  Unison
//   41 F1  Octaves
//   42 F#1 Doubling
// Existing Tension keyswitches remain unchanged: 43/44/45 = G1/G#1/A1.
constexpr int kFirstVoicingKeyswitchNote = 32;
constexpr int kUstReservedVoicingKeyswitchNote = 32;
constexpr int kClusterReservedVoicingKeyswitchNote = 33;
constexpr int kQuartalReservedVoicingKeyswitchNote = 34;
constexpr int kSpreadVoicingKeyswitchNote = 35;
constexpr int kClosedVoicingKeyswitchNote = 36;
constexpr int kDrop2VoicingKeyswitchNote = 37;
constexpr int kDrop3VoicingKeyswitchNote = 38;
constexpr int kDrop24VoicingKeyswitchNote = 39;
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

#pragma once

namespace smartvoicing::harmony
{
// 0.4c4 fix2: global Harmony Mode keyswitches.
//
// These two controls are intentionally above the existing Tension block and
// are active in both Direct Router and Melody Harmonize so the user can always
// switch back and forth from the same keyboard / Sound Variations map.
// Canonical MIDI note numbers are authoritative; DAW octave labels are only UI.
constexpr int kDirectRouterModeKeyswitchNote = 46;
constexpr int kMelodyHarmonizeModeKeyswitchNote = 47;
constexpr int kFirstHarmonyModeKeyswitchNote = kDirectRouterModeKeyswitchNote;
constexpr int kLastHarmonyModeKeyswitchNote = kMelodyHarmonizeModeKeyswitchNote;

bool isHarmonyModeKeyswitch(int midiNote) noexcept;

// Decoder returns the existing processor HarmonyMode numeric contract:
// 0 = Direct Router, 1 = Melody Harmonize. Invalid notes return false without
// mutating modeValue.
bool harmonyModeValueFromKeyswitch(int midiNote, int& modeValue) noexcept;
}

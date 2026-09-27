#include "HarmonyModeKeyswitch.h"

namespace smartvoicing::harmony
{
bool isHarmonyModeKeyswitch(int midiNote) noexcept
{
    return midiNote >= kFirstHarmonyModeKeyswitchNote
        && midiNote <= kLastHarmonyModeKeyswitchNote;
}

bool harmonyModeValueFromKeyswitch(int midiNote, int& modeValue) noexcept
{
    switch (midiNote)
    {
        case kDirectRouterModeKeyswitchNote:
            modeValue = 0;
            return true;
        case kMelodyHarmonizeModeKeyswitchNote:
            modeValue = 1;
            return true;
        default:
            return false;
    }
}
}

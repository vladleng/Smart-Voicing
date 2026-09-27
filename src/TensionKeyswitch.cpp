#include "TensionKeyswitch.h"

namespace smartvoicing::harmony
{
bool isTensionLevelKeyswitch(int midiNote) noexcept
{
    return midiNote == kCleanTensionKeyswitchNote
        || midiNote == kColorTensionKeyswitchNote
        || midiNote == kRichTensionKeyswitchNote;
}

bool tensionLevelFromKeyswitch(int midiNote, TensionLevel& level) noexcept
{
    switch (midiNote)
    {
        case kCleanTensionKeyswitchNote:
            level = TensionLevel::clean;
            return true;
        case kColorTensionKeyswitchNote:
            level = TensionLevel::color;
            return true;
        case kRichTensionKeyswitchNote:
            level = TensionLevel::rich;
            return true;
        default:
            return false;
    }
}
}

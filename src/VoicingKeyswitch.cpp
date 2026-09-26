#include "VoicingKeyswitch.h"

namespace smartvoicing::harmony
{
bool isVoicingTypeKeyswitch(int midiNote) noexcept
{
    return midiNote >= kFirstVoicingKeyswitchNote
        && midiNote <= kLastVoicingKeyswitchNote;
}

bool voicingTypeFromKeyswitch(int midiNote, VoicingType& type) noexcept
{
    switch (midiNote)
    {
        case kClusterVoicingKeyswitchNote:
            type = VoicingType::cluster;
            return true;
        case kUstVoicingKeyswitchNote:
            type = VoicingType::ust;
            return true;
        case kQuartalVoicingKeyswitchNote:
            type = VoicingType::quartal;
            return true;
        case kSpreadVoicingKeyswitchNote:
            type = VoicingType::spread;
            return true;
        case kClosedVoicingKeyswitchNote:
            type = VoicingType::closed;
            return true;
        case kDrop2VoicingKeyswitchNote:
            type = VoicingType::drop2;
            return true;
        case kDrop3VoicingKeyswitchNote:
            type = VoicingType::drop3;
            return true;
        case kDrop24VoicingKeyswitchNote:
            type = VoicingType::drop24;
            return true;
        case kUnisonVoicingKeyswitchNote:
            type = VoicingType::unison;
            return true;
        case kOctavesVoicingKeyswitchNote:
            type = VoicingType::octaves;
            return true;
        case kDoublingVoicingKeyswitchNote:
            type = VoicingType::doubling;
            return true;
        default:
            return false;
    }
}

bool isReservedVoicingTypeKeyswitch(int midiNote) noexcept
{
    switch (midiNote)
    {

        default:
            return false;
    }
}
}

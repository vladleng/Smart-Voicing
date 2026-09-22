#include "VoicingStrategy.h"

namespace smartvoicing::harmony
{
namespace
{
ClosedVoicingContext closedContextFrom(const VoicingContext& context) noexcept
{
    ClosedVoicingContext result;
    result.key = context.key;
    result.harmonic = context.harmonic;
    result.tension = context.tension;
    result.tensionLevel = context.tensionLevel;
    return result;
}
}

VoiceOutput buildVoicing(int melodyNote,
                         VoicingType type,
                         const VoicingContext& context) noexcept
{
    switch (type)
    {
        case VoicingType::closed:
        default:
            return buildClosedVoicing(melodyNote,
                                      context.chord,
                                      closedContextFrom(context));
    }
}

const char* voicingTypeName(VoicingType type) noexcept
{
    switch (type)
    {
        case VoicingType::closed:
            return "Closed";
        default:
            return "Closed";
    }
}
}

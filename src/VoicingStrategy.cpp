#include "VoicingStrategy.h"

#include <array>
#include <utility>

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

bool hasCompleteFourVoiceVertical(const VoiceOutput& voicing) noexcept
{
    for (const auto& voice : voicing.voices)
    {
        if (! voice.active || voice.midiNote < 0 || voice.midiNote > 127)
            return false;
    }

    return true;
}

void sortDescending(std::array<int, 3>& notes) noexcept
{
    for (std::size_t i = 0; i < notes.size(); ++i)
    {
        for (std::size_t j = i + 1; j < notes.size(); ++j)
        {
            if (notes[j] > notes[i])
                std::swap(notes[i], notes[j]);
        }
    }
}
}

VoiceOutput transformClosedToDrop2(const VoiceOutput& closed,
                                   const NormalizedChord& chord) noexcept
{
    // Stage 4/Closed already protects an explicit slash bass in V4. A literal
    // Drop 2 would move another voice below it, so the 0.4b MVP preserves the
    // higher-priority bass contract by falling back to the accepted Closed
    // vertical. A dedicated slash/drop policy can be designed later.
    if (chord.slashBass || ! hasCompleteFourVoiceVertical(closed))
        return closed;

    const auto droppedSecondVoice = closed.voices[1].midiNote - 12;
    if (droppedSecondVoice < 0)
        return closed;

    std::array<int, 3> lowerVoices {
        closed.voices[2].midiNote,
        closed.voices[3].midiNote,
        droppedSecondVoice
    };
    sortDescending(lowerVoices);

    VoiceOutput result = closed;
    // V1 remains the performer-owned melody. V2..V4 remain sounding slots from
    // top to bottom so downstream channel/instrument ordering stays coherent.
    for (std::size_t i = 0; i < lowerVoices.size(); ++i)
    {
        result.voices[i + 1].active = true;
        result.voices[i + 1].midiNote = lowerVoices[i];
    }

    return result;
}

VoiceOutput buildVoicing(int melodyNote,
                         VoicingType type,
                         const VoicingContext& context) noexcept
{
    // Closed is selected exactly once. Drop-family strategies transform this
    // material and therefore cannot silently choose a different tension set.
    const auto closed = buildClosedVoicing(melodyNote,
                                           context.chord,
                                           closedContextFrom(context));

    switch (type)
    {
        case VoicingType::drop2:
            return transformClosedToDrop2(closed, context.chord);
        case VoicingType::closed:
        default:
            return closed;
    }
}

const char* voicingTypeName(VoicingType type) noexcept
{
    switch (type)
    {
        case VoicingType::drop2:
            return "Drop 2";
        case VoicingType::closed:
            return "Closed";
        default:
            return "Closed";
    }
}
}

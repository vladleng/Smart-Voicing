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

VoiceOutput buildUnisonVoicing(int melodyNote) noexcept
{
    VoiceOutput result;
    result.clear();

    if (melodyNote < 0 || melodyNote > 127)
        return result;

    // Four independent sounding slots intentionally share the same MIDI note.
    // The downstream router keeps them separated by channels Ch1..Ch4.
    for (auto& voice : result.voices)
    {
        voice.active = true;
        voice.midiNote = melodyNote;
    }

    return result;
}

VoiceOutput buildOctaveVoicing(int melodyNote) noexcept
{
    VoiceOutput result;
    result.clear();

    if (melodyNote < 0 || melodyNote > 127)
        return result;

    // 0.4c2 project default: lead on top, a doubled middle octave, and a
    // two-octave lower anchor. These are abstract Voice slots, not hard-coded
    // instrument ranges; Stage 7 may later adapt register per Ensemble Profile.
    constexpr std::array<int, 4> offsets { 0, -12, -12, -24 };

    for (std::size_t i = 0; i < offsets.size(); ++i)
    {
        const auto note = melodyNote + offsets[i];
        if (note < 0 || note > 127)
            continue;

        result.voices[i].active = true;
        result.voices[i].midiNote = note;
    }

    return result;
}

VoiceOutput buildDoublingVoicing(int melodyNote) noexcept
{
    VoiceOutput result;
    result.clear();

    if (melodyNote < 0 || melodyNote > 127)
        return result;

    // 0.4c3 project default: two independent unison pairs one octave apart.
    // This complements Unison [0,0,0,0] and Octaves [0,-12,-12,-24]
    // without introducing harmony or instrument-specific range adaptation.
    constexpr std::array<int, 4> offsets { 0, 0, -12, -12 };

    for (std::size_t i = 0; i < offsets.size(); ++i)
    {
        const auto note = melodyNote + offsets[i];
        if (note < 0 || note > 127)
            continue;

        result.voices[i].active = true;
        result.voices[i].midiNote = note;
    }

    return result;
}

VoiceOutput buildVoicing(int melodyNote,
                         VoicingType type,
                         const VoicingContext& context) noexcept
{
    // Unison, Octaves and Doubling are melodic orchestration strategies, not
    // harmonic voicings. They intentionally bypass Closed and therefore cannot
    // accidentally invent or reselect chord/tension material.
    if (type == VoicingType::unison)
        return buildUnisonVoicing(melodyNote);

    if (type == VoicingType::octaves)
        return buildOctaveVoicing(melodyNote);

    if (type == VoicingType::doubling)
        return buildDoublingVoicing(melodyNote);

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
        case VoicingType::doubling:
            return "Doubling";
        case VoicingType::octaves:
            return "Octaves";
        case VoicingType::unison:
            return "Unison";
        case VoicingType::drop2:
            return "Drop 2";
        case VoicingType::closed:
            return "Closed";
        default:
            return "Closed";
    }
}
}

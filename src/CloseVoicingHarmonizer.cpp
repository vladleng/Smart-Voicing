#include "CloseVoicingHarmonizer.h"

#include <array>

namespace smartvoicing::harmony
{
namespace
{
int midiPitchClass(int note) noexcept
{
    auto value = note % kPitchClassCount;
    if (value < 0)
        value += kPitchClassCount;
    return value;
}

bool isChordTone(int midiNote, const NormalizedChord& chord) noexcept
{
    if (midiNote < 0 || midiNote > 127 || ! chord.valid)
        return false;

    auto relative = midiPitchClass(midiNote) - chord.rootPitchClass;
    if (relative < 0)
        relative += kPitchClassCount;

    return chord.hasTone(relative);
}

int nearestChordToneBelow(int upperExclusive,
                          const NormalizedChord& chord,
                          const std::array<bool, kPitchClassCount>& usedPitchClasses,
                          bool requireUnusedPitchClass) noexcept
{
    for (int note = upperExclusive - 1; note >= 0; --note)
    {
        if (! isChordTone(note, chord))
            continue;

        if (requireUnusedPitchClass
            && usedPitchClasses[static_cast<std::size_t>(midiPitchClass(note))])
            continue;

        return note;
    }

    return -1;
}

int nearestPitchClassBelow(int upperExclusive, int pitchClass) noexcept
{
    if (pitchClass < 0 || pitchClass >= kPitchClassCount)
        return -1;

    for (int note = upperExclusive - 1; note >= 0; --note)
        if (midiPitchClass(note) == pitchClass)
            return note;

    return -1;
}
}

VoiceOutput buildCloseVoicing(int melodyNote, const NormalizedChord& chord) noexcept
{
    VoiceOutput output;
    output.clear();

    if (melodyNote < 0 || melodyNote > 127)
        return output;

    output.voices[0].active = true;
    output.voices[0].midiNote = melodyNote;

    // Safe fallback for Stage 3: never guess harmony from Key alone.
    if (! chord.valid)
        return output;

    std::array<bool, kPitchClassCount> usedPitchClasses {};
    usedPitchClasses[static_cast<std::size_t>(midiPitchClass(melodyNote))] = true;

    auto upperExclusive = melodyNote;

    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        auto note = nearestChordToneBelow(upperExclusive, chord, usedPitchClasses, true);

        // Triads and sparse chords do not always provide four distinct pitch
        // classes. Once unique tones are exhausted, double the nearest chord
        // tone while preserving strict top-to-bottom ordering.
        if (note < 0)
            note = nearestChordToneBelow(upperExclusive, chord, usedPitchClasses, false);

        if (note < 0)
            break;

        output.voices[static_cast<std::size_t>(voice)].active = true;
        output.voices[static_cast<std::size_t>(voice)].midiNote = note;
        usedPitchClasses[static_cast<std::size_t>(midiPitchClass(note))] = true;
        upperExclusive = note;
    }

    // For an inversion/slash chord the explicit bass is musical information,
    // not merely spelling. In the first Close-voicing implementation V4 owns
    // that bass when a valid note exists below V3.
    if (chord.slashBass && output.voices[2].active)
    {
        const auto bassNote = nearestPitchClassBelow(output.voices[2].midiNote,
                                                     chord.bassPitchClass);
        if (bassNote >= 0)
        {
            output.voices[3].active = true;
            output.voices[3].midiNote = bassNote;
        }
    }

    return output;
}
}

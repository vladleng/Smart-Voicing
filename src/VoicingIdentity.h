#pragma once

#include "ChordModel.h"

#include <array>

namespace smartvoicing::harmony
{
// A rootless major seventh over its explicit inversion bass can read as the
// minor triad on degree 3 (Cmaj7/E or Cmaj7/G becomes E-G-B). Keep the stated
// root when the fixed melody and the strategy's legal pitch pool permit it.
inline int slashMajorSeventhRootPenalty(const std::array<int, 4>& notes,
                                        const NormalizedChord& chord) noexcept
{
    if (! chord.slashBass || chord.quality != ChordQuality::major
        || ! chord.hasExtension(ChordExtension::majorSeventh))
        return 0;

    for (const int note : notes)
        if ((note % 12 + 12) % 12 == chord.rootPitchClass)
            return 0;

    return 96;
}

// Some hosts send the b9 as a used pitch class without a diatonic degree.
// On a dominant chord that explicit semitone above the root still needs to
// outrank a duplicate guide tone or the preferred spacing of the strategy.
inline int flatNinthOmissionPenalty(const std::array<int, 4>& notes,
                                    const NormalizedChord& chord) noexcept
{
    if (! chord.hasTone(1)
        || (chord.quality != ChordQuality::dominant
            && ! chord.hasAlteration(ChordAlteration::flatNinth)))
        return 0;

    for (const int note : notes)
        if (((note % 12 + 12) % 12 - chord.rootPitchClass + 12) % 12 == 1)
            return 0;

    return 72;
}
}

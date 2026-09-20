#pragma once

#include <JuceHeader.h>
#include "SharedHarmonicContext.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace smartvoicing::debug
{
// Host transport positions and ARA event positions can differ by a tiny
// floating-point amount even when Studio Pro visually places the cursor on the
// same musical grid line. Treat positions inside this window as the same
// start-inclusive boundary. 0.0001 quarter note is only a few samples at
// ordinary tempi, so it fixes cursor/grid jitter without shifting meaningful
// musical event positions.
inline constexpr double kBoundaryTolerancePpq = 1.0e-4;

inline juce::String fifthsName(std::int32_t fifths)
{
    switch (fifths)
    {
        case -7: return "Cb";
        case -6: return "Gb";
        case -5: return "Db";
        case -4: return "Ab";
        case -3: return "Eb";
        case -2: return "Bb";
        case -1: return "F";
        case  0: return "C";
        case  1: return "G";
        case  2: return "D";
        case  3: return "A";
        case  4: return "E";
        case  5: return "B";
        case  6: return "F#";
        case  7: return "C#";
        case  8: return "G#";
        case  9: return "D#";
        case 10: return "A#";
        case 11: return "E#";
        default: return "fifths(" + juce::String(fifths) + ")";
    }
}

inline bool intervalUsed(const std::uint8_t (&intervals)[12], int semitones)
{
    return semitones >= 0 && semitones < 12 && intervals[semitones] != 0;
}

inline bool chordIsUndefined(const SharedChordEvent& event)
{
    for (const auto interval : event.intervals)
        if (interval != 0)
            return false;

    return true;
}

inline juce::String keyText(const SharedKeySignatureEvent& event)
{
    if (event.name[0] != '\0')
        return juce::String::fromUTF8(event.name);

    static constexpr bool majorMask[12] =
        { true, false, true, false, true, true, false, true, false, true, false, true };
    static constexpr bool minorMask[12] =
        { true, false, true, true, false, true, false, true, true, false, true, false };

    bool isMajor = true;
    bool isMinor = true;

    for (int i = 0; i < 12; ++i)
    {
        const auto used = event.intervals[i] != 0;
        isMajor = isMajor && (used == majorMask[i]);
        isMinor = isMinor && (used == minorMask[i]);
    }

    auto result = fifthsName(event.root);
    if (isMajor)
        result << " major";
    else if (isMinor)
        result << " minor";
    else
        result << " key";

    return result;
}

inline juce::String chordText(const SharedChordEvent& event)
{
    if (chordIsUndefined(event))
        return "(no chord)";

    if (event.name[0] != '\0')
        return juce::String::fromUTF8(event.name);

    juce::String result = fifthsName(event.root);

    const auto minorThird = intervalUsed(event.intervals, 3);
    const auto majorThird = intervalUsed(event.intervals, 4);
    const auto diminishedFifth = intervalUsed(event.intervals, 6);
    const auto perfectFifth = intervalUsed(event.intervals, 7);
    const auto augmentedFifth = intervalUsed(event.intervals, 8);
    const auto minorSeventh = intervalUsed(event.intervals, 10);
    const auto majorSeventh = intervalUsed(event.intervals, 11);

    if (minorThird && diminishedFifth)
        result << "dim";
    else if (majorThird && augmentedFifth)
        result << "aug";
    else if (minorThird && perfectFifth)
        result << "m";

    if (majorSeventh)
        result << "maj7";
    else if (minorSeventh)
        result << "7";

    if (event.bass != event.root)
        result << "/" << fifthsName(event.bass);

    return result;
}

template <typename Event>
inline int findActiveEventIndex(const Event* events, int count, double ppq)
{
    if (count <= 0)
        return -1;

    // ARA event positions are start-inclusive. Studio Pro can report a cursor
    // a few floating-point units before the exact ARA event position even when
    // both are visually on the same grid line, so use a tiny PPQ tolerance.
    int active = 0;

    for (int i = 0; i < count; ++i)
    {
        if (events[i].position <= ppq + kBoundaryTolerancePpq)
            active = i;
        else
            break;
    }

    return active;
}

template <typename Event>
inline int findNearestEventIndex(const Event* events, int count, double ppq)
{
    if (count <= 0 || ppq < 0.0)
        return -1;

    int nearest = 0;
    auto bestDistance = (std::numeric_limits<double>::max)();

    for (int i = 0; i < count; ++i)
    {
        const auto distance = std::abs(events[i].position - ppq);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            nearest = i;
        }
    }

    return nearest;
}

inline juce::String boundaryDiagnostics(const SharedHarmonicContextSnapshot& context, double ppq)
{
    juce::String text;

    if (ppq < 0.0)
        return text;

    text << "Boundary diag: PPQ " << juce::String(ppq, 9)
         << " | tolerance " << juce::String(kBoundaryTolerancePpq, 9);

    const auto chordIndex = findNearestEventIndex(context.sheetChords,
                                                  context.sheetChordStoredCount,
                                                  ppq);
    if (chordIndex >= 0)
    {
        const auto position = context.sheetChords[chordIndex].position;
        text << " | nearest chord " << juce::String(position, 9)
             << " | delta " << juce::String(position - ppq, 9);
    }

    return text;
}

inline double tempoBpmAtPpq(const SharedHarmonicContextSnapshot& context, double ppq)
{
    const auto count = context.tempoEntryStoredCount;
    if (count < 2)
        return -1.0;

    int right = 1;

    if (ppq <= context.tempoEntries[0].quarterPosition)
    {
        right = 1;
    }
    else if (ppq >= context.tempoEntries[count - 1].quarterPosition)
    {
        right = count - 1;
    }
    else
    {
        for (int i = 1; i < count; ++i)
        {
            if (ppq < context.tempoEntries[i].quarterPosition)
            {
                right = i;
                break;
            }
        }
    }

    const auto& leftEvent = context.tempoEntries[right - 1];
    const auto& rightEvent = context.tempoEntries[right];

    const auto deltaSeconds = rightEvent.timePosition - leftEvent.timePosition;
    const auto deltaQuarters = rightEvent.quarterPosition - leftEvent.quarterPosition;

    if (std::abs(deltaSeconds) < 1.0e-12)
        return -1.0;

    return (deltaQuarters / deltaSeconds) * 60.0;
}

inline juce::String activeContextText(const SharedHarmonicContextSnapshot& context, double ppq)
{
    juce::String text;

    if (ppq < 0.0)
    {
        text << "Active @ PPQ: n/a (start playback to test current context)\n";
        return text;
    }

    text << "Active @ PPQ " << juce::String(ppq, 6) << ":\n";

    const auto chordIndex = findActiveEventIndex(context.sheetChords,
                                                context.sheetChordStoredCount,
                                                ppq);
    const auto keyIndex = findActiveEventIndex(context.keySignatures,
                                              context.keySignatureStoredCount,
                                              ppq);
    const auto barIndex = findActiveEventIndex(context.barSignatures,
                                              context.barSignatureStoredCount,
                                              ppq);

    text << "  Chord: "
         << (chordIndex >= 0 ? chordText(context.sheetChords[chordIndex]) : "n/a")
         << "\n";
    text << "  Key: "
         << (keyIndex >= 0 ? keyText(context.keySignatures[keyIndex]) : "n/a")
         << "\n";

    if (barIndex >= 0)
    {
        const auto& signature = context.barSignatures[barIndex];
        text << "  Time signature: " << signature.numerator << "/" << signature.denominator << "\n";
    }
    else
    {
        text << "  Time signature: n/a\n";
    }

    const auto bpm = tempoBpmAtPpq(context, ppq);
    text << "  Tempo: " << (bpm > 0.0 ? juce::String(bpm, 2) + " BPM" : "n/a") << "\n";

    return text;
}

inline juce::String timelinePreview(const SharedHarmonicContextSnapshot& context)
{
    juce::String text;

    text << "Chord map (" << context.sheetChordStoredCount
         << "/" << context.sheetChordEventCount << "): ";
    const auto chordPreviewCount = (std::min)(context.sheetChordStoredCount, 8);
    for (int i = 0; i < chordPreviewCount; ++i)
    {
        if (i > 0)
            text << " | ";
        text << juce::String(context.sheetChords[i].position, 6)
             << " " << chordText(context.sheetChords[i]);
    }
    if (context.sheetChordStoredCount > chordPreviewCount)
        text << " | ...";
    text << "\n";

    text << "Key map (" << context.keySignatureStoredCount
         << "/" << context.keySignatureEventCount << "): ";
    const auto keyPreviewCount = (std::min)(context.keySignatureStoredCount, 6);
    for (int i = 0; i < keyPreviewCount; ++i)
    {
        if (i > 0)
            text << " | ";
        text << juce::String(context.keySignatures[i].position, 6)
             << " " << keyText(context.keySignatures[i]);
    }
    if (context.keySignatureStoredCount > keyPreviewCount)
        text << " | ...";
    text << "\n";

    text << "Time-signature map (" << context.barSignatureStoredCount
         << "/" << context.barSignatureEventCount << "): ";
    const auto barPreviewCount = (std::min)(context.barSignatureStoredCount, 6);
    for (int i = 0; i < barPreviewCount; ++i)
    {
        if (i > 0)
            text << " | ";
        const auto& event = context.barSignatures[i];
        text << juce::String(event.position, 6)
             << " " << event.numerator << "/" << event.denominator;
    }
    if (context.barSignatureStoredCount > barPreviewCount)
        text << " | ...";
    text << "\n";

    text << "Tempo map (" << context.tempoEntryStoredCount
         << "/" << context.tempoEntryEventCount << "): ";
    const auto tempoPreviewCount = (std::min)(context.tempoEntryStoredCount, 4);
    for (int i = 0; i < tempoPreviewCount; ++i)
    {
        if (i > 0)
            text << " | ";
        const auto& event = context.tempoEntries[i];
        text << "q" << juce::String(event.quarterPosition, 6)
             << "=" << juce::String(event.timePosition, 6) << "s";
    }
    if (context.tempoEntryStoredCount > tempoPreviewCount)
        text << " | ...";
    text << "\n";

    return text;
}
} // namespace smartvoicing::debug

#include "ChordModel.h"
#include "KeyModel.h"
#include "HarmonicFunction.h"
#include "TensionPolicy.h"
#include "VoicingStrategy.h"

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>

using namespace smartvoicing::harmony;

namespace
{
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (condition)
        return;

    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

VoiceOutput vertical(int v1, int v2, int v3, int v4)
{
    VoiceOutput result;
    result.clear();
    const std::array<int, 4> notes { v1, v2, v3, v4 };
    for (std::size_t i = 0; i < notes.size(); ++i)
    {
        result.voices[i].active = true;
        result.voices[i].midiNote = notes[i];
    }
    return result;
}

ChordContext chord(std::int32_t rootFifths,
                   std::int32_t bassFifths,
                   std::initializer_list<std::pair<int, int>> tones)
{
    ChordContext result;
    result.available = true;
    result.defined = true;
    result.root = rootFifths;
    result.bass = bassFifths;
    for (const auto [semitones, degree] : tones)
        result.intervals.values[static_cast<std::size_t>(semitones)] = static_cast<std::uint8_t>(degree);
    return result;
}

KeyContext key(std::int32_t rootFifths, bool minor)
{
    KeyContext result;
    result.available = true;
    result.defined = true;
    result.root = rootFifths;
    const int majorIntervals[] = { 0, 2, 4, 5, 7, 9, 11 };
    const int minorIntervals[] = { 0, 2, 3, 5, 7, 8, 10 };
    const auto* values = minor ? minorIntervals : majorIntervals;
    for (int i = 0; i < 7; ++i)
        result.intervals.values[static_cast<std::size_t>(values[i])] = 0xFFu;
    return result;
}

VoicingContext makeContext(const NormalizedChord& current,
                           const NormalizedKey& currentKey,
                           int melodyNote,
                           TensionLevel level)
{
    VoicingContext result;
    result.chord = current;
    result.key = currentKey;
    result.harmonic = analyzeHarmonicFunction(current, currentKey);
    result.tension = buildTensionPolicy(current, currentKey, result.harmonic, melodyNote);
    result.tensionLevel = level;
    return result;
}

int pitchClass(int note)
{
    const auto value = note % 12;
    return value < 0 ? value + 12 : value;
}

std::array<int, 4> sortedPitchClasses(const VoiceOutput& voicing)
{
    std::array<int, 4> result {};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = voicing.voices[i].active ? pitchClass(voicing.voices[i].midiNote) : -1;

    for (std::size_t i = 0; i < result.size(); ++i)
        for (std::size_t j = i + 1; j < result.size(); ++j)
            if (result[j] < result[i])
                std::swap(result[i], result[j]);

    return result;
}

void testEnumValuesRemainStable()
{
    expect(static_cast<int>(VoicingType::closed) == 0, "Closed enum value must stay 0");
    expect(static_cast<int>(VoicingType::drop2) == 1, "Drop 2 enum value must stay 1");
    expect(static_cast<int>(VoicingType::unison) == 2, "Unison enum value must stay 2");
    expect(static_cast<int>(VoicingType::octaves) == 3, "Octaves enum value must stay 3");
    expect(static_cast<int>(VoicingType::doubling) == 4, "Doubling enum value must stay 4");
    expect(static_cast<int>(VoicingType::drop3) == 5, "Drop 3 enum value must stay 5");
    expect(static_cast<int>(VoicingType::drop24) == 6, "Drop 2+4 must append as enum value 6");
}

void testPureDrop24LowersSecondAndFourthClosedVoices()
{
    const auto closed = vertical(72, 67, 64, 60);
    NormalizedChord ordinary;
    ordinary.valid = true;

    const auto drop24 = transformClosedToDrop24(closed, ordinary);

    expect(drop24.voices[0].midiNote == 72, "Drop 2+4 must preserve V1 melody exactly");
    expect(drop24.voices[1].midiNote == 64, "Closed V3 must become highest lower voice");
    expect(drop24.voices[2].midiNote == 55, "Closed V2 must be lowered exactly one octave");
    expect(drop24.voices[3].midiNote == 48, "Closed V4 must be lowered exactly one octave");
    expect(sortedPitchClasses(drop24) == sortedPitchClasses(closed),
           "Drop 2+4 must preserve exact Closed pitch classes");
}

void testDispatcherTransformsSelectedClosedMaterial()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto context = makeContext(cmaj7, cMajor, 69, TensionLevel::color);

    const auto closed = buildVoicing(69, VoicingType::closed, context);
    const auto drop24 = buildVoicing(69, VoicingType::drop24, context);

    expect(drop24.voices[0].active && drop24.voices[0].midiNote == closed.voices[0].midiNote,
           "Dispatcher Drop 2+4 must preserve performer-owned V1");
    expect(sortedPitchClasses(drop24) == sortedPitchClasses(closed),
           "Dispatcher Drop 2+4 must preserve Closed harmonic vocabulary");

    const auto expectedSecondDrop = closed.voices[1].midiNote - 12;
    const auto expectedFourthDrop = closed.voices[3].midiNote - 12;
    bool foundSecondDrop = false;
    bool foundFourthDrop = false;
    for (std::size_t i = 1; i < drop24.voices.size(); ++i)
    {
        foundSecondDrop = foundSecondDrop || drop24.voices[i].midiNote == expectedSecondDrop;
        foundFourthDrop = foundFourthDrop || drop24.voices[i].midiNote == expectedFourthDrop;
    }
    expect(foundSecondDrop, "Dispatcher Drop 2+4 must contain Closed V2 lowered by one octave");
    expect(foundFourthDrop, "Dispatcher Drop 2+4 must contain Closed V4 lowered by one octave");
    expect(drop24.voices[1].midiNote >= drop24.voices[2].midiNote
           && drop24.voices[2].midiNote >= drop24.voices[3].midiNote,
           "Drop 2+4 lower slots must stay in sounding top-down order");
}

void testSlashBassRemainsAuthoritative()
{
    const auto closed = vertical(72, 67, 64, 52);
    NormalizedChord slash;
    slash.valid = true;
    slash.slashBass = true;

    const auto actual = transformClosedToDrop24(closed, slash);

    expect(actual.voices[0].midiNote == 72, "Slash-bass Drop 2+4 must preserve V1");
    expect(actual.voices[3].midiNote == 40,
           "Slash-bass Closed V4 must be lowered one octave and remain lowest");
    expect(pitchClass(actual.voices[3].midiNote) == pitchClass(closed.voices[3].midiNote),
           "Slash-bass Drop 2+4 must preserve authoritative bass pitch class");
    expect(actual.voices[1].midiNote >= actual.voices[2].midiNote
           && actual.voices[2].midiNote >= actual.voices[3].midiNote,
           "Slash-bass Drop 2+4 lower slots must remain top-down");
}

void testIncompleteAndUnderflowFallBackToClosed()
{
    NormalizedChord ordinary;
    ordinary.valid = true;

    auto incomplete = vertical(12, 9, 5, 0);
    incomplete.voices[3].active = false;
    incomplete.voices[3].midiNote = -1;
    const auto incompleteResult = transformClosedToDrop24(incomplete, ordinary);
    expect(! incompleteResult.voices[3].active && incompleteResult.voices[3].midiNote == -1,
           "Incomplete Drop 2+4 input must remain unchanged");

    const auto low = vertical(15, 10, 7, 3);
    const auto lowResult = transformClosedToDrop24(low, ordinary);
    for (std::size_t i = 0; i < low.voices.size(); ++i)
        expect(lowResult.voices[i].midiNote == low.voices[i].midiNote,
               "Drop 2+4 MIDI underflow must fall back to Closed without wrapping");
}

void testName()
{
    expect(std::string(voicingTypeName(VoicingType::drop24)) == "Drop 2+4",
           "Voicing Type name must expose Drop 2+4");
}
}

int main()
{
    testEnumValuesRemainStable();
    testPureDrop24LowersSecondAndFourthClosedVoices();
    testDispatcherTransformsSelectedClosedMaterial();
    testSlashBassRemainsAuthoritative();
    testIncompleteAndUnderflowFallBackToClosed();
    testName();

    if (failures != 0)
    {
        std::cerr << failures << " Drop 2+4 strategy test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Drop 2+4 strategy tests passed\n";
    return EXIT_SUCCESS;
}

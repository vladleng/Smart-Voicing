#include "ChordModel.h"
#include "KeyModel.h"
#include "HarmonicFunction.h"
#include "TensionPolicy.h"
#include "VoicingStrategy.h"

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
                           TensionLevel level,
                           const NormalizedChord* next = nullptr)
{
    VoicingContext result;
    result.chord = current;
    result.key = currentKey;
    result.harmonic = next != nullptr
        ? analyzeHarmonicFunction(current, currentKey, *next)
        : analyzeHarmonicFunction(current, currentKey);
    result.tension = buildTensionPolicy(current, currentKey, result.harmonic, melodyNote);
    result.tensionLevel = level;
    return result;
}

ClosedVoicingContext makeLegacyContext(const VoicingContext& context)
{
    ClosedVoicingContext result;
    result.key = context.key;
    result.harmonic = context.harmonic;
    result.tension = context.tension;
    result.tensionLevel = context.tensionLevel;
    return result;
}

void expectSameVoicing(const VoiceOutput& expected,
                       const VoiceOutput& actual,
                       const std::string& label)
{
    for (std::size_t i = 0; i < expected.voices.size(); ++i)
    {
        expect(expected.voices[i].active == actual.voices[i].active,
               label + " voice " + std::to_string(i + 1) + " active mismatch");
        expect(expected.voices[i].midiNote == actual.voices[i].midiNote,
               label + " voice " + std::to_string(i + 1)
               + " note expected " + std::to_string(expected.voices[i].midiNote)
               + " got " + std::to_string(actual.voices[i].midiNote));
    }
}

void testClosedDispatcherPreservesCmaj7()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto context = makeContext(cmaj7, cMajor, 67, TensionLevel::clean);

    const auto expected = buildClosedVoicing(67, cmaj7, makeLegacyContext(context));
    const auto actual = buildVoicing(67, VoicingType::closed, context);
    expectSameVoicing(expected, actual, "Cmaj7 Closed dispatcher");
}

void testClosedDispatcherPreservesTargetAwareDominant()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto context = makeContext(g7, cMajor, 69, TensionLevel::color, &cmaj7);

    const auto expected = buildClosedVoicing(69, g7, makeLegacyContext(context));
    const auto actual = buildVoicing(69, VoicingType::closed, context);
    expectSameVoicing(expected, actual, "G7 -> Cmaj7 target-aware Closed dispatcher");
}

void testClosedDispatcherPreservesMinorTargetRichColour()
{
    const auto aMinor = normalizeKey(key(3, true));
    const auto e7 = normalizeChord(chord(4, 4, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto am = normalizeChord(chord(3, 3, {{0, 1}, {3, 3}, {7, 5}}));
    const auto context = makeContext(e7, aMinor, 71, TensionLevel::rich, &am);

    const auto expected = buildClosedVoicing(71, e7, makeLegacyContext(context));
    const auto actual = buildVoicing(71, VoicingType::closed, context);
    expectSameVoicing(expected, actual, "E7 -> Am Rich Closed dispatcher");
}

void testVoicingTypeName()
{
    expect(std::string(voicingTypeName(VoicingType::closed)) == "Closed",
           "Closed voicing type name");
}
}

int main()
{
    testClosedDispatcherPreservesCmaj7();
    testClosedDispatcherPreservesTargetAwareDominant();
    testClosedDispatcherPreservesMinorTargetRichColour();
    testVoicingTypeName();

    if (failures != 0)
    {
        std::cerr << failures << " VoicingStrategy test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "VoicingStrategy tests passed\n";
    return EXIT_SUCCESS;
}

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

int pitchClass(int midiNote)
{
    const auto value = midiNote % 12;
    return value < 0 ? value + 12 : value;
}

std::array<int, 4> sortedPitchClasses(const VoiceOutput& voicing)
{
    std::array<int, 4> result {};
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = voicing.voices[i].active ? pitchClass(voicing.voices[i].midiNote) : -1;

    for (std::size_t i = 0; i < result.size(); ++i)
    {
        for (std::size_t j = i + 1; j < result.size(); ++j)
        {
            if (result[j] < result[i])
                std::swap(result[i], result[j]);
        }
    }

    return result;
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

void testDrop2TransformsSelectedClosedMaterial()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto context = makeContext(cmaj7, cMajor, 69, TensionLevel::color);

    const auto closed = buildVoicing(69, VoicingType::closed, context);
    const auto drop2 = buildVoicing(69, VoicingType::drop2, context);

    expect(drop2.voices[0].active && drop2.voices[0].midiNote == closed.voices[0].midiNote,
           "Drop 2 must preserve performer-owned V1 melody");
    expect(sortedPitchClasses(drop2) == sortedPitchClasses(closed),
           "Drop 2 must preserve the exact Closed pitch-class material");

    const auto expectedDropped = closed.voices[1].midiNote - 12;
    bool foundDropped = false;
    for (std::size_t i = 1; i < drop2.voices.size(); ++i)
        foundDropped = foundDropped || drop2.voices[i].midiNote == expectedDropped;
    expect(foundDropped,
           "Drop 2 must lower the Closed second voice by exactly one octave");

    expect(drop2.voices[1].midiNote >= drop2.voices[2].midiNote
           && drop2.voices[2].midiNote >= drop2.voices[3].midiNote,
           "Drop 2 lower output slots must remain in sounding top-down order");
}

void testDrop2PreservesMinorTargetRichVocabulary()
{
    const auto cMinor = normalizeKey(key(0, true));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto cm = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto context = makeContext(g7, cMinor, 65, TensionLevel::rich, &cm);

    const auto closed = buildVoicing(65, VoicingType::closed, context);
    const auto drop2 = buildVoicing(65, VoicingType::drop2, context);

    expect(sortedPitchClasses(drop2) == sortedPitchClasses(closed),
           "G7 -> Cm Rich Drop 2 must keep Closed b9/b13 vocabulary unchanged");
}

void testDrop2FallsBackForExplicitSlashBass()
{
    const auto cMajor = normalizeKey(key(0, false));
    // Cmaj7/E: E is encoded as four fifths from C in the provider model.
    const auto cmaj7OverE = normalizeChord(chord(0, 4, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto context = makeContext(cmaj7OverE, cMajor, 67, TensionLevel::color);

    expect(cmaj7OverE.slashBass, "Cmaj7/E test chord must be recognized as slash bass");

    const auto closed = buildVoicing(67, VoicingType::closed, context);
    const auto drop2 = buildVoicing(67, VoicingType::drop2, context);
    expectSameVoicing(closed, drop2,
                      "Drop 2 slash-bass safety fallback must preserve Closed vertical");
}

void testUnisonDuplicatesPerformerMelodyAcrossAllVoices()
{
    VoicingContext context;
    context.tensionLevel = TensionLevel::clean;

    const auto unison = buildVoicing(69, VoicingType::unison, context);
    for (std::size_t i = 0; i < unison.voices.size(); ++i)
    {
        expect(unison.voices[i].active,
               "Unison voice " + std::to_string(i + 1) + " must be active");
        expect(unison.voices[i].midiNote == 69,
               "Unison voice " + std::to_string(i + 1) + " must duplicate melody pitch exactly");
    }
}

void testUnisonDoesNotDependOnTensionLevelOrHarmony()
{
    VoicingContext cleanContext;
    cleanContext.tensionLevel = TensionLevel::clean;

    VoicingContext richContext;
    richContext.tensionLevel = TensionLevel::rich;

    const auto clean = buildVoicing(73, VoicingType::unison, cleanContext);
    const auto rich = buildVoicing(73, VoicingType::unison, richContext);
    expectSameVoicing(clean, rich,
                      "Pure Unison must not invent harmonic differences for Tension Level");
}

void testUnisonRejectsInvalidMelodyNote()
{
    const auto below = buildUnisonVoicing(-1);
    const auto above = buildUnisonVoicing(128);

    for (std::size_t i = 0; i < below.voices.size(); ++i)
    {
        expect(! below.voices[i].active && below.voices[i].midiNote == -1,
               "Invalid low melody must return empty Unison output");
        expect(! above.voices[i].active && above.voices[i].midiNote == -1,
               "Invalid high melody must return empty Unison output");
    }
}

void testOctavesUseDocumentedDefaultLayout()
{
    VoicingContext context;
    context.tensionLevel = TensionLevel::clean;

    const auto octaves = buildVoicing(72, VoicingType::octaves, context);
    const std::array<int, 4> expected { 72, 60, 60, 48 };

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        expect(octaves.voices[i].active,
               "Octaves voice " + std::to_string(i + 1) + " must be active in valid range");
        expect(octaves.voices[i].midiNote == expected[i],
               "Octaves voice " + std::to_string(i + 1)
               + " must follow [0,-12,-12,-24] layout");
        expect(pitchClass(octaves.voices[i].midiNote) == pitchClass(72),
               "Octaves voice " + std::to_string(i + 1) + " must preserve melody pitch class");
    }
}

void testOctavesDoNotDependOnTensionLevelOrHarmony()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));

    const auto cleanContext = makeContext(cmaj7, cMajor, 74, TensionLevel::clean);
    const auto richContext = makeContext(g7, cMajor, 74, TensionLevel::rich, &cmaj7);

    const auto clean = buildVoicing(74, VoicingType::octaves, cleanContext);
    const auto rich = buildVoicing(74, VoicingType::octaves, richContext);
    expectSameVoicing(clean, rich,
                      "Pure Octaves must not invent harmonic differences for Chord or Tension Level");
}

void testOctavesDoNotWrapBelowMidiRange()
{
    const auto octaves = buildOctaveVoicing(12);

    expect(octaves.voices[0].active && octaves.voices[0].midiNote == 12,
           "Low Octaves V1 must preserve performer melody");
    expect(octaves.voices[1].active && octaves.voices[1].midiNote == 0,
           "Low Octaves V2 must use melody-12 when valid");
    expect(octaves.voices[2].active && octaves.voices[2].midiNote == 0,
           "Low Octaves V3 must independently duplicate melody-12 when valid");
    expect(! octaves.voices[3].active && octaves.voices[3].midiNote == -1,
           "Low Octaves V4 must stay inactive instead of wrapping melody-24");
}

void testVoicingTypeName()
{
    expect(std::string(voicingTypeName(VoicingType::closed)) == "Closed",
           "Closed voicing type name");
    expect(std::string(voicingTypeName(VoicingType::drop2)) == "Drop 2",
           "Drop 2 voicing type name");
    expect(std::string(voicingTypeName(VoicingType::unison)) == "Unison",
           "Unison voicing type name");
    expect(std::string(voicingTypeName(VoicingType::octaves)) == "Octaves",
           "Octaves voicing type name");
}
}

int main()
{
    testClosedDispatcherPreservesCmaj7();
    testClosedDispatcherPreservesTargetAwareDominant();
    testClosedDispatcherPreservesMinorTargetRichColour();
    testDrop2TransformsSelectedClosedMaterial();
    testDrop2PreservesMinorTargetRichVocabulary();
    testDrop2FallsBackForExplicitSlashBass();
    testUnisonDuplicatesPerformerMelodyAcrossAllVoices();
    testUnisonDoesNotDependOnTensionLevelOrHarmony();
    testUnisonRejectsInvalidMelodyNote();
    testOctavesUseDocumentedDefaultLayout();
    testOctavesDoNotDependOnTensionLevelOrHarmony();
    testOctavesDoNotWrapBelowMidiRange();
    testVoicingTypeName();

    if (failures != 0)
    {
        std::cerr << failures << " VoicingStrategy test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "VoicingStrategy tests passed\n";
    return EXIT_SUCCESS;
}

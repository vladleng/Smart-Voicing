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

void testDoublingUsesDocumentedPairedLayout()
{
    VoicingContext context;
    context.tensionLevel = TensionLevel::clean;

    const auto doubling = buildVoicing(72, VoicingType::doubling, context);
    const std::array<int, 4> expected { 72, 72, 60, 60 };

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        expect(doubling.voices[i].active,
               "Doubling voice " + std::to_string(i + 1) + " must be active in valid range");
        expect(doubling.voices[i].midiNote == expected[i],
               "Doubling voice " + std::to_string(i + 1)
               + " must follow [0,0,-12,-12] layout");
        expect(pitchClass(doubling.voices[i].midiNote) == pitchClass(72),
               "Doubling voice " + std::to_string(i + 1) + " must preserve melody pitch class");
    }
}

void testDoublingDoesNotDependOnTensionLevelOrHarmony()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));

    const auto cleanContext = makeContext(cmaj7, cMajor, 76, TensionLevel::clean);
    const auto richContext = makeContext(g7, cMajor, 76, TensionLevel::rich, &cmaj7);

    const auto clean = buildVoicing(76, VoicingType::doubling, cleanContext);
    const auto rich = buildVoicing(76, VoicingType::doubling, richContext);
    expectSameVoicing(clean, rich,
                      "Pure Doubling must not invent harmonic differences for Chord or Tension Level");
}

void testDoublingDoesNotWrapBelowMidiRange()
{
    const auto doubling = buildDoublingVoicing(5);

    expect(doubling.voices[0].active && doubling.voices[0].midiNote == 5,
           "Low Doubling V1 must preserve performer melody");
    expect(doubling.voices[1].active && doubling.voices[1].midiNote == 5,
           "Low Doubling V2 must preserve upper unison pair");
    expect(! doubling.voices[2].active && doubling.voices[2].midiNote == -1,
           "Low Doubling V3 must stay inactive instead of wrapping melody-12");
    expect(! doubling.voices[3].active && doubling.voices[3].midiNote == -1,
           "Low Doubling V4 must stay inactive instead of wrapping melody-12");
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
    expect(std::string(voicingTypeName(VoicingType::doubling)) == "Doubling",
           "Doubling voicing type name");
}

void testCleanSlashMajorSeventhKeepsItsRoot()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto overE = normalizeChord(chord(0, 4, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto overG = normalizeChord(chord(0, 1, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    // The score's two slash bars include E4/B4 over E and G4/B4/E5 over G.
    for (const auto& [harmony, melody] : {
             std::pair { &overE, 64 }, { &overE, 71 },
             { &overG, 67 }, { &overG, 71 }, { &overG, 76 } })
    {
        const auto ctx = makeContext(*harmony, cMajor, melody, TensionLevel::clean);
        for (const auto type : { VoicingType::closed, VoicingType::drop2,
                                 VoicingType::drop3, VoicingType::drop24,
                                 VoicingType::spread, VoicingType::quartal,
                                 VoicingType::ust, VoicingType::cluster })
        {
            const auto output = buildVoicing(melody, type, ctx);
            const auto label = std::string(voicingTypeName(type)) + " melody "
                + std::to_string(melody);
            expect(output.voices[0].active && output.voices[0].midiNote == melody,
                   label + " keeps V1");
            expect(output.voices[3].active
                       && pitchClass(output.voices[3].midiNote) == harmony->bassPitchClass,
                   label + " keeps slash bass");
            bool hasRoot = false;
            for (const auto& voice : output.voices)
                hasRoot = hasRoot || (voice.active
                    && pitchClass(voice.midiNote) == harmony->rootPitchClass);
            expect(hasRoot, label + " preserves Cmaj7 identity");
        }
    }
}

void testCleanPartialUstRetainsNamedThirteenth()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto c13 = normalizeChord(chord(0, 0,
        {{0, 1}, {2, 9}, {4, 3}, {10, 7}, {9, 13}}));
    const auto output = buildVoicing(70, VoicingType::ust,
                                     makeContext(c13, cMajor, 70, TensionLevel::clean));
    bool hasThirteenth = false, hasThird = false;
    for (const auto& voice : output.voices)
    {
        hasThirteenth = hasThirteenth || (voice.active && pitchClass(voice.midiNote) == 9);
        hasThird = hasThird || (voice.active && pitchClass(voice.midiNote) == 4);
    }
    expect(output.voices[0].active && output.voices[0].midiNote == 70,
           "C13 UST keeps Bb4 performer melody");
    expect(hasThirteenth, "C13 partial UST keeps named A/13");
    expect(hasThird, "C13 partial UST retains E/3 beside named A/13");
}

void testVoiceLeadingAcrossEightHarmonicStrategies()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto dm7 = normalizeChord(chord(2, 2, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));

    const std::array<VoicingType, 8> types {
        VoicingType::closed, VoicingType::drop2, VoicingType::drop3,
        VoicingType::drop24, VoicingType::spread, VoicingType::quartal,
        VoicingType::ust, VoicingType::cluster
    };

    for (const auto type : types)
    {
        auto initialContext = makeContext(cmaj7, cMajor, 76, TensionLevel::clean);
        const auto initial = buildVoicing(76, type, initialContext);
        VoiceLeadingState state;
        state.commit(initial);

        // Same vertical must not be re-generated differently solely because
        // we remember the exactly sounding version.
        initialContext.voiceLeading = &state;
        const auto repeated = buildVoicing(76, type, initialContext);
        expectSameVoicing(initial, repeated,
                          std::string(voicingTypeName(type)) + " unchanged harmony");

        const std::array<NormalizedChord, 3> progression { dm7, g7, cmaj7 };
        const std::array<int, 3> melodyNotes { 74, 71, 72 };
        for (std::size_t chordIndex = 0; chordIndex < progression.size(); ++chordIndex)
        {
            auto next = makeContext(progression[chordIndex], cMajor,
                                    melodyNotes[chordIndex], TensionLevel::clean);
            next.voiceLeading = &state;
            const auto result = buildVoicing(melodyNotes[chordIndex], type, next);
            expect(result.voices[0].active
                       && result.voices[0].midiNote == melodyNotes[chordIndex],
                   std::string(voicingTypeName(type)) + " performer melody stays immutable");
            for (int voice = 1; voice < kVoiceCount; ++voice)
            {
                const auto& upper = result.voices[static_cast<std::size_t>(voice - 1)];
                const auto& lower = result.voices[static_cast<std::size_t>(voice)];
                if (upper.active && lower.active)
                    expect(upper.midiNote >= lower.midiNote,
                           std::string(voicingTypeName(type)) + " preserves sounding order");
            }
            state.commit(result);
        }
    }
}

void testVoiceLeadingCanActuallyChangeClosedCandidate()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto dm7 = normalizeChord(chord(2, 2, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    bool changedOutput = false;

    // Search representative melody registers. This is a test fixture search,
    // never executed on the realtime audio thread.
    for (int previousMelody = 64; previousMelody <= 79 && !changedOutput; ++previousMelody)
    {
        const auto oldContext = makeContext(cmaj7, cMajor, previousMelody,
                                             TensionLevel::clean);
        VoiceLeadingState previous;
        previous.commit(buildVoicing(previousMelody, VoicingType::closed, oldContext));
        if (!previous.hasPrevious())
            continue;

        for (int melody = 65; melody <= 78 && !changedOutput; ++melody)
        {
            auto next = makeContext(dm7, cMajor, melody, TensionLevel::clean);
            const auto withoutHistory = buildVoicing(melody, VoicingType::closed, next);
            next.voiceLeading = &previous;
            const auto withHistory = buildVoicing(melody, VoicingType::closed, next);
            for (int voice = 1; voice < kVoiceCount; ++voice)
                changedOutput = changedOutput
                    || withoutHistory.voices[static_cast<std::size_t>(voice)].midiNote
                    != withHistory.voices[static_cast<std::size_t>(voice)].midiNote;
        }
    }
    expect(changedOutput, "history must change an actual Closed voicing in a Cmaj7 -> Dm7 turn");
}

void testBoundedClosedAndDropContinuity()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto dm7 = normalizeChord(chord(2, 2, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto cmaj7OverG = normalizeChord(chord(0, 1,
        {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const std::array<VoicingType, 4> family {
        VoicingType::closed, VoicingType::drop2,
        VoicingType::drop3, VoicingType::drop24
    };
    for (const auto type : family)
        for (const auto& nextChord : { dm7, g7, cmaj7, cmaj7OverG })
            for (int preceding = 67; preceding <= 76; preceding += 3)
            {
                VoiceLeadingState previous;
                previous.commit(buildVoicing(preceding, type,
                    makeContext(cmaj7, cMajor, preceding, TensionLevel::clean)));
                if (! previous.hasPrevious())
                    continue;
                for (int melody = 67; melody <= 76; melody += 3)
                {
                    auto current = makeContext(nextChord, cMajor,
                                               melody, TensionLevel::clean);
                    const auto baseline = buildVoicing(melody, type, current);
                    current.voiceLeading = &previous;
                    const auto ranked = buildVoicing(melody, type, current);
                    expect(ranked.voices[0].active && ranked.voices[0].midiNote == melody,
                           "0.5b never shifts performer V1");
                    if (! baseline.voices[3].active || ! ranked.voices[3].active)
                        continue;

                    const auto baseMotion = evaluateVoiceLeading(baseline, previous);
                    const auto rankedMotion = evaluateVoiceLeading(ranked, previous);
                    expect(baseMotion.valid && rankedMotion.valid
                               && rankedMotion.cost <= baseMotion.cost,
                           std::string(voicingTypeName(type))
                           + " continuity may not increase total movement against static reference");
                    if (nextChord.slashBass)
                        expect(ranked.voices[3].midiNote % 12 == nextChord.bassPitchClass,
                               "slash bass remains authoritative");
                    if (type == VoicingType::closed)
                        expect(ranked.voices[0].midiNote - ranked.voices[3].midiNote <= 24,
                               "Closed remains within the 24 semitone search depth");
                }
            }
}

void testClosedHistoryRespectsUpperSpacingAndStructuralBass()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7OverE = normalizeChord(chord(0, 4,
        {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));

    // The score comparison exposed B3-C3 (11 semitones) in a four-way Closed.
    // This fixture deliberately offers a low previous V2 so motion ranking
    // would be tempted to open the upper gap.
    VoiceLeadingState previous;
    VoiceOutput previousSounding;
    const std::array<int, kVoiceCount> starting { 55, 48, 43, 40 };
    for (int i = 0; i < kVoiceCount; ++i)
        previousSounding.voices[static_cast<std::size_t>(i)] = {
            true, starting[static_cast<std::size_t>(i)]
        };
    previous.commit(previousSounding);
    auto current = makeContext(cmaj7OverE, cMajor, 59, TensionLevel::clean);
    const auto staticVoicing = buildVoicing(59, VoicingType::closed, current);
    current.voiceLeading = &previous;
    const auto optimized = buildVoicing(59, VoicingType::closed, current);
    const auto staticGap = staticVoicing.voices[0].midiNote
                         - staticVoicing.voices[1].midiNote;
    const auto allowedGap = staticGap > 7 ? staticGap : 7;
    expect(optimized.voices[0].midiNote - optimized.voices[1].midiNote <= allowedGap,
           "history cannot open Closed upper-support gap to an exposed seventh");
    expect(optimized.voices[3].midiNote % 12 == cmaj7OverE.bassPitchClass,
           "Cmaj7/E explicit E bass stays lowest");
    expect(optimized.voices[0].midiNote == 59, "Cmaj7/E keeps performer B3");
}

void testClosedHistoryDoesNotReplaceStructuralBassWithFlatNinth()
{
    const auto cMajor = normalizeKey(key(0, false));
    for (const auto annotation : { 9, 255 })
    {
        // G7(b9) is an explicit chromatic extension even when the ARA source
        // supplies generic-used instead of the explicit ninth degree marker.
        const auto g7b9 = normalizeChord(chord(1, 1,
            {{0, 1}, {1, annotation}, {4, 3}, {7, 5}, {10, 7}}));
        auto current = makeContext(g7b9, cMajor, 56, TensionLevel::clean);
        const auto staticVoicing = buildVoicing(56, VoicingType::closed, current);

        VoiceLeadingState previous;
        VoiceOutput previousSounding;
        const std::array<int, kVoiceCount> prior { 60, 55, 50, 43 };
        for (int i = 0; i < kVoiceCount; ++i)
            previousSounding.voices[static_cast<std::size_t>(i)] = {
                true, prior[static_cast<std::size_t>(i)]
            };
        previous.commit(previousSounding);
        current.voiceLeading = &previous;
        const auto result = buildVoicing(56, VoicingType::closed, current);
        const int flatNinthPitchClass = (g7b9.rootPitchClass + 1) % 12;
        if (staticVoicing.voices[3].active
            && staticVoicing.voices[3].midiNote % 12 != flatNinthPitchClass)
            expect(result.voices[3].midiNote % 12 != flatNinthPitchClass,
                   "Flat ninth must not displace structural Closed bass");
        expect(result.voices[0].midiNote == 56, "G7b9 keeps Ab3 melody");
        bool foundThird = false, foundSeventh = false;
        for (const auto& slot : result.voices)
        {
            if (! slot.active)
                continue;
            foundThird = foundThird || slot.midiNote % 12 == 11;
            foundSeventh = foundSeventh || slot.midiNote % 12 == 5;
        }
        expect(foundThird && foundSeventh, "G7b9 must keep B and F guide tones");
    }
}

void testClosedCleanRetainsNaturalThirteenthAndSlashBassRange()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto c7add13 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {7, 5}, {10, 7}, {9, 13}}));
    VoiceLeadingState previous;
    VoiceOutput prior;
    const std::array<int, kVoiceCount> last { 58, 55, 52, 48 };
    for (int i = 0; i < kVoiceCount; ++i)
        prior.voices[static_cast<std::size_t>(i)] = {
            true, last[static_cast<std::size_t>(i)]
        };
    previous.commit(prior);
    auto current = makeContext(c7add13, cMajor, 58, TensionLevel::clean);
    const auto baseline = buildVoicing(58, VoicingType::closed, current);
    current.voiceLeading = &previous;
    const auto ranked = buildVoicing(58, VoicingType::closed, current);
    bool written13Baseline = false, written13Ranked = false;
    for (const auto& voice : baseline.voices)
        written13Baseline = written13Baseline
            || (voice.active && voice.midiNote % 12 == 9);
    for (const auto& voice : ranked.voices)
        written13Ranked = written13Ranked
            || (voice.active && voice.midiNote % 12 == 9);
    expect(! written13Baseline || written13Ranked,
           "natural written thirteenth remains available under bass protection");

    const auto slash = normalizeChord(chord(0, 1,
        {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    auto slashContext = makeContext(slash, cMajor, 64, TensionLevel::clean);
    slashContext.voiceLeading = &previous;
    const auto rankedSlash = buildVoicing(64, VoicingType::closed, slashContext);
    expect(rankedSlash.voices[3].active
           && rankedSlash.voices[3].midiNote % 12 == slash.bassPitchClass,
           "Cmaj7/G bass identity survives melody at E4");
    // E4 MIDI64 - G2 MIDI43 = 21, within the 24-semitone search depth.
    expect(64 - 43 == 21, "E4 to G2 is 21 semitones, not >24");
}

void testLowRegisterClosedAvoidsRootFlatNineSemitone()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto g7b9 = normalizeChord(chord(1, 1,
        {{0, 1}, {1, 9}, {4, 3}, {7, 5}, {10, 7}}));

    // Same harmonic bar as the user's 32-bar Clean score.
    // F3 melody makes G2 and Ab2 an unnecessarily muddy minor second;
    // a rootless G7b9 (3/b7/b9 plus fifth) is an acceptable alternative.
    const auto referenceContext = makeContext(g7b9, cMajor, 53,
                                              TensionLevel::clean);
    const auto noHistory = buildVoicing(53, VoicingType::closed, referenceContext);

    VoiceLeadingState previous;
    const auto earlier = buildVoicing(56, VoicingType::closed,
        makeContext(g7b9, cMajor, 56, TensionLevel::clean));
    previous.commit(earlier);
    auto currentContext = referenceContext;
    currentContext.voiceLeading = &previous;
    const auto optimized = buildVoicing(53, VoicingType::closed, currentContext);

    for (const auto& vertical : { noHistory, optimized })
    {
        expect(vertical.voices[0].active && vertical.voices[0].midiNote == 53,
               "low-register test keeps F3 performer melody");
        bool third = false, seventh = false, flatNinth = false;
        for (const auto& slot : vertical.voices)
        {
            if (! slot.active) continue;
            third = third || slot.midiNote % 12 == 11; // B
            seventh = seventh || slot.midiNote % 12 == 5; // F
            flatNinth = flatNinth || slot.midiNote % 12 == 8; // Ab
        }
        expect(third && seventh && flatNinth,
               "G7b9 rootless alternative preserves 3, b7 and explicit b9");
        const auto& upperLow = vertical.voices[2];
        const auto& lowerLow = vertical.voices[3];
        expect(upperLow.active && lowerLow.active,
               "Closed must not drop a voice to escape low interval");
        if (upperLow.active && lowerLow.active)
            expect(upperLow.midiNote >= 48 || lowerLow.midiNote >= 48
                       || upperLow.midiNote - lowerLow.midiNote > 1,
                   "G7b9 should prefer an alternative to G2-Ab2 low semitone");
    }
}

void testBassRegisterPenaltyDoesNotForbidSlashOrUpperCloseSemitones()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto cmaj7G = normalizeChord(chord(0, 1,
        {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    for (const auto melody : { 60, 64, 67 })
    {
        auto ctx = makeContext(cmaj7, cMajor, melody, TensionLevel::clean);
        auto closed = buildVoicing(melody, VoicingType::closed, ctx);
        expect(closed.voices[0].midiNote == melody,
               "Cmaj7 melody remains unchanged under low-register penalty");

        ctx = makeContext(cmaj7G, cMajor, melody, TensionLevel::clean);
        closed = buildVoicing(melody, VoicingType::closed, ctx);
        expect(closed.voices[3].active
                   && closed.voices[3].midiNote % 12 == 7,
               "Cmaj7/G explicit G in V4 overrides low-register preference");
    }
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
    testDoublingUsesDocumentedPairedLayout();
    testDoublingDoesNotDependOnTensionLevelOrHarmony();
    testDoublingDoesNotWrapBelowMidiRange();
    testVoicingTypeName();
    testCleanSlashMajorSeventhKeepsItsRoot();
    testCleanPartialUstRetainsNamedThirteenth();
    testVoiceLeadingAcrossEightHarmonicStrategies();
    testVoiceLeadingCanActuallyChangeClosedCandidate();
    testBoundedClosedAndDropContinuity();
    testClosedHistoryRespectsUpperSpacingAndStructuralBass();
    testClosedHistoryDoesNotReplaceStructuralBassWithFlatNinth();
    testClosedCleanRetainsNaturalThirteenthAndSlashBassRange();
    testLowRegisterClosedAvoidsRootFlatNineSemitone();
    testBassRegisterPenaltyDoesNotForbidSlashOrUpperCloseSemitones();

    if (failures != 0)
    {
        std::cerr << failures << " VoicingStrategy test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "VoicingStrategy tests passed\n";
    return EXIT_SUCCESS;
}

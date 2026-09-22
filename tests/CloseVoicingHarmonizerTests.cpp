#include "ChordModel.h"
#include "CloseVoicingHarmonizer.h"

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

ClosedVoicingContext context(HarmonicFunction function = HarmonicFunction::undefined)
{
    ClosedVoicingContext result;
    if (function != HarmonicFunction::undefined)
    {
        result.harmonic.valid = true;
        result.harmonic.effectiveFunction = function;
    }
    return result;
}

ClosedVoicingContext keyAwareContext(const NormalizedChord& chordModel,
                                     const NormalizedKey& keyModel,
                                     int melodyNote,
                                     TensionLevel level = TensionLevel::clean,
                                     const NormalizedChord* nextChord = nullptr)
{
    ClosedVoicingContext result;
    result.key = keyModel;
    result.harmonic = nextChord != nullptr
        ? analyzeHarmonicFunction(chordModel, keyModel, *nextChord)
        : analyzeHarmonicFunction(chordModel, keyModel);
    result.tension = buildTensionPolicy(chordModel, keyModel, result.harmonic, melodyNote);
    result.tensionLevel = level;
    return result;
}

void expectVoice(const VoiceOutput& output, int voice, int midiNote, const std::string& label)
{
    const auto index = static_cast<std::size_t>(voice);
    expect(output.voices[index].active, label + " active");
    expect(output.voices[index].midiNote == midiNote,
           label + " expected " + std::to_string(midiNote)
           + " got " + std::to_string(output.voices[index].midiNote));
}

bool containsPitchClass(const VoiceOutput& output, int pitchClass)
{
    for (const auto& voice : output.voices)
    {
        if (! voice.active || voice.midiNote < 0)
            continue;

        auto value = voice.midiNote % 12;
        if (value < 0)
            value += 12;
        if (value == pitchClass)
            return true;
    }

    return false;
}

void testMaj7ClosedVoicingKeepsStage3Reference()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildClosedVoicing(67, cmaj7, context()); // G4

    expectVoice(output, 0, 67, "Cmaj7 V1 melody G4");
    expectVoice(output, 1, 64, "Cmaj7 V2 E4");
    expectVoice(output, 2, 60, "Cmaj7 V3 C4");
    expectVoice(output, 3, 59, "Cmaj7 V4 B3");
}

void testMaj7RootMelodyPrefersTrueClosedSpan()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildClosedVoicing(60, cmaj7, context()); // C4 root melody

    expectVoice(output, 0, 60, "Cmaj7 root melody C4 preserved");
    expectVoice(output, 1, 59, "Cmaj7 root melody V2 B3 seventh");
    expectVoice(output, 2, 55, "Cmaj7 root melody V3 G3 fifth");
    expectVoice(output, 3, 52, "Cmaj7 root melody V4 E3 third");
    expect(output.voices[0].midiNote - output.voices[3].midiNote <= 12,
           "Cmaj7 root melody stays within one-octave Closed span");
}

void testNinthMelodyBuildsMusicalClosedVertical()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildClosedVoicing(62, cmaj7, context()); // D4 = melody-imposed 9

    expectVoice(output, 0, 62, "Cmaj7/9 melody D4 preserved");
    expectVoice(output, 1, 59, "Cmaj7/9 V2 B3 preferred 3rd below melody");
    expectVoice(output, 2, 55, "Cmaj7/9 V3 G3");
    expectVoice(output, 3, 52, "Cmaj7/9 V4 E3 guide tone");
}

void testMinorNinthMelodyUsesThirdAndSeventh()
{
    const auto dm7 = normalizeChord(chord(2, 2, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto output = buildClosedVoicing(64, dm7, context(HarmonicFunction::predominant)); // E4 = 9

    expectVoice(output, 0, 64, "Dm7/9 melody E4 preserved");
    expectVoice(output, 1, 60, "Dm7/9 V2 C4 seventh");
    expectVoice(output, 2, 57, "Dm7/9 V3 A3 fifth");
    expectVoice(output, 3, 53, "Dm7/9 V4 F3 third");
}

void testDominantGuideTonesPermitRootOmission()
{
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto output = buildClosedVoicing(69, g7, context(HarmonicFunction::dominant)); // A4 = 9

    expectVoice(output, 0, 69, "G7/9 melody A4 preserved");
    expectVoice(output, 1, 65, "G7/9 V2 F4 seventh");
    expectVoice(output, 2, 62, "G7/9 V3 D4 fifth");
    expectVoice(output, 3, 59, "G7/9 V4 B3 third");

    for (const auto& voice : output.voices)
        expect(voice.midiNote % 12 != 7, "G7/9 root G may be omitted when 3rd/7th define harmony");
}

void testTensionLevelsChangeColourWithoutChangingMelody()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));

    const auto cleanCtx = keyAwareContext(cmaj7, cMajor, 67, TensionLevel::clean);
    const auto colorCtx = keyAwareContext(cmaj7, cMajor, 67, TensionLevel::color);
    const auto richCtx = keyAwareContext(cmaj7, cMajor, 67, TensionLevel::rich);

    const auto clean = buildClosedVoicing(67, cmaj7, cleanCtx);
    const auto color = buildClosedVoicing(67, cmaj7, colorCtx);
    const auto rich = buildClosedVoicing(67, cmaj7, richCtx);

    expectVoice(clean, 0, 67, "Clean Cmaj7 V1 G4");
    expectVoice(clean, 1, 64, "Clean Cmaj7 V2 E4");
    expectVoice(clean, 2, 60, "Clean Cmaj7 V3 C4");
    expectVoice(clean, 3, 59, "Clean Cmaj7 V4 B3");

    expectVoice(color, 0, 67, "Color Cmaj7 V1 G4");
    expectVoice(color, 1, 64, "Color Cmaj7 V2 E4 guide tone");
    expectVoice(color, 2, 62, "Color Cmaj7 V3 D4 preferred 9");
    expectVoice(color, 3, 59, "Color Cmaj7 V4 B3 guide tone");

    expectVoice(rich, 0, 67, "Rich Cmaj7 V1 remains performer melody");
    expect(richCtx.tension.isHarmonyCandidate(2, TensionLevel::rich),
           "Rich keeps normal preferred tensions available");
}

void testAvoidAsHarmonyNeverDisplacesPerformerMelody()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto ctx = keyAwareContext(cmaj7, cMajor, 65, TensionLevel::rich); // F4 = avoid 11, performer-owned
    const auto output = buildClosedVoicing(65, cmaj7, ctx);

    expect(ctx.tension.tone(5).role == TensionRole::avoidAsHarmony,
           "Cmaj7 F/11 remains avoid-as-harmony");
    expect(ctx.tension.tone(5).melodyImposed,
           "Cmaj7 F/11 is independently marked melody-imposed");
    expectVoice(output, 0, 65, "avoid melody F4 remains V1");

    for (int voice = 1; voice < 4; ++voice)
    {
        const auto note = output.voices[static_cast<std::size_t>(voice)].midiNote;
        expect((note % 12 + 12) % 12 != 5,
               "avoid F pitch class is not generated in lower harmony");
    }
}

void testResolutionAwareContextNowFeedsDominantTensions()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto d7 = normalizeChord(chord(2, 2, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto am7 = normalizeChord(chord(3, 3, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));

    auto confirmed = keyAwareContext(d7, cMajor, 69, TensionLevel::color, &g7);
    expect(confirmed.harmonic.appliedDominantConfirmed,
           "D7->G carries confirmed V/V evidence into Closed context");
    expect(confirmed.harmonic.dominantResolutionConfirmed,
           "D7->G also carries generic dominant target evidence");

    auto candidateOnly = keyAwareContext(d7, cMajor, 69, TensionLevel::color, &am7);
    expect(candidateOnly.harmonic.appliedDominantCandidate,
           "D7->Am keeps V/V candidate evidence");
    expect(! candidateOnly.harmonic.appliedDominantConfirmed,
           "D7->Am is not falsely confirmed");
    expect(! candidateOnly.harmonic.dominantResolutionConfirmed,
           "D7->Am does not borrow Am target quality");

    const auto confirmedOutput = buildClosedVoicing(69, d7, confirmed); // A4
    const auto candidateOutput = buildClosedVoicing(69, d7, candidateOnly);

    expectVoice(confirmedOutput, 0, 69, "confirmed D7 V1 melody A4");
    expectVoice(confirmedOutput, 1, 66, "confirmed D7 V2 F#4 guide tone");
    expectVoice(confirmedOutput, 2, 64, "confirmed D7 V3 E4 preferred 9");
    expectVoice(confirmedOutput, 3, 60, "confirmed D7 V4 C4 guide tone");

    for (int voice = 0; voice < 4; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        expect(confirmedOutput.voices[index].midiNote == candidateOutput.voices[index].midiNote,
               "confirmed/unconfirmed evidence still shares conservative Color baseline");
    }
}

void testRichAdmitsAlteredDominantPoolButDoesNotRewriteChord()
{
    const auto cMajor = normalizeKey(key(0, false));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto ctx = keyAwareContext(g7, cMajor, 71, TensionLevel::rich); // B4 melody = chord 3rd

    expect(ctx.tension.tone(1).alteredCandidate,
           "Rich dominant context marks b9 as altered candidate");
    expect(ctx.tension.isHarmonyCandidate(1, TensionLevel::rich),
           "Rich admits b9 to candidate pool");
    expect(! ctx.tension.isHarmonyCandidate(1, TensionLevel::color),
           "Color does not admit inferred b9");

    const auto output = buildClosedVoicing(71, g7, ctx);
    expectVoice(output, 0, 71, "Rich G7 keeps B4 melody unchanged");
}

void testHalfDiminishedKeepsCharacteristicFlatFifth()
{
    const auto aMinor = normalizeKey(key(3, true));
    const auto bm7b5 = normalizeChord(chord(5, 5, {{0, 1}, {3, 3}, {6, 5}, {10, 7}}));

    expect(bm7b5.quality == ChordQuality::halfDiminished,
           "Bm7b5 normalized as half-diminished");

    for (const auto level : { TensionLevel::clean, TensionLevel::color, TensionLevel::rich })
    {
        const auto ctx = keyAwareContext(bm7b5, aMinor, 59, level); // B3 melody/root
        const auto output = buildClosedVoicing(59, bm7b5, ctx);

        expectVoice(output, 0, 59, "Bm7b5 keeps performer B3 melody");
        expect(containsPitchClass(output, 5),
               "Bm7b5 keeps characteristic F/b5 instead of replacing it with inferred 11");
    }
}

void testMinorDominantRichUsesDirectedTension()
{
    const auto aMinor = normalizeKey(key(3, true));
    const auto e7 = normalizeChord(chord(4, 4, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto am7 = normalizeChord(chord(3, 3, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));

    const auto colorCtx = keyAwareContext(e7, aMinor, 59, TensionLevel::color, &am7); // B3
    const auto richCtx = keyAwareContext(e7, aMinor, 59, TensionLevel::rich, &am7);
    const auto color = buildClosedVoicing(59, e7, colorCtx);
    const auto rich = buildClosedVoicing(59, e7, richCtx);

    expect(colorCtx.tension.functionalProfile == FunctionalTensionProfile::dominantMinorTarget,
           "E7->Am uses minor-target profile in Color too");
    expect(! colorCtx.tension.isHarmonyCandidate(1, TensionLevel::color),
           "Color does not auto-add E7 b9");
    expect(! containsPitchClass(color, 1),
           "Color E7->Am does not insert C# natural 13 from generic Mixolydian");

    expect(richCtx.tension.functionalProfile == FunctionalTensionProfile::dominantMinorTarget,
           "Rich E7->Am sees minor target");
    expect(richCtx.tension.resolutionConfirmed,
           "Rich E7->Am has confirmed resolution evidence");
    expect(containsPitchClass(rich, 5),
           "Rich E7->Am selects functionally directed F/b9 for this voicing");
    expect(! containsPitchClass(rich, 1),
           "Rich E7->Am does not use C# natural 13 as blind dominant colour");
}

void testPlainTriadStaysConservativeEvenWithKey()
{
    const auto cMajorKey = normalizeKey(key(0, false));
    const auto cMajor = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}}));
    const auto ctx = keyAwareContext(cMajor, cMajorKey, 67, TensionLevel::rich);
    const auto output = buildClosedVoicing(67, cMajor, ctx); // G4

    expectVoice(output, 0, 67, "triad V1 G4");
    expectVoice(output, 1, 64, "triad V2 E4");
    expectVoice(output, 2, 60, "triad V3 C4");
    expectVoice(output, 3, 55, "triad V4 G3 structural doubling");
}

void testSlashBassOwnsV4()
{
    const auto cOverE = normalizeChord(chord(0, 4, {{0, 1}, {4, 3}, {7, 5}})); // C/E
    const auto output = buildClosedVoicing(67, cOverE, context()); // G4

    expectVoice(output, 0, 67, "C/E V1 G4");
    expectVoice(output, 1, 64, "C/E V2 E4");
    expectVoice(output, 2, 60, "C/E V3 C4");
    expectVoice(output, 3, 52, "C/E V4 explicit E bass");
}

void testNoChordFallback()
{
    NormalizedChord noChord;
    const auto output = buildClosedVoicing(67, noChord, context());

    expectVoice(output, 0, 67, "fallback V1 melody");
    for (int voice = 1; voice < 4; ++voice)
        expect(! output.voices[static_cast<std::size_t>(voice)].active,
               "fallback lower voice inactive");
}

void testLegacyEntryPointUsesSameClosedEngine()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto legacy = buildCloseVoicing(62, cmaj7);
    const auto closed = buildClosedVoicing(62, cmaj7, context());

    for (int voice = 0; voice < 4; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        expect(legacy.voices[index].active == closed.voices[index].active,
               "legacy/closed active state matches");
        expect(legacy.voices[index].midiNote == closed.voices[index].midiNote,
               "legacy/closed note matches");
    }
}
}

int main()
{
    testMaj7ClosedVoicingKeepsStage3Reference();
    testMaj7RootMelodyPrefersTrueClosedSpan();
    testNinthMelodyBuildsMusicalClosedVertical();
    testMinorNinthMelodyUsesThirdAndSeventh();
    testDominantGuideTonesPermitRootOmission();
    testTensionLevelsChangeColourWithoutChangingMelody();
    testAvoidAsHarmonyNeverDisplacesPerformerMelody();
    testResolutionAwareContextNowFeedsDominantTensions();
    testRichAdmitsAlteredDominantPoolButDoesNotRewriteChord();
    testHalfDiminishedKeepsCharacteristicFlatFifth();
    testMinorDominantRichUsesDirectedTension();
    testPlainTriadStaysConservativeEvenWithKey();
    testSlashBassOwnsV4();
    testNoChordFallback();
    testLegacyEntryPointUsesSameClosedEngine();

    if (failures != 0)
    {
        std::cerr << failures << " Closed Voicing test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.3e Functional Tension tests passed.\n";
    return EXIT_SUCCESS;
}

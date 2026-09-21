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

void expectVoice(const VoiceOutput& output, int voice, int midiNote, const std::string& label)
{
    const auto index = static_cast<std::size_t>(voice);
    expect(output.voices[index].active, label + " active");
    expect(output.voices[index].midiNote == midiNote,
           label + " expected " + std::to_string(midiNote)
           + " got " + std::to_string(output.voices[index].midiNote));
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

void testTriadStillRetainsRootIdentity()
{
    const auto cMajor = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}}));
    const auto output = buildClosedVoicing(67, cMajor, context()); // G4

    expectVoice(output, 0, 67, "triad V1 G4");
    expectVoice(output, 1, 64, "triad V2 E4");
    expectVoice(output, 2, 60, "triad V3 C4");
    expectVoice(output, 3, 55, "triad V4 G3 doubling nearest structural tone");
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
    testNinthMelodyBuildsMusicalClosedVertical();
    testMinorNinthMelodyUsesThirdAndSeventh();
    testDominantGuideTonesPermitRootOmission();
    testTriadStillRetainsRootIdentity();
    testSlashBassOwnsV4();
    testNoChordFallback();
    testLegacyEntryPointUsesSameClosedEngine();

    if (failures != 0)
    {
        std::cerr << failures << " Closed Voicing test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.3b Closed Voicing tests passed.\n";
    return EXIT_SUCCESS;
}

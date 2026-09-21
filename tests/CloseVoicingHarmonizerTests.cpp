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

void expectVoice(const VoiceOutput& output, int voice, int midiNote, const std::string& label)
{
    const auto index = static_cast<std::size_t>(voice);
    expect(output.voices[index].active, label + " active");
    expect(output.voices[index].midiNote == midiNote,
           label + " expected " + std::to_string(midiNote)
           + " got " + std::to_string(output.voices[index].midiNote));
}

void testMaj7CloseVoicing()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildCloseVoicing(67, cmaj7); // G4

    expectVoice(output, 0, 67, "Cmaj7 V1 melody G4");
    expectVoice(output, 1, 64, "Cmaj7 V2 E4");
    expectVoice(output, 2, 60, "Cmaj7 V3 C4");
    expectVoice(output, 3, 59, "Cmaj7 V4 B3");
}

void testNonChordMelodyIsPreserved()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildCloseVoicing(66, cmaj7); // F#4, intentionally outside Cmaj7

    expectVoice(output, 0, 66, "non-chord melody remains F#4");
    expectVoice(output, 1, 64, "non-chord V2 E4");
    expectVoice(output, 2, 60, "non-chord V3 C4");
    expectVoice(output, 3, 59, "non-chord V4 B3");
}

void testTriadDoublingStaysClose()
{
    const auto cMajor = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}}));
    const auto output = buildCloseVoicing(67, cMajor); // G4

    expectVoice(output, 0, 67, "triad V1 G4");
    expectVoice(output, 1, 64, "triad V2 E4");
    expectVoice(output, 2, 60, "triad V3 C4");
    expectVoice(output, 3, 55, "triad V4 G3 doubling nearest tone");
}

void testSlashBassOwnsV4()
{
    const auto cOverE = normalizeChord(chord(0, 4, {{0, 1}, {4, 3}, {7, 5}})); // C/E
    const auto output = buildCloseVoicing(67, cOverE); // G4

    expectVoice(output, 0, 67, "C/E V1 G4");
    expectVoice(output, 1, 64, "C/E V2 E4");
    expectVoice(output, 2, 60, "C/E V3 C4");
    expectVoice(output, 3, 52, "C/E V4 explicit E bass");
}

void testNoChordFallback()
{
    NormalizedChord noChord;
    const auto output = buildCloseVoicing(67, noChord);

    expectVoice(output, 0, 67, "fallback V1 melody");
    for (int voice = 1; voice < 4; ++voice)
        expect(! output.voices[static_cast<std::size_t>(voice)].active,
               "fallback lower voice inactive");
}
}

int main()
{
    testMaj7CloseVoicing();
    testNonChordMelodyIsPreserved();
    testTriadDoublingStaysClose();
    testSlashBassOwnsV4();
    testNoChordFallback();

    if (failures != 0)
    {
        std::cerr << failures << " close-voicing test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.2c Close-voicing tests passed.\n";
    return EXIT_SUCCESS;
}

#include "ChordModel.h"
#include "CloseVoicingHarmonizer.h"
#include "LiveReharmonizer.h"

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

void expectTransition(const ReharmonizationPlan& plan,
                      int voice,
                      int oldNote,
                      int newNote,
                      const std::string& label)
{
    const auto& transition = plan.voices[static_cast<std::size_t>(voice)];
    expect(transition.oldNote == oldNote, label + " old note");
    expect(transition.newNote == newNote, label + " new note");
    expect(transition.noteOff == (oldNote >= 0), label + " note off flag");
    expect(transition.noteOn == (newNote >= 0), label + " note on flag");
}

void expectNoTransition(const ReharmonizationPlan& plan,
                        int voice,
                        const std::string& label)
{
    const auto& transition = plan.voices[static_cast<std::size_t>(voice)];
    expect(! transition.noteOff && ! transition.noteOn, label + " no MIDI transition");
}

void testSameChordProducesNoTransition()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto current = buildCloseVoicing(67, cmaj7);
    const auto desired = buildCloseVoicing(67, cmaj7);
    const auto plan = planLowerVoiceReharmonization(current, desired);

    expect(! plan.lowerVoicesChanged, "same chord must not retrigger lower voices");
    for (int voice = 1; voice < 4; ++voice)
        expectNoTransition(plan, voice, "same chord voice");
}

void testChordChangeKeepsMelodyAndChangesLowerVoices()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto fmaj7 = normalizeChord(chord(-1, -1, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));

    const auto current = buildCloseVoicing(67, cmaj7); // G4 / E4 / C4 / B3
    const auto desired = buildCloseVoicing(67, fmaj7); // G4 / F4 / E4 / C4
    const auto plan = planLowerVoiceReharmonization(current, desired);

    expect(plan.lowerVoicesChanged, "Cmaj7 -> Fmaj7 must reharmonize lower voices");
    expectNoTransition(plan, 0, "V1 melody");
    expectTransition(plan, 1, 64, 65, "V2 Cmaj7 -> Fmaj7");
    expectTransition(plan, 2, 60, 64, "V3 Cmaj7 -> Fmaj7");
    expectTransition(plan, 3, 59, 60, "V4 Cmaj7 -> Fmaj7");
}

void testNoChordFallbackClearsOnlyLowerVoices()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    NormalizedChord noChord;

    const auto current = buildCloseVoicing(67, cmaj7);
    const auto desired = buildCloseVoicing(67, noChord);
    const auto plan = planLowerVoiceReharmonization(current, desired);

    expect(plan.lowerVoicesChanged, "no chord must clear generated harmony");
    expectNoTransition(plan, 0, "no chord V1 melody");
    expectTransition(plan, 1, 64, -1, "fallback V2 off");
    expectTransition(plan, 2, 60, -1, "fallback V3 off");
    expectTransition(plan, 3, 59, -1, "fallback V4 off");
}

void testChordReturnsAfterFallback()
{
    NormalizedChord noChord;
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));

    const auto current = buildCloseVoicing(67, noChord); // only G4
    const auto desired = buildCloseVoicing(67, g7);     // G4 / F4 / D4 / B3
    const auto plan = planLowerVoiceReharmonization(current, desired);

    expect(plan.lowerVoicesChanged, "valid chord after fallback must restore harmony");
    expectTransition(plan, 1, -1, 65, "restore V2");
    expectTransition(plan, 2, -1, 62, "restore V3");
    expectTransition(plan, 3, -1, 59, "restore V4");
}

void testSequenceCanBeAppliedWithoutStaleVoices()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto fmaj7 = normalizeChord(chord(-1, -1, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto dm7 = normalizeChord(chord(2, 2, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto g7 = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));

    auto current = buildCloseVoicing(67, cmaj7);
    const auto f = buildCloseVoicing(67, fmaj7);
    auto plan = planLowerVoiceReharmonization(current, f);
    expect(plan.lowerVoicesChanged, "sequence step Fmaj7 changes");
    current = f;

    const auto d = buildCloseVoicing(67, dm7);
    plan = planLowerVoiceReharmonization(current, d);
    expect(plan.lowerVoicesChanged, "sequence step Dm7 changes");
    current = d;

    const auto g = buildCloseVoicing(67, g7);
    plan = planLowerVoiceReharmonization(current, g);
    expect(plan.lowerVoicesChanged, "sequence step G7 changes");

    // Dm7 -> G7 keeps F4 and D4 as common tones in the same Voice slots.
    // Only V4 must move C4 -> B3; this is the first anti-retrigger rule for
    // live reharmonization, not yet the later voice-leading engine.
    expectNoTransition(plan, 1, "shared F4 stays sounding");
    expectNoTransition(plan, 2, "shared D4 stays sounding");
    expectTransition(plan, 3, 60, 59, "C4 -> B3 replacement");
}
}

int main()
{
    testSameChordProducesNoTransition();
    testChordChangeKeepsMelodyAndChangesLowerVoices();
    testNoChordFallbackClearsOnlyLowerVoices();
    testChordReturnsAfterFallback();
    testSequenceCanBeAppliedWithoutStaleVoices();

    if (failures != 0)
    {
        std::cerr << failures << " live-reharmonization test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.2d live-reharmonization tests passed.\n";
    return EXIT_SUCCESS;
}

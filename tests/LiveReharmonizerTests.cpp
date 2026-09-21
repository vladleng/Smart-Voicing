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

void testChordChangeKeepsMelodyAndCommonClosedVoices()
{
    const auto cmaj7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto fmaj7 = normalizeChord(chord(-1, -1, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));

    // 0.3b candidate-based Closed:
    // Cmaj7 + G4 -> G4 / E4 / C4 / B3
    // Fmaj7 + G4 -> G4 / E4 / C4 / A3
    // E4 and C4 are valid common tones and must not be retriggered just because
    // the chord changes. Only V4 moves B3 -> A3.
    const auto current = buildCloseVoicing(67, cmaj7);
    const auto desired = buildCloseVoicing(67, fmaj7);
    const auto plan = planLowerVoiceReharmonization(current, desired);

    expect(plan.lowerVoicesChanged, "Cmaj7 -> Fmaj7 must reharmonize changed lower voice");
    expectNoTransition(plan, 0, "V1 melody");
    expectNoTransition(plan, 1, "V2 common E4 remains sounding");
    expectNoTransition(plan, 2, "V3 common C4 remains sounding");
    expectTransition(plan, 3, 59, 57, "V4 B3 -> A3");
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

    expectNoTransition(plan, 1, "shared F4 stays sounding");
    expectNoTransition(plan, 2, "shared D4 stays sounding");
    expectTransition(plan, 3, 60, 59, "C4 -> B3 replacement");
}

void testSampleAccurateBoundaryScheduling()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSamples = 256;

    expect(sampleOffsetFromTimelineSeconds(10.0, 10.005, sampleRate, blockSamples) == 240,
           "5 ms timeline delta at 48 kHz must land at sample 240");

    expect(sampleOffsetFromPpq(4.0, 4.01, 120.0, sampleRate, blockSamples) == 240,
           "0.01 quarter at 120 BPM / 48 kHz must land at sample 240");

    expect(sampleOffsetFromTimelineSeconds(10.0,
                                           10.0 + static_cast<double>(blockSamples) / sampleRate,
                                           sampleRate,
                                           blockSamples) == -1,
           "event exactly at next block start must belong to next block");

    expect(sampleOffsetFromPpq(4.0, 3.99, 120.0, sampleRate, blockSamples) == -1,
           "past PPQ boundary must not be scheduled in current block");
}

void testMelodyGateImmediateReleaseWithoutSustain()
{
    MelodyGateState gate;
    gate.beginNote(67);
    expect(gate.ownsVoicing() && gate.keyDown(), "gate owns played melody");

    const auto decision = gate.endNote(67);
    expect(decision.releaseVoicing, "note off without sustain releases voicing");
    expect(! gate.ownsVoicing(), "released gate owns no melody");
}

void testMelodyGateSustainKeepsVoicingUntilPedalUp()
{
    MelodyGateState gate;
    gate.beginNote(67);
    expect(! gate.setSustain(true).releaseVoicing, "pedal down does not release");

    const auto noteOff = gate.endNote(67);
    expect(! noteOff.releaseVoicing, "note off under sustain keeps voicing");
    expect(gate.ownsVoicing() && ! gate.keyDown(), "sustain owns released melody");

    const auto pedalUp = gate.setSustain(false);
    expect(pedalUp.releaseVoicing, "pedal up releases sustain-owned melody");
    expect(! gate.ownsVoicing(), "pedal-up leaves no melody ownership");
}

void testMelodyGateNewNoteDuringSustainBecomesPhysicalOwner()
{
    MelodyGateState gate;
    gate.beginNote(67);
    gate.setSustain(true);
    gate.endNote(67);

    gate.beginNote(69);
    expect(gate.activeNote() == 69 && gate.keyDown(), "new note replaces sustain-owned melody");

    const auto pedalUp = gate.setSustain(false);
    expect(! pedalUp.releaseVoicing, "pedal up must not release physically held new note");
    expect(gate.ownsVoicing() && gate.keyDown(), "new physical melody remains owned");
}

void testMelodyGateIgnoresUnrelatedNoteOff()
{
    MelodyGateState gate;
    gate.beginNote(67);
    const auto decision = gate.endNote(65);
    expect(! decision.releaseVoicing, "unrelated note off does not release active melody");
    expect(gate.activeNote() == 67 && gate.keyDown(), "active melody survives unrelated note off");
}
}

int main()
{
    testSameChordProducesNoTransition();
    testChordChangeKeepsMelodyAndCommonClosedVoices();
    testNoChordFallbackClearsOnlyLowerVoices();
    testChordReturnsAfterFallback();
    testSequenceCanBeAppliedWithoutStaleVoices();
    testSampleAccurateBoundaryScheduling();
    testMelodyGateImmediateReleaseWithoutSustain();
    testMelodyGateSustainKeepsVoicingUntilPedalUp();
    testMelodyGateNewNoteDuringSustainBecomesPhysicalOwner();
    testMelodyGateIgnoresUnrelatedNoteOff();

    if (failures != 0)
    {
        std::cerr << failures << " live-reharmonization/integration test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.3b live-reharmonization/integration tests passed.\n";
    return EXIT_SUCCESS;
}

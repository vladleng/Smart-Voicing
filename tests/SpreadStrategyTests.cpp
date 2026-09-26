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

void expect(bool pass, const std::string& message)
{
    if (! pass)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

ChordContext chord(int rootFifths, int bassFifths,
                   std::initializer_list<std::pair<int, int>> tones)
{
    ChordContext result;
    result.available = result.defined = true;
    result.root = rootFifths;
    result.bass = bassFifths;
    for (const auto [semitones, degree] : tones)
        result.intervals.values[static_cast<std::size_t>(semitones)] =
            static_cast<std::uint8_t>(degree);
    return result;
}

KeyContext cMajor()
{
    KeyContext result;
    result.available = result.defined = true;
    for (const auto interval : { 0, 2, 4, 5, 7, 9, 11 })
        result.intervals.values[static_cast<std::size_t>(interval)] = 0xFFu;
    return result;
}

VoicingContext context(const NormalizedChord& current, int melody,
                       TensionLevel level = TensionLevel::clean,
                       const NormalizedChord* next = nullptr)
{
    VoicingContext result;
    result.chord = current;
    result.key = normalizeKey(cMajor());
    result.harmonic = next ? analyzeHarmonicFunction(current, result.key, *next)
                           : analyzeHarmonicFunction(current, result.key);
    result.tension = buildTensionPolicy(current, result.key, result.harmonic, melody);
    result.tensionLevel = level;
    return result;
}

int relative(int note, const NormalizedChord& chord)
{
    return ((note % 12) - chord.rootPitchClass + 12) % 12;
}

bool hasRelative(const VoiceOutput& output, const NormalizedChord& chord, int interval)
{
    for (const auto& voice : output.voices)
        if (voice.active && relative(voice.midiNote, chord) == interval)
            return true;
    return false;
}

void testIndependentBottomUpSpread()
{
    const auto major = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto ctx = context(major, 69);
    const auto spread = buildVoicing(69, VoicingType::spread, ctx);
    const auto closed = buildVoicing(69, VoicingType::closed, ctx);

    expect(static_cast<int>(VoicingType::spread) == 7, "Spread appends enum value 7");
    expect(std::string(voicingTypeName(VoicingType::spread)) == "Spread", "Spread name");
    expect(spread.voices[0].active && spread.voices[0].midiNote == 69,
           "V1 is performer melody");
    expect(spread.voices[3].active && relative(spread.voices[3].midiNote, major) == 0,
           "ordinary Spread has a root anchor");
    expect(spread.voices[0].midiNote - spread.voices[3].midiNote >= 17,
           "Spread vertical is open, unlike Closed");
    expect(spread.voices[0].midiNote - spread.voices[3].midiNote
               > closed.voices[0].midiNote - closed.voices[3].midiNote,
           "Spread has independent bottom-up shape");
    expect(hasRelative(spread, major, 4) && hasRelative(spread, major, 11),
           "third and seventh retain chord identity");
    for (int i = 1; i < 4; ++i)
        expect(spread.voices[i].active
                   && spread.voices[i - 1].midiNote > spread.voices[i].midiNote,
               "all four voices have strict descending sounding order");
}

void testSlashBassAndCharacteristicTone()
{
    const auto slash = normalizeChord(chord(0, 1, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto output = buildVoicing(72, VoicingType::spread, context(slash, 72));
    expect(slash.slashBass && output.voices[3].active
               && output.voices[3].midiNote % 12 == slash.bassPitchClass,
           "explicit slash bass remains lowest anchor");

    const auto halfDim = normalizeChord(chord(1, 1, {{0, 1}, {3, 3}, {6, 5}, {10, 7}}));
    const auto hd = buildVoicing(72, VoicingType::spread, context(halfDim, 72));
    expect(hasRelative(hd, halfDim, 6), "m7b5 characteristic flat fifth survives");
}

void testStage4PoolAndFallbacks()
{
    const auto dominant = normalizeChord(chord(1, 1, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    const auto minorTarget = normalizeChord(chord(-3, -3, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    const auto unresolved = buildVoicing(74, VoicingType::spread,
                                         context(dominant, 74, TensionLevel::rich));
    const auto directed = buildVoicing(74, VoicingType::spread,
                                       context(dominant, 74, TensionLevel::rich, &minorTarget));
    for (const auto* output : { &unresolved, &directed })
    {
        for (int i = 1; i < 3; ++i)
        {
            if (! output->voices[i].active)
                continue;
            const auto relativeTone = relative(output->voices[i].midiNote, dominant);
            const auto policy = context(dominant, 74, TensionLevel::rich,
                                        output == &directed ? &minorTarget : nullptr).tension;
            expect(dominant.hasTone(relativeTone)
                       || policy.isHarmonyCandidate(relativeTone, TensionLevel::rich),
                   "Spread inner voices use Stage 4 candidate vocabulary");
        }
    }

    VoicingContext absent;
    const auto noChord = buildVoicing(72, VoicingType::spread, absent);
    expect(noChord.voices[0].active && noChord.voices[0].midiNote == 72
               && ! noChord.voices[1].active, "no chord gives V1 only");
    const auto low = buildVoicing(12, VoicingType::spread, context(dominant, 12));
    expect(low.voices[0].midiNote == 12 && ! low.voices[3].active,
           "unsafe register gives V1 only without wrapping");
    const auto again = buildVoicing(74, VoicingType::spread,
                                    context(dominant, 74, TensionLevel::rich, &minorTarget));
    for (std::size_t i = 0; i < again.voices.size(); ++i)
        expect(again.voices[i].active == directed.voices[i].active
                   && again.voices[i].midiNote == directed.voices[i].midiNote,
               "same complete input yields deterministic output");
}
}

int main()
{
    testIndependentBottomUpSpread();
    testSlashBassAndCharacteristicTone();
    testStage4PoolAndFallbacks();
    if (failures)
        return EXIT_FAILURE;
    std::cout << "Spread strategy tests passed\n";
    return EXIT_SUCCESS;
}

#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"
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
void expect(bool good, const char* message)
{
    if (!good) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
ChordContext chord(int root, int bass, std::initializer_list<std::pair<int, int>> tones)
{
    ChordContext c;
    c.available = c.defined = true;
    c.root = root; c.bass = bass;
    for (const auto& [semitones, degree] : tones)
        c.intervals.values[static_cast<std::size_t>(semitones)] =
            static_cast<std::uint8_t>(degree);
    return c;
}
KeyContext cMajor()
{
    KeyContext k;
    k.available = k.defined = true;
    for (int i : {0, 2, 4, 5, 7, 9, 11})
        k.intervals.values[static_cast<std::size_t>(i)] = 0xFFu;
    return k;
}
VoicingContext context(const NormalizedChord& c, int melody,
                       TensionLevel level = TensionLevel::clean)
{
    VoicingContext v;
    v.chord = c; v.key = normalizeKey(cMajor());
    v.harmonic = analyzeHarmonicFunction(c, v.key);
    v.tension = buildTensionPolicy(c, v.key, v.harmonic, melody);
    v.tensionLevel = level;
    return v;
}
int degree(int note, const NormalizedChord& c)
{
    return ((note % 12) - c.rootPitchClass + 12) % 12;
}
bool has(const VoiceOutput& output, const NormalizedChord& c, int d)
{
    for (const auto& voice : output.voices)
        if (voice.active && degree(voice.midiNote, c) == d) return true;
    return false;
}
void testDenseUpperGroup()
{
    const auto major = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto cluster = buildVoicing(72, VoicingType::cluster, context(major, 72));
    expect(static_cast<int>(VoicingType::cluster) == 10, "append enum without renumbering");
    expect(std::string(voicingTypeName(VoicingType::cluster)) == "Cluster", "UI name");
    expect(cluster.voices[0].active && cluster.voices[0].midiNote == 72,
           "performed melody remains V1");
    for (int i = 1; i < 4; ++i)
        expect(cluster.voices[i].active && cluster.voices[i - 1].midiNote
                   > cluster.voices[i].midiNote, "complete descending vertical");
    expect(cluster.voices[0].midiNote - cluster.voices[2].midiNote <= 9,
           "upper group remains dense");
    for (int i = 1; i < 4; ++i)
        expect(major.hasTone(degree(cluster.voices[i].midiNote, major)),
               "Clean Cmaj7 uses only chord tones");
    const auto repeated = buildVoicing(72, VoicingType::cluster, context(major, 72));
    for (std::size_t i = 0; i < repeated.voices.size(); ++i)
        expect(repeated.voices[i].active == cluster.voices[i].active
               && repeated.voices[i].midiNote == cluster.voices[i].midiNote,
               "same context is deterministic");
}
void testAuthorityAndSafety()
{
    const auto slash = normalizeChord(chord(0, 4, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    const auto s = buildVoicing(76, VoicingType::cluster, context(slash, 76));
    expect(s.voices[3].active && s.voices[3].midiNote % 12 == slash.bassPitchClass,
           "slash bass owns V4");
    const auto halfDim = normalizeChord(chord(1, 1, {{0, 1}, {3, 3}, {6, 5}, {10, 7}}));
    const auto h = buildVoicing(72, VoicingType::cluster, context(halfDim, 72));
    expect(has(h, halfDim, 6), "half-diminished flat fifth remains audible");
    const auto sharp11 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {6, 11}, {7, 5}, {10, 7}}));
    const auto a = buildVoicing(74, VoicingType::cluster, context(sharp11, 74));
    expect(has(a, sharp11, 6), "explicit sharp eleventh remains audible");
    const auto plain = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}}));
    const auto rich = buildVoicing(76, VoicingType::cluster,
                                   context(plain, 76, TensionLevel::rich));
    for (int i = 1; i < 4; ++i)
        if (rich.voices[i].active)
            expect(plain.hasTone(degree(rich.voices[i].midiNote, plain)),
                   "plain triad does not invent harmonic extensions");
    VoicingContext absent;
    const auto noChord = buildVoicing(72, VoicingType::cluster, absent);
    expect(noChord.voices[0].active && !noChord.voices[1].active,
           "no context preserves V1 only");
    const auto low = buildVoicing(1, VoicingType::cluster, context(plain, 1));
    expect(low.voices[0].midiNote == 1 && !low.voices[3].active,
           "low MIDI falls back without underflow");
}
void testStage4Vocabulary()
{
    const auto dominant = normalizeChord(chord(1, 1,
        {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    for (auto level : {TensionLevel::clean, TensionLevel::color, TensionLevel::rich})
    {
        const auto ctx = context(dominant, 65, level);
        const auto output = buildVoicing(65, VoicingType::cluster, ctx);
        expect(output.voices[0].midiNote == 65, "melody stays exact");
        for (int i = 1; i < 3; ++i)
            if (output.voices[i].active)
            {
                const int d = degree(output.voices[i].midiNote, dominant);
                expect(dominant.hasTone(d) || ctx.tension.isHarmonyCandidate(d, level),
                       "Cluster does not bypass Stage 4 pool");
            }
    }
}
}
int main()
{
    testDenseUpperGroup(); testAuthorityAndSafety(); testStage4Vocabulary();
    if (failures) return EXIT_FAILURE;
    std::cout << "Cluster strategy tests passed\n";
    return EXIT_SUCCESS;
}

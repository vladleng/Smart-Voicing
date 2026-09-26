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
void expect(bool ok, const char* message)
{
    if (! ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
ChordContext chord(int root, int bass, std::initializer_list<std::pair<int,int>> tones)
{
    ChordContext c;
    c.available = c.defined = true;
    c.root = root; c.bass = bass;
    for (const auto [semitones, degree] : tones)
        c.intervals.values[static_cast<std::size_t>(semitones)] =
            static_cast<std::uint8_t>(degree);
    return c;
}
KeyContext cMajor()
{
    KeyContext k; k.available = k.defined = true;
    for (int i : { 0, 2, 4, 5, 7, 9, 11 })
        k.intervals.values[static_cast<std::size_t>(i)] = 0xFFu;
    return k;
}
VoicingContext context(const NormalizedChord& current, int melody,
                       TensionLevel level = TensionLevel::clean,
                       const NormalizedChord* next = nullptr)
{
    VoicingContext v;
    v.chord = current; v.key = normalizeKey(cMajor());
    v.harmonic = next ? analyzeHarmonicFunction(current, v.key, *next)
                      : analyzeHarmonicFunction(current, v.key);
    v.tension = buildTensionPolicy(current, v.key, v.harmonic, melody);
    v.tensionLevel = level;
    return v;
}
int relative(int note, const NormalizedChord& c)
{
    return ((note % 12) - c.rootPitchClass + 12) % 12;
}
bool has(const VoiceOutput& v, const NormalizedChord& c, int interval)
{
    for (const auto& voice : v.voices)
        if (voice.active && relative(voice.midiNote,c) == interval) return true;
    return false;
}
void testFourthStructure()
{
    // Explicit Cmaj13 allows D5-A4-E4-B3: three perfect fourths. Clean
    // quartal shapes must never require notes outside the Stage 4 pool.
    const auto c = normalizeChord(chord(0, 0, {{0,1},{2,9},{4,3},{7,5},{9,13},{11,7}}));
    const auto v = buildVoicing(74, VoicingType::quartal, context(c, 74, TensionLevel::rich));
    const auto closed = buildVoicing(74, VoicingType::closed, context(c, 74, TensionLevel::rich));
    expect(static_cast<int>(VoicingType::quartal) == 8, "Quartal appends enum 8");
    expect(std::string(voicingTypeName(VoicingType::quartal)) == "Quartal", "name");
    expect(v.voices[0].active && v.voices[0].midiNote == 74, "V1 exact melody");
    int fourths = 0;
    for (int i = 1; i < 4; ++i)
    {
        expect(v.voices[i].active && v.voices[i-1].midiNote > v.voices[i].midiNote,
               "four voices descend");
        const int gap = v.voices[i-1].midiNote - v.voices[i].midiNote;
        if (gap == 5 || gap == 6) ++fourths;
        expect(c.hasTone(relative(v.voices[i].midiNote,c))
                   || context(c,74,TensionLevel::rich).tension.isHarmonyCandidate(
                          relative(v.voices[i].midiNote,c),TensionLevel::rich),
               "Stage 4 vocabulary only");
    }
    expect(fourths >= 2, "quartal objective creates stacked fourths when feasible");
    expect(v.voices[3].midiNote != closed.voices[3].midiNote
               || v.voices[2].midiNote != closed.voices[2].midiNote,
           "Quartal is independent rather than a renamed Closed");
}
void testAuthorityAndSafety()
{
    const auto slash = normalizeChord(chord(0,1,{{0,1},{4,3},{7,5},{11,7}}));
    const auto sv = buildVoicing(72,VoicingType::quartal,context(slash,72));
    expect(sv.voices[3].active
               && sv.voices[3].midiNote % 12 == slash.bassPitchClass,
           "explicit slash bass is lowest");
    const auto halfDim = normalizeChord(chord(1,1,{{0,1},{3,3},{6,5},{10,7}}));
    const auto hd = buildVoicing(72,VoicingType::quartal,context(halfDim,72));
    expect(has(hd,halfDim,6), "half diminished b5 retained");
    const auto sharp11 = normalizeChord(chord(0,0,{{0,1},{4,3},{6,11},{7,5},{10,7}}));
    const auto altered = buildVoicing(74,VoicingType::quartal,context(sharp11,74));
    expect(has(altered,sharp11,6), "explicit #11 remains in the quartal vertical");
    const auto triad = normalizeChord(chord(0,0,{{0,1},{4,3},{7,5}}));
    const auto triadRich = buildVoicing(72,VoicingType::quartal,
                                        context(triad,72,TensionLevel::rich));
    for (int i=1; i<4; ++i)
        if (triadRich.voices[i].active)
            expect(triad.hasTone(relative(triadRich.voices[i].midiNote,triad)),
                   "plain triad does not acquire inferred extensions");
    VoicingContext absent;
    const auto noChord = buildVoicing(72,VoicingType::quartal,absent);
    expect(noChord.voices[0].active && ! noChord.voices[1].active,
           "no chord preserves melody only");
    const auto low = buildVoicing(1,VoicingType::quartal,context(halfDim,1));
    expect(low.voices[0].midiNote == 1 && ! low.voices[3].active,
           "low MIDI register never wraps");
    const auto again = buildVoicing(72,VoicingType::quartal,context(halfDim,72));
    for (std::size_t i=0; i<hd.voices.size(); ++i)
        expect(hd.voices[i].active == again.voices[i].active
                   && hd.voices[i].midiNote == again.voices[i].midiNote,
               "deterministic output");
}
void testTargetVocabulary()
{
    const auto dominant = normalizeChord(chord(1,1,{{0,1},{4,3},{7,5},{10,7}}));
    const auto target = normalizeChord(chord(-3,-3,{{0,1},{3,3},{7,5},{10,7}}));
    for (auto level : { TensionLevel::clean,TensionLevel::color,TensionLevel::rich })
    {
        const auto ctx = context(dominant,65,level,&target);
        const auto v = buildVoicing(65,VoicingType::quartal,ctx);
        expect(v.voices[0].midiNote == 65, "target case preserves melody");
        for (int i=1; i<4; ++i)
            if (v.voices[i].active)
            {
                const auto degree = relative(v.voices[i].midiNote,dominant);
                expect(dominant.hasTone(degree)
                           || ctx.tension.isHarmonyCandidate(degree,level),
                       "target colours remain Stage 4 controlled");
            }
    }
}
}
int main()
{
    testFourthStructure(); testAuthorityAndSafety(); testTargetVocabulary();
    if (failures) return EXIT_FAILURE;
    std::cout << "Quartal strategy tests passed\n";
    return EXIT_SUCCESS;
}

#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"
#include "TensionPolicy.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>

using namespace smartvoicing::harmony;

namespace
{
void expect(bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

KeyContext makeKey(std::int32_t rootFifths, bool minor)
{
    KeyContext key;
    key.available = true;
    key.defined = true;
    key.root = rootFifths;

    const int majorIntervals[] = { 0, 2, 4, 5, 7, 9, 11 };
    const int minorIntervals[] = { 0, 2, 3, 5, 7, 8, 10 };
    const auto* values = minor ? minorIntervals : majorIntervals;

    for (int i = 0; i < 7; ++i)
        key.intervals.values[static_cast<std::size_t>(values[i])] = 0xFFu;

    return key;
}

ChordContext makeChord(std::int32_t rootFifths,
                       std::initializer_list<int> relativeTones)
{
    ChordContext chord;
    chord.available = true;
    chord.defined = true;
    chord.root = rootFifths;
    chord.bass = rootFifths;

    for (const auto semitone : relativeTones)
        chord.intervals.values[static_cast<std::size_t>(semitone)] = 0xFFu;

    return chord;
}

ChordContext makeDegreeChord(std::int32_t rootFifths,
                             std::initializer_list<std::pair<int, int>> tones)
{
    ChordContext chord;
    chord.available = true;
    chord.defined = true;
    chord.root = rootFifths;
    chord.bass = rootFifths;

    for (const auto& [semitone, degree] : tones)
        chord.intervals.values[static_cast<std::size_t>(semitone)] =
            static_cast<std::uint8_t>(degree);

    return chord;
}
}

int main()
{
    const auto cMajor = normalizeKey(makeKey(0, false));
    const auto cMaj7 = normalizeChord(makeChord(0, { 0, 4, 7, 11 }));
    const auto cMaj7Analysis = analyzeHarmonicFunction(cMaj7, cMajor);
    const auto cMaj7Policy = buildTensionPolicy(cMaj7, cMajor, cMaj7Analysis);

    expect(cMaj7Policy.valid, "Cmaj7 tension policy is valid");
    expect(cMaj7Policy.tone(0).role == TensionRole::chordTone, "C root is structural chord tone");
    expect(cMaj7Policy.tone(4).role == TensionRole::chordTone, "E third is structural chord tone");
    expect(cMaj7Policy.tone(2).role == TensionRole::preferred, "D/9 is preferred on Cmaj7");
    expect(cMaj7Policy.tone(5).role == TensionRole::avoidAsHarmony, "F/11 is avoid-as-harmony above E on Cmaj7");
    expect(cMaj7Policy.tone(9).role == TensionRole::preferred, "A/13 is preferred on Cmaj7");
    expect(! cMaj7Policy.isHarmonyCandidate(5, TensionLevel::rich), "avoid 11 is not generated harmony candidate");

    // Tension Level changes inferred-colour eligibility without changing role.
    expect(! cMaj7Policy.isHarmonyCandidate(2, TensionLevel::clean),
           "Clean does not auto-generate inferred Cmaj7 9");
    expect(cMaj7Policy.isHarmonyCandidate(2, TensionLevel::color),
           "Color admits Preferred Cmaj7 9");
    expect(cMaj7Policy.isHarmonyCandidate(2, TensionLevel::rich),
           "Rich also admits Preferred Cmaj7 9");

    // Avoid-as-harmony never means forbidden melody. A performer-owned F remains
    // valid and is marked independently as melody-imposed.
    const auto cMaj7WithMelodyF = buildTensionPolicy(cMaj7, cMajor, cMaj7Analysis, 65); // F4
    expect(cMaj7WithMelodyF.tone(5).role == TensionRole::avoidAsHarmony,
           "F remains avoid-as-harmony classification");
    expect(cMaj7WithMelodyF.tone(5).melodyImposed,
           "F melody is preserved as melody-imposed despite avoid role");

    const auto dMin7 = normalizeChord(makeChord(2, { 0, 3, 7, 10 }));
    const auto dMinAnalysis = analyzeHarmonicFunction(dMin7, cMajor);
    const auto dMinPolicy = buildTensionPolicy(dMin7, cMajor, dMinAnalysis);
    expect(dMinPolicy.tone(2).role == TensionRole::preferred, "Dm7 9 is preferred");
    expect(dMinPolicy.tone(5).role == TensionRole::preferred, "Dm7 11 is preferred");
    expect(dMinPolicy.tone(9).role == TensionRole::preferred, "Dm7 13 is preferred in C major/Dorian context");

    const auto g7 = normalizeChord(makeChord(1, { 0, 4, 7, 10 }));
    const auto g7Analysis = analyzeHarmonicFunction(g7, cMajor);
    const auto g7Policy = buildTensionPolicy(g7, cMajor, g7Analysis);
    expect(g7Policy.tone(2).role == TensionRole::preferred, "G7 natural 9 is preferred");
    expect(g7Policy.tone(5).role == TensionRole::avoidAsHarmony, "G7 natural 11 is avoid above B");
    expect(g7Policy.tone(9).role == TensionRole::preferred, "G7 natural 13 is preferred");
    expect(g7Policy.tone(2).fromFunctionScale, "dominant baseline comes from function scale");

    // Rich expands the dominant candidate vocabulary, but altered colours remain
    // contextual rather than automatically preferred.
    expect(g7Policy.tone(1).role == TensionRole::contextual,
           "G7 b9 is contextual altered colour");
    expect(g7Policy.tone(1).alteredCandidate,
           "G7 b9 is marked altered candidate");
    expect(! g7Policy.isHarmonyCandidate(1, TensionLevel::clean),
           "Clean rejects inferred dominant b9");
    expect(! g7Policy.isHarmonyCandidate(1, TensionLevel::color),
           "Color keeps inferred dominant b9 conservative");
    expect(g7Policy.isHarmonyCandidate(1, TensionLevel::rich),
           "Rich admits context-supported dominant b9 candidate");

    // Applied dominant must not blindly inherit the global key. D7 in C uses a
    // Mixolydian dominant baseline: F# remains structural, while F-natural/#9
    // is reserved for Rich rather than inherited from the global key.
    const auto d7 = normalizeChord(makeChord(2, { 0, 4, 7, 10 }));
    const auto gMajor = normalizeChord(makeChord(1, { 0, 4, 7 }));
    const auto d7Analysis = analyzeHarmonicFunction(d7, cMajor, gMajor);
    const auto d7Policy = buildTensionPolicy(d7, cMajor, d7Analysis);
    expect(d7Analysis.appliedDominantConfirmed, "D7->G is confirmed V/V context");
    expect(d7Policy.tone(2).role == TensionRole::preferred, "D7 9 is preferred");
    expect(d7Policy.tone(5).role == TensionRole::avoidAsHarmony, "D7 11 is avoid above F#");
    expect(d7Policy.tone(9).role == TensionRole::preferred, "D7 13 is preferred");
    expect(d7Policy.tone(3).role == TensionRole::contextual && d7Policy.tone(3).alteredCandidate,
           "D7 #9-class pitch is reserved as Rich altered colour");
    expect(! d7Policy.isHarmonyCandidate(3, TensionLevel::color),
           "Color does not admit altered D7 #9 candidate");

    // Explicit tensions from Chord Track outrank level/inference and avoid heuristics.
    const auto cMaj7Sharp11 = normalizeChord(makeDegreeChord(0,
        { { 0, 1 }, { 4, 3 }, { 7, 5 }, { 11, 7 }, { 6, 11 } }));
    const auto cMaj7Sharp11Policy = buildTensionPolicy(
        cMaj7Sharp11, cMajor, analyzeHarmonicFunction(cMaj7Sharp11, cMajor));
    expect(cMaj7Sharp11Policy.tone(6).role == TensionRole::explicitTension,
           "explicit #11 remains Explicit even outside active key");
    expect(cMaj7Sharp11Policy.tone(6).explicitFromChord,
           "explicit #11 is marked as coming from Chord Track");
    expect(cMaj7Sharp11Policy.isHarmonyCandidate(6, TensionLevel::clean),
           "explicit #11 remains candidate even at Clean");

    const auto c7Flat9 = normalizeChord(makeDegreeChord(0,
        { { 0, 1 }, { 4, 3 }, { 7, 5 }, { 10, 7 }, { 1, 9 } }));
    const auto c7Flat9Policy = buildTensionPolicy(
        c7Flat9, cMajor, analyzeHarmonicFunction(c7Flat9, cMajor));
    expect(c7Flat9Policy.tone(1).role == TensionRole::explicitTension,
           "explicit b9 bypasses general half-step avoid heuristic");
    expect(c7Flat9Policy.isHarmonyCandidate(1, TensionLevel::clean),
           "explicit b9 survives Clean level");

    // Modal interchange inference uses the parallel source collection.
    const auto fMin7 = normalizeChord(makeChord(-1, { 0, 3, 7, 10 }));
    const auto fMinAnalysis = analyzeHarmonicFunction(fMin7, cMajor);
    const auto fMinPolicy = buildTensionPolicy(fMin7, cMajor, fMinAnalysis);
    expect(fMinAnalysis.modalInterchangeCandidate, "Fm7 is parallel-minor candidate");
    expect(fMinPolicy.tone(2).fromFunctionScale, "Fm7 9 derives from parallel-minor collection");
    expect(fMinPolicy.tone(2).role == TensionRole::preferred, "Fm7 9 remains preferred");

    KeyContext missingKeyContext;
    const auto missingKey = normalizeKey(missingKeyContext);
    const auto noKeyPolicy = buildTensionPolicy(cMaj7, missingKey, HarmonicAnalysis {});
    expect(noKeyPolicy.valid, "explicit chord alone still produces a valid policy shell");
    expect(noKeyPolicy.tone(2).role == TensionRole::unavailable,
           "without Key/Function no inferred 9 is invented on Cmaj7");

    HarmonicAnalysis dominantWithoutKey;
    dominantWithoutKey.valid = true;
    dominantWithoutKey.effectiveFunction = HarmonicFunction::dominant;
    const auto noKeyDominantPolicy = buildTensionPolicy(g7, missingKey, dominantWithoutKey);
    expect(noKeyDominantPolicy.tone(2).role == TensionRole::unavailable,
           "without Key context dominant function alone does not invent a Mixolydian 9");
    expect(noKeyDominantPolicy.tone(9).role == TensionRole::unavailable,
           "without Key context dominant function alone does not invent a Mixolydian 13");
    expect(noKeyDominantPolicy.tone(1).role == TensionRole::unavailable,
           "without Key context Rich altered candidates are not invented either");

    expect(std::string(tensionLevelName(TensionLevel::clean)) == "Clean", "Clean level name");
    expect(std::string(tensionLevelName(TensionLevel::color)) == "Color", "Color level name");
    expect(std::string(tensionLevelName(TensionLevel::rich)) == "Rich", "Rich level name");

    std::cout << "SmartVoicingTensionPolicyTests 0.3d: OK\n";
    return 0;
}

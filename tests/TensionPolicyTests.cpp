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
    expect(cMaj7Policy.functionalProfile == FunctionalTensionProfile::neutral,
           "Cmaj7 uses neutral functional tension profile");
    expect(cMaj7Policy.tone(0).role == TensionRole::chordTone, "C root is structural chord tone");
    expect(cMaj7Policy.tone(4).role == TensionRole::chordTone, "E third is structural chord tone");
    expect(cMaj7Policy.tone(2).role == TensionRole::preferred, "D/9 is preferred on Cmaj7");
    expect(cMaj7Policy.tone(5).role == TensionRole::avoidAsHarmony, "F/11 is avoid-as-harmony above E on Cmaj7");
    expect(cMaj7Policy.tone(9).role == TensionRole::preferred, "A/13 is preferred on Cmaj7");
    expect(! cMaj7Policy.isHarmonyCandidate(5, TensionLevel::rich), "avoid 11 is not generated harmony candidate");

    expect(! cMaj7Policy.isHarmonyCandidate(2, TensionLevel::clean),
           "Clean does not auto-generate inferred Cmaj7 9");
    expect(cMaj7Policy.isHarmonyCandidate(2, TensionLevel::color),
           "Color admits Preferred Cmaj7 9");
    expect(cMaj7Policy.isHarmonyCandidate(2, TensionLevel::rich),
           "Rich also admits Preferred Cmaj7 9");

    const auto cMaj7WithMelodyF = buildTensionPolicy(cMaj7, cMajor, cMaj7Analysis, 65);
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
    expect(g7Policy.functionalProfile == FunctionalTensionProfile::dominantUnresolved,
           "G7 without real next chord stays unresolved; target is not inferred from Key");
    expect(! g7Policy.resolutionConfirmed,
           "G7 without next chord has no confirmed resolution");
    expect(g7Policy.tone(2).role == TensionRole::preferred, "unresolved G7 natural 9 keeps generic Color status");
    expect(g7Policy.tone(5).role == TensionRole::avoidAsHarmony, "G7 natural 11 is avoid above B");
    expect(g7Policy.tone(9).role == TensionRole::preferred, "unresolved G7 natural 13 keeps generic Color status");
    expect(g7Policy.tone(2).fromFunctionScale, "dominant baseline comes from function profile");

    expect(g7Policy.tone(1).role == TensionRole::contextual,
           "G7 b9 is contextual altered colour");
    expect(g7Policy.tone(1).alteredCandidate,
           "G7 b9 is marked altered candidate");
    expect(! g7Policy.tone(1).functionallyDirected,
           "without actual next chord G7 b9 is not falsely marked directed");
    expect(! g7Policy.isHarmonyCandidate(1, TensionLevel::clean),
           "Clean rejects inferred dominant b9");
    expect(! g7Policy.isHarmonyCandidate(1, TensionLevel::color),
           "Color keeps unresolved dominant b9 conservative");
    expect(g7Policy.isHarmonyCandidate(1, TensionLevel::rich),
           "Rich may admit unresolved altered candidate but receives no directed reward");

    // Major-target resolution evidence: G7 -> Cmaj7.
    const auto g7ToCMajAnalysis = analyzeHarmonicFunction(g7, cMajor, cMaj7);
    const auto g7ToCMajPolicy = buildTensionPolicy(g7, cMajor, g7ToCMajAnalysis);
    expect(g7ToCMajAnalysis.dominantResolutionConfirmed,
           "G7->Cmaj7 confirms generic dominant resolution");
    expect(g7ToCMajAnalysis.dominantTargetQuality == ChordQuality::major,
           "G7->Cmaj7 preserves major target quality");
    expect(g7ToCMajPolicy.functionalProfile == FunctionalTensionProfile::dominantMajorTarget,
           "G7->Cmaj7 selects major-target tension profile");
    expect(g7ToCMajPolicy.resolutionConfirmed,
           "G7->Cmaj7 policy knows resolution is confirmed");
    expect(g7ToCMajPolicy.tone(1).functionallyDirected,
           "confirmed major-target G7 marks altered b9 as directed Rich candidate");

    const auto d7 = normalizeChord(makeChord(2, { 0, 4, 7, 10 }));
    const auto gMajor = normalizeChord(makeChord(1, { 0, 4, 7 }));
    const auto d7Analysis = analyzeHarmonicFunction(d7, cMajor, gMajor);
    const auto d7Policy = buildTensionPolicy(d7, cMajor, d7Analysis);
    expect(d7Analysis.appliedDominantConfirmed, "D7->G is confirmed V/V context");
    expect(d7Analysis.dominantResolutionConfirmed, "D7->G confirms generic dominant target too");
    expect(d7Policy.functionalProfile == FunctionalTensionProfile::dominantMajorTarget,
           "D7->G uses major-target functional profile");
    expect(d7Policy.tone(2).role == TensionRole::preferred, "D7 9 is preferred");
    expect(d7Policy.tone(5).role == TensionRole::avoidAsHarmony, "D7 11 is avoid above F#");
    expect(d7Policy.tone(9).role == TensionRole::preferred, "D7 13 is preferred");
    expect(d7Policy.tone(3).role == TensionRole::contextual && d7Policy.tone(3).alteredCandidate,
           "D7 #9-class pitch is reserved as Rich altered colour");
    expect(! d7Policy.isHarmonyCandidate(3, TensionLevel::color),
           "Color does not admit altered D7 #9 candidate");

    const auto aMin7InC = normalizeChord(makeChord(3, { 0, 3, 7, 10 }));
    const auto d7ToAmAnalysis = analyzeHarmonicFunction(d7, cMajor, aMin7InC);
    const auto d7ToAmPolicy = buildTensionPolicy(d7, cMajor, d7ToAmAnalysis);
    expect(! d7ToAmAnalysis.dominantResolutionConfirmed,
           "D7->Am is not treated as confirmed dominant resolution");
    expect(d7ToAmPolicy.functionalProfile == FunctionalTensionProfile::dominantUnresolved,
           "D7->Am stays unresolved rather than stealing Am target mode");

    // Minor-target dominant regression: Bm7b5 | E7 | Am.
    const auto aMinor = normalizeKey(makeKey(3, true));
    const auto e7 = normalizeChord(makeChord(4, { 0, 4, 7, 10 }));
    const auto aMin7 = normalizeChord(makeChord(3, { 0, 3, 7, 10 }));
    const auto e7ToAmAnalysis = analyzeHarmonicFunction(e7, aMinor, aMin7);
    const auto e7ToAmPolicy = buildTensionPolicy(e7, aMinor, e7ToAmAnalysis);

    expect(e7ToAmAnalysis.dominantResolutionConfirmed,
           "E7->Am confirms generic dominant resolution");
    expect(e7ToAmAnalysis.dominantTargetQuality == ChordQuality::minor,
           "E7->Am preserves minor target quality");
    expect(e7ToAmPolicy.functionalProfile == FunctionalTensionProfile::dominantMinorTarget,
           "E7->Am selects minor-target profile");
    expect(e7ToAmPolicy.resolutionConfirmed,
           "E7->Am policy carries confirmed target evidence");
    expect(e7ToAmPolicy.tone(1).role == TensionRole::contextual
           && e7ToAmPolicy.tone(1).alteredCandidate
           && e7ToAmPolicy.tone(1).functionallyDirected,
           "E7 b9 is a directed Rich candidate into Am");
    expect(e7ToAmPolicy.tone(8).role == TensionRole::preferred
           && e7ToAmPolicy.tone(8).alteredCandidate
           && e7ToAmPolicy.tone(8).functionallyDirected,
           "E7 b13 is target-aware inside Color for confirmed minor target");
    expect(e7ToAmPolicy.tone(3).role == TensionRole::contextual
           && e7ToAmPolicy.tone(3).alteredCandidate
           && ! e7ToAmPolicy.tone(3).functionallyDirected,
           "E7 #9 remains contextual and is not derived from minor-target evidence alone");
    expect(e7ToAmPolicy.tone(6).role == TensionRole::contextual
           && e7ToAmPolicy.tone(6).alteredCandidate
           && ! e7ToAmPolicy.tone(6).functionallyDirected,
           "E7 #11/b5 pitch class remains contextual and is not target-directed into Am");
    expect(e7ToAmPolicy.tone(9).role == TensionRole::unavailable,
           "E7 natural 13 C# is not inferred Color for confirmed A minor target");
    expect(! e7ToAmPolicy.isHarmonyCandidate(1, TensionLevel::color),
           "Color does not auto-add stronger E7 b9 tension");
    expect(e7ToAmPolicy.isHarmonyCandidate(1, TensionLevel::rich),
           "Rich may use functionally directed E7 b9");
    expect(e7ToAmPolicy.isHarmonyCandidate(8, TensionLevel::color),
           "Color may use functionally natural E7 b13 into Am");
    expect(e7ToAmPolicy.isHarmonyCandidate(8, TensionLevel::rich),
           "Rich keeps functionally natural E7 b13 available");

    // Explicit altered fifth remains authoritative and semantically distinct
    // from inferred b13/#11 tension roles.
    const auto e7Flat5 = normalizeChord(makeDegreeChord(4,
        { { 0, 1 }, { 4, 3 }, { 6, 5 }, { 10, 7 } }));
    const auto e7Flat5Policy = buildTensionPolicy(
        e7Flat5, aMinor, analyzeHarmonicFunction(e7Flat5, aMinor, aMin7));
    expect(e7Flat5Policy.tone(6).role == TensionRole::chordTone,
           "explicit E7b5 keeps b5 as an authoritative chord tone");
    expect(e7Flat5Policy.tone(6).explicitFromChord,
           "explicit E7b5 b5 is marked as coming from Chord Track");
    expect(e7Flat5Policy.isHarmonyCandidate(6, TensionLevel::clean),
           "explicit E7b5 b5 survives even at Clean");

    // 0.3f user regression: no next chord means no target guess. Adding the
    // explicit Dm target changes A7 from unresolved generic colour to a real
    // minor-target profile.
    const auto a7 = normalizeChord(makeChord(3, { 0, 4, 7, 10 }));
    const auto a7NoTarget = buildTensionPolicy(a7, cMajor,
        analyzeHarmonicFunction(a7, cMajor));
    expect(a7NoTarget.functionalProfile == FunctionalTensionProfile::dominantUnresolved,
           "A7 without following Dm does not invent a resolution target");
    expect(! a7NoTarget.resolutionConfirmed,
           "A7 without next chord remains unconfirmed");
    expect(a7NoTarget.tone(9).role == TensionRole::preferred,
           "unresolved A7 may retain generic natural-13 Color vocabulary");

    const auto a7ToDmAnalysis = analyzeHarmonicFunction(a7, cMajor, dMin7);
    const auto a7ToDmPolicy = buildTensionPolicy(a7, cMajor, a7ToDmAnalysis);
    expect(a7ToDmAnalysis.dominantResolutionConfirmed,
           "A7->Dm confirms real dominant target from Chord Track");
    expect(a7ToDmAnalysis.dominantTargetQuality == ChordQuality::minor,
           "A7->Dm preserves minor target quality");
    expect(a7ToDmPolicy.functionalProfile == FunctionalTensionProfile::dominantMinorTarget,
           "A7->Dm selects minor-target profile only when Dm is actually present");
    expect(a7ToDmPolicy.tone(8).role == TensionRole::preferred
           && a7ToDmPolicy.isHarmonyCandidate(8, TensionLevel::color),
           "A7 b13/F becomes functionally natural Color into Dm");
    expect(a7ToDmPolicy.tone(9).role == TensionRole::unavailable,
           "A7 natural 13/F# is not inferred Color once Dm target is confirmed");
    expect(! a7ToDmPolicy.isHarmonyCandidate(1, TensionLevel::color),
           "A7 b9 stays outside Color");
    expect(a7ToDmPolicy.isHarmonyCandidate(1, TensionLevel::rich),
           "A7 b9 becomes available to Rich with confirmed Dm target");
    expect(! a7ToDmPolicy.tone(3).functionallyDirected,
           "A7 #9 is not promoted merely because Dm is the minor target");
    expect(! a7ToDmPolicy.tone(6).functionallyDirected,
           "A7 #11/b5-class pitch is not promoted merely because Dm is the minor target");

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
    expect(noKeyDominantPolicy.functionalProfile == FunctionalTensionProfile::neutral,
           "without Key context no functional dominant profile is invented");
    expect(noKeyDominantPolicy.tone(2).role == TensionRole::unavailable,
           "without Key context dominant function alone does not invent a 9");
    expect(noKeyDominantPolicy.tone(9).role == TensionRole::unavailable,
           "without Key context dominant function alone does not invent a 13");
    expect(noKeyDominantPolicy.tone(1).role == TensionRole::unavailable,
           "without Key context Rich altered candidates are not invented either");

    expect(std::string(tensionLevelName(TensionLevel::clean)) == "Clean", "Clean level name");
    expect(std::string(tensionLevelName(TensionLevel::color)) == "Color", "Color level name");
    expect(std::string(tensionLevelName(TensionLevel::rich)) == "Rich", "Rich level name");
    expect(std::string(functionalTensionProfileName(FunctionalTensionProfile::dominantMinorTarget))
               == "Dominant -> minor target",
           "functional profile diagnostic name");

    std::cout << "SmartVoicingTensionPolicyTests 0.4a fix2: OK\n";
    return 0;
}

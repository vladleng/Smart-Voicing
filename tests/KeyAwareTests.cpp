#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>

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
}

int main()
{
    const auto cMajor = normalizeKey(makeKey(0, false));
    expect(cMajor.valid, "C major must normalize");
    expect(cMajor.mode == KeyMode::major, "C major mode detection");
    expect(cMajor.rootPitchClass == 0, "C root pitch class");
    expect(scaleDegreeForPitchClass(cMajor, 0) == 1, "C is I in C major");
    expect(scaleDegreeForPitchClass(cMajor, 2) == 2, "D is II in C major");
    expect(scaleDegreeForPitchClass(cMajor, 11) == 7, "B is VII in C major");
    expect(scaleDegreeForPitchClass(cMajor, 1) == 0, "C# is chromatic in C major");

    const auto aMinor = normalizeKey(makeKey(3, true)); // A = +3 fifths
    expect(aMinor.valid, "A minor must normalize");
    expect(aMinor.mode == KeyMode::minor, "A minor mode detection");
    expect(aMinor.rootPitchClass == 9, "A root pitch class");
    expect(scaleDegreeForPitchClass(aMinor, 9) == 1, "A is I in A minor");
    expect(scaleDegreeForPitchClass(aMinor, 0) == 3, "C is III in A minor");

    const auto cMaj7 = normalizeChord(makeChord(0, { 0, 4, 7, 11 }));
    const auto cAnalysis = analyzeHarmonicFunction(cMaj7, cMajor);
    expect(cAnalysis.valid, "Cmaj7/C analysis valid");
    expect(cAnalysis.rootScaleDegree == 1, "Cmaj7 is I in C major");
    expect(cAnalysis.rootFunction == HarmonicFunction::tonic, "I is tonic family");
    expect(cAnalysis.relation == HarmonicRelation::diatonic, "Cmaj7 is diatonic in C major");
    expect(! cAnalysis.appliedDominantCandidate, "Cmaj7 is not applied dominant");

    const auto dMin7 = normalizeChord(makeChord(2, { 0, 3, 7, 10 })); // D = +2 fifths
    const auto dMinAnalysis = analyzeHarmonicFunction(dMin7, cMajor);
    expect(dMinAnalysis.rootScaleDegree == 2, "Dm7 is II in C major");
    expect(dMinAnalysis.rootFunction == HarmonicFunction::predominant, "ii is predominant family");
    expect(dMinAnalysis.relation == HarmonicRelation::diatonic, "Dm7 is diatonic in C major");

    const auto g7 = normalizeChord(makeChord(1, { 0, 4, 7, 10 }));
    const auto gAnalysis = analyzeHarmonicFunction(g7, cMajor);
    expect(gAnalysis.rootScaleDegree == 5, "G7 is V in C major");
    expect(gAnalysis.effectiveFunction == HarmonicFunction::dominant, "V7 is dominant");
    expect(gAnalysis.relation == HarmonicRelation::diatonic, "G7 is diatonic in C major");
    expect(! gAnalysis.appliedDominantCandidate, "primary V is not labelled applied dominant");

    const auto d7 = normalizeChord(makeChord(2, { 0, 4, 7, 10 }));
    const auto d7InC = analyzeHarmonicFunction(d7, cMajor);
    expect(d7InC.rootScaleDegree == 2, "D7 root is II in C major");
    expect(d7InC.relation == HarmonicRelation::chromatic, "D7 contains F# and is chromatic in C major");
    expect(d7InC.appliedDominantCandidate, "D7 in C is applied-dominant candidate");
    expect(d7InC.appliedTargetScaleDegree == 5, "D7 targets scale degree V (G)");
    expect(d7InC.effectiveFunction == HarmonicFunction::dominant, "applied dominant candidate has dominant effective function");

    const auto a7 = normalizeChord(makeChord(3, { 0, 4, 7, 10 }));
    const auto a7InC = analyzeHarmonicFunction(a7, cMajor);
    expect(a7InC.appliedDominantCandidate, "A7 in C is applied-dominant candidate");
    expect(a7InC.appliedTargetScaleDegree == 2, "A7 targets scale degree II (D)");

    const auto gMajor = normalizeKey(makeKey(1, false));
    const auto d7InG = analyzeHarmonicFunction(d7, gMajor);
    expect(d7InG.rootScaleDegree == 5, "D7 is V in G major");
    expect(d7InG.relation == HarmonicRelation::diatonic, "D7 is diatonic in G major");
    expect(! d7InG.appliedDominantCandidate, "D7 in G is primary dominant, not applied");

    KeyContext missingKey;
    const auto noKey = normalizeKey(missingKey);
    expect(! noKey.valid, "missing Key must stay invalid");
    expect(! analyzeHarmonicFunction(d7, noKey).valid, "analysis safely falls back when Key is missing");

    std::cout << "SmartVoicingKeyAwareTests: OK\n";
    return 0;
}

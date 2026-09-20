#include "ChordModel.h"

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

ChordContext genericChord(std::initializer_list<int> tones)
{
    ChordContext result;
    result.available = true;
    result.defined = true;

    for (const auto semitones : tones)
        result.intervals.values[static_cast<std::size_t>(semitones)] = 0xFFu;

    return result;
}

void testCircleOfFifthsConversion()
{
    expect(circleOfFifthsToPitchClass(0) == 0, "C -> pitch class 0");
    expect(circleOfFifthsToPitchClass(1) == 7, "G -> pitch class 7");
    expect(circleOfFifthsToPitchClass(2) == 2, "D -> pitch class 2");
    expect(circleOfFifthsToPitchClass(-1) == 5, "F -> pitch class 5");
    expect(circleOfFifthsToPitchClass(-2) == 10, "Bb -> pitch class 10");
    expect(circleOfFifthsToPitchClass(7) == 1, "C# -> pitch class 1");
    expect(circleOfFifthsToPitchClass(-5) == 1, "Db -> pitch class 1");
}

void testBasicQualities()
{
    const auto major = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}}));
    expect(major.valid, "major is valid");
    expect(major.quality == ChordQuality::major, "major quality");
    expect(normalizedChordSymbol(major) == "C", "major symbol");

    const auto minor = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {7, 5}}));
    expect(minor.quality == ChordQuality::minor, "minor quality");
    expect(normalizedChordSymbol(minor) == "Cm", "minor symbol");

    const auto dominant = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {10, 7}}));
    expect(dominant.quality == ChordQuality::dominant, "dominant quality");
    expect(dominant.hasExtension(ChordExtension::minorSeventh), "dominant minor seventh flag");
    expect(normalizedChordSymbol(dominant) == "C7", "dominant symbol");

    const auto major7 = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {11, 7}}));
    expect(major7.quality == ChordQuality::major, "major7 keeps major family");
    expect(major7.hasExtension(ChordExtension::majorSeventh), "major seventh flag");
    expect(normalizedChordSymbol(major7) == "Cmaj7", "major7 symbol");

    const auto minor7 = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {7, 5}, {10, 7}}));
    expect(minor7.quality == ChordQuality::minor, "minor7 keeps minor family");
    expect(minor7.hasExtension(ChordExtension::minorSeventh), "minor7 seventh flag");
    expect(normalizedChordSymbol(minor7) == "Cm7", "minor7 symbol");
}

void testDiminishedAndSuspended()
{
    const auto diminished = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {6, 5}}));
    expect(diminished.quality == ChordQuality::diminished, "diminished quality");
    expect(diminished.hasAlteration(ChordAlteration::flatFifth), "diminished flat fifth");
    expect(normalizedChordSymbol(diminished) == "Cdim", "diminished symbol does not duplicate b5");

    const auto diminished7 = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {6, 5}, {9, 7}}));
    expect(diminished7.quality == ChordQuality::diminished, "diminished7 family");
    expect(normalizedChordSymbol(diminished7) == "Cdim7", "diminished7 symbol");

    const auto halfDiminished = normalizeChord(chord(0, 0, {{0, 1}, {3, 3}, {6, 5}, {10, 7}}));
    expect(halfDiminished.quality == ChordQuality::halfDiminished, "half-diminished quality");
    expect(normalizedChordSymbol(halfDiminished) == "Cm7b5", "half-diminished symbol");

    const auto sus2 = normalizeChord(chord(0, 0, {{0, 1}, {2, 2}, {7, 5}}));
    expect(sus2.quality == ChordQuality::suspended2, "sus2 quality");
    expect(normalizedChordSymbol(sus2) == "Csus2", "sus2 symbol");

    const auto sus4 = normalizeChord(chord(2, 2, {{0, 1}, {5, 4}, {7, 5}}));
    expect(sus4.quality == ChordQuality::suspended4, "sus4 quality");
    expect(normalizedChordSymbol(sus4) == "Dsus4", "sus4 symbol");

    const auto sevenSus4 = normalizeChord(chord(2, 2, {{0, 1}, {5, 4}, {7, 5}, {10, 7}}));
    expect(sevenSus4.quality == ChordQuality::suspended4, "7sus4 quality");
    expect(normalizedChordSymbol(sevenSus4) == "D7sus4", "7sus4 symbol");

    const auto augmented = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {8, 5}}));
    expect(augmented.quality == ChordQuality::augmented, "augmented quality");
    expect(augmented.hasAlteration(ChordAlteration::sharpFifth), "augmented sharp fifth");
    expect(normalizedChordSymbol(augmented) == "Caug", "augmented symbol does not duplicate #5");
}

void testExtensionsAndAlterations()
{
    const auto sixth = normalizeChord(chord(0, 0, {{0, 1}, {4, 3}, {7, 5}, {9, 6}}));
    expect(sixth.hasExtension(ChordExtension::sixth), "sixth extension");
    expect(normalizedChordSymbol(sixth) == "C6", "sixth symbol");

    const auto nine = normalizeChord(chord(0, 0,
        {{0, 1}, {2, 9}, {4, 3}, {7, 5}, {10, 7}}));
    expect(nine.quality == ChordQuality::dominant, "9 chord remains dominant");
    expect(normalizedChordSymbol(nine) == "C9", "9 chord symbol");

    const auto thirteen = normalizeChord(chord(0, 0,
        {{0, 1}, {2, 9}, {4, 3}, {7, 5}, {9, 13}, {10, 7}}));
    expect(thirteen.quality == ChordQuality::dominant, "13 chord remains dominant");
    expect(thirteen.hasExtension(ChordExtension::ninth), "13 chord has ninth");
    expect(thirteen.hasExtension(ChordExtension::thirteenth), "13 chord has thirteenth");
    expect(normalizedChordSymbol(thirteen) == "C13", "13 chord symbol");

    const auto flat9 = normalizeChord(chord(0, 0,
        {{0, 1}, {1, 9}, {4, 3}, {7, 5}, {10, 7}}));
    expect(flat9.hasAlteration(ChordAlteration::flatNinth), "b9 detected from degree annotation");
    expect(normalizedChordSymbol(flat9) == "C7b9", "b9 symbol");

    const auto sharp9 = normalizeChord(chord(0, 0,
        {{0, 1}, {3, 9}, {4, 3}, {7, 5}, {10, 7}}));
    expect(sharp9.quality == ChordQuality::dominant, "#9 does not become minor third");
    expect(sharp9.hasAlteration(ChordAlteration::sharpNinth), "#9 detected from degree annotation");
    expect(normalizedChordSymbol(sharp9) == "C7#9", "#9 symbol");

    const auto flat5 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {6, 5}, {10, 7}}));
    expect(flat5.quality == ChordQuality::dominant, "7b5 remains dominant");
    expect(flat5.hasAlteration(ChordAlteration::flatFifth), "b5 detected");
    expect(normalizedChordSymbol(flat5) == "C7b5", "b5 symbol");

    const auto sharp5 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {8, 5}, {10, 7}}));
    expect(sharp5.quality == ChordQuality::dominant, "7#5 remains dominant");
    expect(sharp5.hasAlteration(ChordAlteration::sharpFifth), "#5 detected");
    expect(normalizedChordSymbol(sharp5) == "C7#5", "#5 symbol");

    const auto sharp11 = normalizeChord(chord(2, 2,
        {{0, 1}, {4, 3}, {6, 11}, {7, 5}, {10, 7}}));
    expect(sharp11.hasExtension(ChordExtension::eleventh), "#11 is an eleventh extension");
    expect(sharp11.hasAlteration(ChordAlteration::sharpEleventh), "#11 detected without becoming b5");
    expect(! sharp11.hasAlteration(ChordAlteration::flatFifth), "#11 is not misclassified as b5");
    expect(normalizedChordSymbol(sharp11) == "D7#11", "#11 symbol keeps dominant 7 base");

    const auto flat13 = normalizeChord(chord(0, 0,
        {{0, 1}, {4, 3}, {7, 5}, {8, 13}, {10, 7}}));
    expect(flat13.hasExtension(ChordExtension::thirteenth), "b13 is a thirteenth extension");
    expect(flat13.hasAlteration(ChordAlteration::flatThirteenth), "b13 detected without becoming #5");
    expect(! flat13.hasAlteration(ChordAlteration::sharpFifth), "b13 is not misclassified as #5");
    expect(normalizedChordSymbol(flat13) == "C7b13", "b13 symbol keeps dominant 7 base");
}

void testSlashBassAndGenericUsage()
{
    const auto slash = normalizeChord(chord(3, 5, {{0, 1}, {4, 3}, {7, 5}})); // A/B
    expect(slash.slashBass, "A/B slash bass detected");
    expect(slash.rootPitchClass == 9, "A/B root pitch class A");
    expect(slash.bassPitchClass == 11, "A/B bass pitch class B");
    expect(slash.rootFifths == 3 && slash.bassFifths == 5, "A/B spelling retained");
    expect(normalizedChordSymbol(slash) == "A/B", "slash chord symbol");

    const auto genericMajor = normalizeChord(genericChord({0, 4, 7}));
    expect(genericMajor.quality == ChordQuality::major, "generic used mask still classifies major");

    const auto genericMinor = normalizeChord(genericChord({0, 3, 7}));
    expect(genericMinor.quality == ChordQuality::minor, "generic used mask still classifies minor");
}

void testUndefinedChord()
{
    ChordContext source;
    source.available = true;
    source.defined = false;
    const auto normalized = normalizeChord(source);
    expect(! normalized.valid, "undefined chord is not valid");
    expect(normalized.quality == ChordQuality::undefined, "undefined quality");
    expect(normalizedChordSymbol(normalized) == "(no chord)", "undefined symbol");
}
}

int main()
{
    testCircleOfFifthsConversion();
    testBasicQualities();
    testDiminishedAndSuspended();
    testExtensionsAndAlterations();
    testSlashBassAndGenericUsage();
    testUndefinedChord();

    if (failures != 0)
    {
        std::cerr << failures << " chord-model test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Smart Voicing 0.2b chord-model tests passed.\n";
    return EXIT_SUCCESS;
}

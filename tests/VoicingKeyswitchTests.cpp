#include "TensionKeyswitch.h"
#include "VoicingKeyswitch.h"

#include <cstdlib>
#include <iostream>
#include <string>

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

void expectMapping(int note, VoicingType expected, const std::string& label)
{
    VoicingType actual = VoicingType::closed;
    const auto decoded = voicingTypeFromKeyswitch(note, actual);
    expect(decoded, label + " must decode");
    expect(actual == expected, label + " must select expected VoicingType");
    expect(isVoicingTypeKeyswitch(note), label + " must belong to reserved control block");
    expect(! isReservedVoicingTypeKeyswitch(note), label + " must not be marked reserved");
}

void expectReserved(int note, const std::string& label)
{
    VoicingType value = VoicingType::doubling;
    expect(isVoicingTypeKeyswitch(note), label + " must stay inside control block");
    expect(isReservedVoicingTypeKeyswitch(note), label + " must be marked reserved");
    expect(! voicingTypeFromKeyswitch(note, value), label + " must not decode yet");
    expect(value == VoicingType::doubling, label + " must not mutate current VoicingType state");
}

void testImplementedMap()
{
    expectMapping(36, VoicingType::closed, "MIDI 36 / C1 Closed");
    expectMapping(37, VoicingType::drop2, "MIDI 37 / C#1 Drop 2");
    expectMapping(38, VoicingType::drop3, "MIDI 38 / D1 Drop 3");
    expectMapping(39, VoicingType::drop24, "MIDI 39 / D#1 Drop 2+4");
    expectMapping(40, VoicingType::unison, "MIDI 40 / E1 Unison");
    expectMapping(41, VoicingType::octaves, "MIDI 41 / F1 Octaves");
    expectMapping(42, VoicingType::doubling, "MIDI 42 / F#1 Doubling");
}

void testFutureSlotsAreReservedButNotDecoded()
{
    expectReserved(32, "MIDI 32 / G#0 UST reserved");
    expectReserved(33, "MIDI 33 / A0 Cluster reserved");
    expectReserved(34, "MIDI 34 / A#0 Quartal reserved");
    expectReserved(35, "MIDI 35 / B0 Spread reserved");
}

void testBlockBoundariesAndTensionSeparation()
{
    expect(! isVoicingTypeKeyswitch(31), "MIDI 31 is below Voicing Type block");
    expect(! isVoicingTypeKeyswitch(43), "MIDI 43 is outside Voicing Type block");
    expect(! isVoicingTypeKeyswitch(127), "MIDI 127 is outside Voicing Type block");

    expect(kLastVoicingKeyswitchNote + 1 == kCleanTensionKeyswitchNote,
           "Voicing Type block must end immediately before stable Tension block");
    expect(kClosedVoicingKeyswitchNote == 36,
           "frequently used Voicing block must start at MIDI 36 / C1");
    expect(kDrop2VoicingKeyswitchNote == 37
           && kDrop3VoicingKeyswitchNote == 38
           && kDrop24VoicingKeyswitchNote == 39,
           "Drop family must follow Closed chromatically at C#1/D1/D#1");
    expect(kUnisonVoicingKeyswitchNote == 40
           && kOctavesVoicingKeyswitchNote == 41
           && kDoublingVoicingKeyswitchNote == 42,
           "melodic textures must occupy E1/F1/F#1 immediately below Tension block");

    expect(! isVoicingTypeKeyswitch(kCleanTensionKeyswitchNote),
           "Clean Tension keyswitch must not collide with Voicing Type block");
    expect(! isVoicingTypeKeyswitch(kColorTensionKeyswitchNote),
           "Color Tension keyswitch must not collide with Voicing Type block");
    expect(! isVoicingTypeKeyswitch(kRichTensionKeyswitchNote),
           "Rich Tension keyswitch must not collide with Voicing Type block");
}
}

int main()
{
    testImplementedMap();
    testFutureSlotsAreReservedButNotDecoded();
    testBlockBoundariesAndTensionSeparation();

    if (failures != 0)
    {
        std::cerr << failures << " Voicing keyswitch test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Voicing keyswitch tests passed\n";
    return EXIT_SUCCESS;
}

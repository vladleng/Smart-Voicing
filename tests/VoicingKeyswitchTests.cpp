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
}

void testImplementedMap()
{
    expectMapping(32, VoicingType::closed, "MIDI 32 Closed");
    expectMapping(33, VoicingType::drop2, "MIDI 33 Drop 2");
    expectMapping(40, VoicingType::unison, "MIDI 40 Unison");
    expectMapping(41, VoicingType::octaves, "MIDI 41 Octaves");
    expectMapping(42, VoicingType::doubling, "MIDI 42 Doubling");
}

void testFutureSlotsAreReservedButNotDecoded()
{
    for (int note = 34; note <= 39; ++note)
    {
        VoicingType value = VoicingType::doubling;
        expect(isVoicingTypeKeyswitch(note),
               "future MIDI note " + std::to_string(note) + " must stay inside control block");
        expect(isReservedVoicingTypeKeyswitch(note),
               "future MIDI note " + std::to_string(note) + " must be marked reserved");
        expect(! voicingTypeFromKeyswitch(note, value),
               "future MIDI note " + std::to_string(note) + " must not select an unimplemented type");
        expect(value == VoicingType::doubling,
               "reserved decoder must not mutate existing VoicingType state");
    }
}

void testBlockBoundariesAndTensionSeparation()
{
    expect(! isVoicingTypeKeyswitch(31), "MIDI 31 is below Voicing Type block");
    expect(! isVoicingTypeKeyswitch(43), "MIDI 43 is outside Voicing Type block");
    expect(! isVoicingTypeKeyswitch(127), "MIDI 127 is outside Voicing Type block");

    expect(kLastVoicingKeyswitchNote + 1 == kCleanTensionKeyswitchNote,
           "Voicing Type block must end immediately before stable Tension block");
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

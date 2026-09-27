#include "HarmonyModeKeyswitch.h"
#include "TensionKeyswitch.h"

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

void testExactModeMap()
{
    int value = -1;
    expect(harmonyModeValueFromKeyswitch(46, value), "MIDI 46 must decode");
    expect(value == 0, "MIDI 46 must select Direct Router / mode 0");
    expect(isHarmonyModeKeyswitch(46), "MIDI 46 must belong to Harmony Mode block");

    value = -1;
    expect(harmonyModeValueFromKeyswitch(47, value), "MIDI 47 must decode");
    expect(value == 1, "MIDI 47 must select Melody Harmonize / mode 1");
    expect(isHarmonyModeKeyswitch(47), "MIDI 47 must belong to Harmony Mode block");
}

void testBoundariesAndNoMutation()
{
    expect(! isHarmonyModeKeyswitch(45), "MIDI 45 must remain outside Harmony Mode block");
    expect(! isHarmonyModeKeyswitch(48), "MIDI 48 must remain outside Harmony Mode block");

    int value = 1;
    expect(! harmonyModeValueFromKeyswitch(45, value), "MIDI 45 must not decode as a mode");
    expect(value == 1, "invalid decoder input must not mutate mode state");
}

void testTensionAdjacency()
{
    expect(kRichTensionKeyswitchNote + 1 == kDirectRouterModeKeyswitchNote,
           "Harmony Mode block must start immediately above stable Tension block");
    expect(kMelodyHarmonizeModeKeyswitchNote == kDirectRouterModeKeyswitchNote + 1,
           "Direct Router and Melody Harmonize must be adjacent keys");
}
}

int main()
{
    testExactModeMap();
    testBoundariesAndNoMutation();
    testTensionAdjacency();

    if (failures != 0)
    {
        std::cerr << failures << " Harmony Mode keyswitch test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Harmony Mode keyswitch tests passed\n";
    return EXIT_SUCCESS;
}

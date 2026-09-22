#include "TensionKeyswitch.h"

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

void testStableMidiMap()
{
    expect(kCleanTensionKeyswitchNote == 43, "Clean keyswitch is MIDI 43");
    expect(kColorTensionKeyswitchNote == 44, "Color keyswitch is MIDI 44");
    expect(kRichTensionKeyswitchNote == 45, "Rich keyswitch is MIDI 45");
}

void testDecodeAllThreeLevels()
{
    auto level = TensionLevel::rich;

    expect(tensionLevelFromKeyswitch(kCleanTensionKeyswitchNote, level),
           "Clean note is recognized");
    expect(level == TensionLevel::clean, "Clean note selects shared Clean state");

    expect(tensionLevelFromKeyswitch(kColorTensionKeyswitchNote, level),
           "Color note is recognized");
    expect(level == TensionLevel::color, "Color note selects shared Color state");

    expect(tensionLevelFromKeyswitch(kRichTensionKeyswitchNote, level),
           "Rich note is recognized");
    expect(level == TensionLevel::rich, "Rich note selects shared Rich state");
}

void testUnrelatedNotesAreIgnoredWithoutChangingState()
{
    auto level = TensionLevel::color;
    expect(! isTensionLevelKeyswitch(42), "MIDI 42 is not a tension keyswitch");
    expect(! isTensionLevelKeyswitch(46), "MIDI 46 is not a tension keyswitch");
    expect(! tensionLevelFromKeyswitch(60, level), "MIDI 60 is ignored");
    expect(level == TensionLevel::color, "Ignored note does not change tension state");
}
}

int main()
{
    testStableMidiMap();
    testDecodeAllThreeLevels();
    testUnrelatedNotesAreIgnoredWithoutChangingState();

    if (failures != 0)
    {
        std::cerr << failures << " TensionKeyswitch test(s) failed\n";
        return 1;
    }

    std::cout << "TensionKeyswitch tests passed\n";
    return 0;
}

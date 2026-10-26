#include "VoiceLeading.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace smartvoicing::harmony;

namespace
{
int failures = 0;

void expect(bool condition, const std::string& label)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}

VoiceOutput vertical(int v1, int v2, int v3, int v4)
{
    VoiceOutput result;
    result.clear();
    const int notes[] { v1, v2, v3, v4 };
    for (int i = 0; i < kVoiceCount; ++i)
    {
        if (notes[i] >= 0)
            result.voices[static_cast<std::size_t>(i)] = { true, notes[i] };
    }
    return result;
}

void testHistoryAndReset()
{
    VoiceLeadingState state;
    expect(! state.hasPrevious(), "new state has no history");

    state.commit(vertical(72, 67, 64, 60));
    expect(state.hasPrevious(), "four-voice state committed");
    expect(state.previous().voices[2].midiNote == 64, "voice slots remembered");

    state.commit(vertical(72, -1, -1, -1));
    expect(! state.hasPrevious(), "V1-only input clears stale harmonic history");

    state.commit(vertical(72, 67, 64, 60));
    state.reset();
    expect(! state.hasPrevious(), "reset clears history");
}

void testSmoothnessPreferences()
{
    VoiceLeadingState state;
    state.commit(vertical(72, 67, 64, 60));

    const auto common = evaluateVoiceLeading(vertical(74, 67, 64, 60), state);
    const auto step = evaluateVoiceLeading(vertical(74, 69, 64, 60), state);
    const auto leap = evaluateVoiceLeading(vertical(74, 69, 60, 48), state);

    expect(common.valid && common.cost == 0, "V1 motion never penalized");
    expect(common.commonTones == 3, "three stable inner voices rewarded");
    expect(step.cost > common.cost && step.stepwiseMoves == 1, "step over common tone");
    expect(leap.cost > step.cost && leap.largeLeaps >= 1, "large leap penalized");
}

void testIdentityAndCrossings()
{
    VoiceLeadingState state;
    state.commit(vertical(76, 71, 67, 64));
    const auto good = evaluateVoiceLeading(vertical(77, 71, 67, 64), state);
    const auto reassigned = evaluateVoiceLeading(vertical(77, 67, 71, 64), state);

    expect(good.cost < reassigned.cost, "lower voices are not interchangeable");
    expect(reassigned.crossings == 1, "voice crossing explicitly reported");

    const auto incomplete = evaluateVoiceLeading(vertical(77, 71, -1, 64), state);
    expect(incomplete.valid, "incomplete vertical is scoreable");
    expect(incomplete.cost > good.cost, "unnecessary dropped line costs movement");
}

void testBoundedRealtimeRanking()
{
    VoiceLeadingState state;
    auto first = vertical(74, 69, 65, 60);
    state.commit(first);
    expect(voiceLeadingAdjustment(first, &state) == 0,
           "retained V2/V3/V4 receive no continuity penalty");
    const auto nearby = vertical(76, 70, 65, 60);
    const auto leap = vertical(76, 62, 53, 48);
    const auto stepPenalty = voiceLeadingAdjustment(nearby, &state);
    const auto leapPenalty = voiceLeadingAdjustment(leap, &state);
    expect(stepPenalty > 0 && stepPenalty < leapPenalty,
           "bounded tie-break favors stepwise motion to large leaps");
    expect(leapPenalty == 10, "continuity tie-break is capped at ten");
    expect(voiceLeadingAdjustment(leap, nullptr) == 0,
           "legacy strategy with no previous voice has no penalty");

    VoiceLeadingState empty;
    expect(voiceLeadingAdjustment(leap, &empty) == 0,
           "empty history retains exact Stage 5 candidate rank");

    // The array path is used inside the strategy search loops.
    const std::array<int, kVoiceCount> notes { 76, 70, 65, 60 };
    expect(voiceLeadingAdjustment(notes, &state) == stepPenalty,
           "array and VoiceOutput scoring agree");
}

void testNoHistoryAndInputSafety()
{
    VoiceLeadingState state;
    const auto initial = evaluateVoiceLeading(vertical(69, 69, 69, 69), state);
    expect(initial.valid && initial.cost == 0, "equal notes are legal for unison texture");

    auto invalid = vertical(72, 67, 64, 60);
    invalid.voices[3].midiNote = 128;
    expect(! evaluateVoiceLeading(invalid, state).valid, "invalid MIDI rejected");
    state.commit(invalid);
    expect(! state.hasPrevious(), "invalid MIDI cannot enter history");

    auto empty = vertical(-1, -1, -1, -1);
    expect(! evaluateVoiceLeading(empty, state).valid, "no V1 rejected");
}
}

int main()
{
    testHistoryAndReset();
    testSmoothnessPreferences();
    testIdentityAndCrossings();
    testNoHistoryAndInputSafety();
    testBoundedRealtimeRanking();
    if (failures != 0)
    {
        std::cerr << failures << " Voice Leading test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Voice Leading 0.5a core tests passed\n";
    return EXIT_SUCCESS;
}

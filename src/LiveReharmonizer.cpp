#include "LiveReharmonizer.h"

#include <cmath>

namespace smartvoicing::harmony
{
namespace
{
int slotNote(const VoiceSlot& slot) noexcept
{
    return slot.active ? slot.midiNote : -1;
}

int roundedSampleOffset(double exactSamples, int blockSamples) noexcept
{
    if (! std::isfinite(exactSamples) || blockSamples <= 0)
        return -1;

    const auto offset = static_cast<long long>(std::llround(exactSamples));
    if (offset < 0 || offset >= blockSamples)
        return -1;

    return static_cast<int>(offset);
}
}

ReharmonizationPlan planLowerVoiceReharmonization(const VoiceOutput& current,
                                                  const VoiceOutput& desired) noexcept
{
    ReharmonizationPlan plan;

    // V1 belongs to the played melody and must never be rewritten by a chord
    // transition. Only generated lower voices participate in reharmonization.
    for (int voice = 1; voice < kVoiceCount; ++voice)
    {
        const auto index = static_cast<std::size_t>(voice);
        const auto oldNote = slotNote(current.voices[index]);
        const auto newNote = slotNote(desired.voices[index]);

        if (oldNote == newNote)
            continue;

        auto& transition = plan.voices[index];
        transition.oldNote = oldNote;
        transition.newNote = newNote;
        transition.noteOff = oldNote >= 0;
        transition.noteOn = newNote >= 0;
        plan.lowerVoicesChanged = true;
    }

    return plan;
}

int sampleOffsetFromTimelineSeconds(double blockStartSeconds,
                                    double eventSeconds,
                                    double sampleRate,
                                    int blockSamples) noexcept
{
    if (blockStartSeconds < 0.0 || eventSeconds < 0.0 || sampleRate <= 0.0)
        return -1;

    return roundedSampleOffset((eventSeconds - blockStartSeconds) * sampleRate,
                               blockSamples);
}

int sampleOffsetFromPpq(double blockStartPpq,
                        double eventPpq,
                        double bpm,
                        double sampleRate,
                        int blockSamples) noexcept
{
    if (blockStartPpq < 0.0 || eventPpq < 0.0 || bpm <= 0.0 || sampleRate <= 0.0)
        return -1;

    const auto secondsPerQuarter = 60.0 / bpm;
    return roundedSampleOffset((eventPpq - blockStartPpq) * secondsPerQuarter * sampleRate,
                               blockSamples);
}
}

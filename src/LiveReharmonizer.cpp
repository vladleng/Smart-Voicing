#include "LiveReharmonizer.h"

namespace smartvoicing::harmony
{
namespace
{
int slotNote(const VoiceSlot& slot) noexcept
{
    return slot.active ? slot.midiNote : -1;
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
}

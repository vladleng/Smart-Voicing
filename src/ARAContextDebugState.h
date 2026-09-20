#pragma once

#include <JuceHeader.h>
#include "SharedHarmonicContext.h"

#include <mutex>
#include <unordered_map>

struct ARAContextDebugSnapshot
{
    bool documentControllerCreated = false;
    bool hostContentAccessAvailable = false;
    int musicalContextCount = 0;

    bool keySignaturesAvailable = false;
    int keySignatureEventCount = 0;

    bool sheetChordsAvailable = false;
    int sheetChordEventCount = 0;

    bool tempoEntriesAvailable = false;
    int tempoEntryEventCount = 0;

    bool barSignaturesAvailable = false;
    int barSignatureEventCount = 0;

    int registeredControllerCount = 0;
    bool sharedContextAvailable = false;

    SharedHarmonicContextSnapshot harmonicContext;
};

class ARAContextDebugState final
{
public:
    static ARAContextDebugState& instance()
    {
        static ARAContextDebugState state;
        return state;
    }

    void publishSnapshot(const void* source, const ARAContextDebugSnapshot& newSnapshot)
    {
        const std::scoped_lock lock(mutex);
        snapshots[source] = newSnapshot;
    }

    void removeSource(const void* source)
    {
        const std::scoped_lock lock(mutex);
        snapshots.erase(source);
    }

    ARAContextDebugSnapshot getSnapshot() const
    {
        const std::scoped_lock lock(mutex);

        ARAContextDebugSnapshot best;
        best.registeredControllerCount = static_cast<int>(snapshots.size());

        int bestScore = -1;
        for (const auto& [source, candidate] : snapshots)
        {
            juce::ignoreUnused(source);

            const auto contentTypes = static_cast<int>(candidate.keySignaturesAvailable)
                                    + static_cast<int>(candidate.sheetChordsAvailable)
                                    + static_cast<int>(candidate.tempoEntriesAvailable)
                                    + static_cast<int>(candidate.barSignaturesAvailable);

            const auto eventCount = candidate.keySignatureEventCount
                                  + candidate.sheetChordEventCount
                                  + candidate.tempoEntryEventCount
                                  + candidate.barSignatureEventCount;

            const auto score = candidate.musicalContextCount * 10000
                             + contentTypes * 1000
                             + eventCount * 10
                             + static_cast<int>(candidate.hostContentAccessAvailable);

            if (score > bestScore)
            {
                best = candidate;
                bestScore = score;
            }
        }

        best.registeredControllerCount = static_cast<int>(snapshots.size());
        best.sharedContextAvailable = best.musicalContextCount > 0
                                   && (best.keySignaturesAvailable
                                       || best.sheetChordsAvailable
                                       || best.tempoEntriesAvailable
                                       || best.barSignaturesAvailable);
        return best;
    }

private:
    ARAContextDebugState() = default;

    mutable std::mutex mutex;
    std::unordered_map<const void*, ARAContextDebugSnapshot> snapshots;
};

#pragma once

#include <JuceHeader.h>
#include <mutex>

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
};

class ARAContextDebugState final
{
public:
    static ARAContextDebugState& instance()
    {
        static ARAContextDebugState state;
        return state;
    }

    void setSnapshot(const ARAContextDebugSnapshot& newSnapshot)
    {
        const std::scoped_lock lock(mutex);
        snapshot = newSnapshot;
    }

    ARAContextDebugSnapshot getSnapshot() const
    {
        const std::scoped_lock lock(mutex);
        return snapshot;
    }

private:
    ARAContextDebugState() = default;

    mutable std::mutex mutex;
    ARAContextDebugSnapshot snapshot;
};

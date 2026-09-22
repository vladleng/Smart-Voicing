#include "ARAContextProvider.h"
#include "HarmonicContextDebugText.h"
#include "SharedHarmonicContext.h"

#include <algorithm>
#include <cmath>

namespace smartvoicing::harmony
{
namespace
{
constexpr double kTimelineEpsilon = 1.0e-12;

IntervalMask copyIntervals(const std::uint8_t (&source)[kPitchClassCount]) noexcept
{
    IntervalMask result;
    std::copy(std::begin(source), std::end(source), result.values.begin());
    return result;
}

template <typename Event>
int findActiveEventIndexRealtime(const Event* events, int count, double ppq) noexcept
{
    if (count <= 0 || ppq < 0.0)
        return -1;

    int active = -1;
    for (int index = 0; index < count; ++index)
    {
        if (events[index].position <= ppq + kTimelineEpsilon)
            active = index;
        else
            break;
    }

    return active;
}

HarmonicContext makeContext(const SharedHarmonicContextSnapshot& shared, double ppq) noexcept
{
    HarmonicContext result;
    result.providerConnected = shared.connected;
    result.positionAvailable = ppq >= 0.0;
    result.playing = shared.transportPlaying;
    result.ppq = ppq;
    result.harmonicRevision = shared.revision;
    result.transportRevision = shared.transportRevision;

    if (! result.positionAvailable)
        return result;

    // 0.3c boundary rule:
    // - while transport is PLAYING, the musical engine must switch context only
    //   on the exact ARA event boundary (sample-accurate path);
    // - while transport is STOPPED, keep the small UI/cursor tolerance that makes
    //   Studio Pro grid inspection stable despite floating-point cursor jitter.
    //
    // Using the UI tolerance during playback can expose the next chord a few
    // samples too early and create microscopic transient MIDI notes at bar joins.
    const auto realtime = shared.transportPlaying;

    const auto chordIndex = realtime
        ? findActiveEventIndexRealtime(shared.sheetChords, shared.sheetChordStoredCount, ppq)
        : smartvoicing::debug::findActiveEventIndex(shared.sheetChords,
                                                    shared.sheetChordStoredCount,
                                                    ppq);
    if (chordIndex >= 0)
    {
        const auto& chord = shared.sheetChords[chordIndex];
        result.chord.available = shared.sheetChordsAvailable;
        result.chord.defined = ! smartvoicing::debug::chordIsUndefined(chord);
        result.chord.startPpq = chord.position;
        result.chord.root = chord.root;
        result.chord.bass = chord.bass;
        result.chord.intervals = copyIntervals(chord.intervals);
    }

    const auto keyIndex = realtime
        ? findActiveEventIndexRealtime(shared.keySignatures, shared.keySignatureStoredCount, ppq)
        : smartvoicing::debug::findActiveEventIndex(shared.keySignatures,
                                                    shared.keySignatureStoredCount,
                                                    ppq);
    if (keyIndex >= 0)
    {
        const auto& key = shared.keySignatures[keyIndex];
        result.key.available = shared.keySignaturesAvailable;
        result.key.defined = true;
        result.key.startPpq = key.position;
        result.key.root = key.root;
        result.key.intervals = copyIntervals(key.intervals);
    }

    const auto barIndex = realtime
        ? findActiveEventIndexRealtime(shared.barSignatures, shared.barSignatureStoredCount, ppq)
        : smartvoicing::debug::findActiveEventIndex(shared.barSignatures,
                                                    shared.barSignatureStoredCount,
                                                    ppq);
    if (barIndex >= 0)
    {
        const auto& bar = shared.barSignatures[barIndex];
        result.timeSignature.available = shared.barSignaturesAvailable;
        result.timeSignature.startPpq = bar.position;
        result.timeSignature.numerator = bar.numerator;
        result.timeSignature.denominator = bar.denominator;
    }

    return result;
}

double quarterToTime(const SharedHarmonicContextSnapshot& shared, double ppq) noexcept
{
    const auto count = shared.tempoEntryStoredCount;
    if (! shared.tempoEntriesAvailable || count <= 0 || ppq < 0.0)
        return -1.0;

    if (count == 1)
    {
        const auto& point = shared.tempoEntries[0];
        return std::abs(ppq - point.quarterPosition) <= smartvoicing::debug::kBoundaryTolerancePpq
            ? point.timePosition
            : -1.0;
    }

    int right = 1;
    while (right < count && shared.tempoEntries[right].quarterPosition < ppq)
        ++right;

    if (right >= count)
        right = count - 1;

    const auto left = right - 1;
    const auto& a = shared.tempoEntries[left];
    const auto& b = shared.tempoEntries[right];
    const auto deltaQuarter = b.quarterPosition - a.quarterPosition;
    if (std::abs(deltaQuarter) <= kTimelineEpsilon)
        return -1.0;

    const auto alpha = (ppq - a.quarterPosition) / deltaQuarter;
    return a.timePosition + alpha * (b.timePosition - a.timePosition);
}

double timeToQuarter(const SharedHarmonicContextSnapshot& shared, double seconds) noexcept
{
    const auto count = shared.tempoEntryStoredCount;
    if (! shared.tempoEntriesAvailable || count <= 0 || seconds < 0.0)
        return -1.0;

    if (count == 1)
    {
        const auto& point = shared.tempoEntries[0];
        return std::abs(seconds - point.timePosition) <= 1.0e-9
            ? point.quarterPosition
            : -1.0;
    }

    int right = 1;
    while (right < count && shared.tempoEntries[right].timePosition < seconds)
        ++right;

    if (right >= count)
        right = count - 1;

    const auto left = right - 1;
    const auto& a = shared.tempoEntries[left];
    const auto& b = shared.tempoEntries[right];
    const auto deltaTime = b.timePosition - a.timePosition;
    if (std::abs(deltaTime) <= kTimelineEpsilon)
        return -1.0;

    const auto alpha = (seconds - a.timePosition) / deltaTime;
    return a.quarterPosition + alpha * (b.quarterPosition - a.quarterPosition);
}
}

HarmonicContext ARAContextProvider::currentContext() noexcept
{
    const auto shared = SharedHarmonicContextBridge::instance().read();
    const auto ppq = shared.transportAvailable ? shared.transportPpq : -1.0;
    return makeContext(shared, ppq);
}

HarmonicContext ARAContextProvider::contextAt(double ppq) noexcept
{
    const auto shared = SharedHarmonicContextBridge::instance().read();
    return makeContext(shared, ppq);
}

double ARAContextProvider::nextChordStartAfter(double ppq) noexcept
{
    const auto shared = SharedHarmonicContextBridge::instance().read();
    if (! shared.sheetChordsAvailable || shared.sheetChordStoredCount <= 0 || ppq < 0.0)
        return -1.0;

    // During playback use only a numerical epsilon. The wider tolerance belongs
    // to stopped cursor diagnostics; using it in the realtime scheduler can skip
    // an imminent boundary that is still a few samples ahead.
    const auto tolerance = shared.transportPlaying
        ? kTimelineEpsilon
        : smartvoicing::debug::kBoundaryTolerancePpq;
    const auto threshold = ppq + tolerance;

    for (int index = 0; index < shared.sheetChordStoredCount; ++index)
    {
        const auto position = shared.sheetChords[index].position;
        if (position > threshold)
            return position;
    }

    return -1.0;
}

double ARAContextProvider::secondsAtPpq(double ppq) noexcept
{
    return quarterToTime(SharedHarmonicContextBridge::instance().read(), ppq);
}

double ARAContextProvider::ppqAtSeconds(double seconds) noexcept
{
    return timeToQuarter(SharedHarmonicContextBridge::instance().read(), seconds);
}
}

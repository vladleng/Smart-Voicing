#include "ARAContextProvider.h"
#include "HarmonicContextDebugText.h"
#include "SharedHarmonicContext.h"

#include <algorithm>

namespace smartvoicing::harmony
{
namespace
{
IntervalMask copyIntervals(const std::uint8_t (&source)[kPitchClassCount]) noexcept
{
    IntervalMask result;
    std::copy(std::begin(source), std::end(source), result.values.begin());
    return result;
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

    const auto chordIndex = smartvoicing::debug::findActiveEventIndex(shared.sheetChords,
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

    const auto keyIndex = smartvoicing::debug::findActiveEventIndex(shared.keySignatures,
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

    const auto barIndex = smartvoicing::debug::findActiveEventIndex(shared.barSignatures,
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
}

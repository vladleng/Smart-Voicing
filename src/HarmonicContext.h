#pragma once

#include <array>
#include <cstdint>

namespace smartvoicing::harmony
{
inline constexpr int kPitchClassCount = 12;
inline constexpr int kVoiceCount = 4;

struct IntervalMask
{
    std::array<std::uint8_t, kPitchClassCount> values {};

    bool any() const noexcept
    {
        for (const auto value : values)
            if (value != 0)
                return true;

        return false;
    }
};

// Host-neutral representation of the active chord. root/bass deliberately keep
// the provider's native pitch encoding for 0.2a; canonical pitch-class
// normalisation belongs to 0.2b.
struct ChordContext
{
    bool available = false;
    bool defined = false;
    double startPpq = -1.0;
    std::int32_t root = 0;
    std::int32_t bass = 0;
    IntervalMask intervals;
};

struct KeyContext
{
    bool available = false;
    bool defined = false;
    double startPpq = -1.0;
    std::int32_t root = 0;
    IntervalMask intervals;
};

struct TimeSignatureContext
{
    bool available = false;
    double startPpq = -1.0;
    std::int32_t numerator = 4;
    std::int32_t denominator = 4;
};

struct HarmonicContext
{
    bool providerConnected = false;
    bool positionAvailable = false;
    bool playing = false;
    double ppq = -1.0;
    std::uint64_t harmonicRevision = 0;
    std::uint64_t transportRevision = 0;

    ChordContext chord;
    KeyContext key;
    TimeSignatureContext timeSignature;
};

struct VoiceSlot
{
    bool active = false;
    int midiNote = -1;
};

// Neutral output contract between future Harmony Core and the existing Router.
// No MIDI channel or host-specific routing information is stored here.
struct VoiceOutput
{
    std::array<VoiceSlot, kVoiceCount> voices {};

    void clear() noexcept
    {
        for (auto& voice : voices)
            voice = {};
    }
};

class IHarmonicContextProvider
{
public:
    virtual ~IHarmonicContextProvider() = default;

    virtual HarmonicContext currentContext() noexcept = 0;
    virtual HarmonicContext contextAt(double ppq) noexcept = 0;
};
}

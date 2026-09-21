#pragma once

#include "HarmonicContext.h"

#include <array>
#include <cstdint>

namespace smartvoicing::harmony
{
enum class KeyMode : std::uint8_t
{
    undefined = 0,
    major,
    minor,
    custom
};

struct NormalizedKey
{
    bool valid = false;
    std::int32_t rootFifths = 0;
    int rootPitchClass = 0;
    KeyMode mode = KeyMode::undefined;
    std::array<bool, kPitchClassCount> tones {};

    bool hasRelativeTone(int semitones) const noexcept;
    bool hasPitchClass(int pitchClass) const noexcept;
};

NormalizedKey normalizeKey(const KeyContext& source) noexcept;
const char* keyModeName(KeyMode mode) noexcept;

// Returns the 1-based scale degree for an absolute pitch class by walking the
// active key's own interval mask from the tonic upward. 0 means the pitch class
// is not part of the key or the key is unavailable.
int scaleDegreeForPitchClass(const NormalizedKey& key, int absolutePitchClass) noexcept;
}

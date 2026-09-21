#include "KeyModel.h"
#include "ChordModel.h"

#include <array>

namespace smartvoicing::harmony
{
namespace
{
constexpr int wrap12(int value) noexcept
{
    value %= kPitchClassCount;
    return value < 0 ? value + kPitchClassCount : value;
}

constexpr std::array<bool, kPitchClassCount> majorMask
{
    true, false, true, false, true, true, false, true, false, true, false, true
};

constexpr std::array<bool, kPitchClassCount> minorMask
{
    true, false, true, true, false, true, false, true, true, false, true, false
};

bool equalsMask(const std::array<bool, kPitchClassCount>& tones,
                const std::array<bool, kPitchClassCount>& reference) noexcept
{
    for (int i = 0; i < kPitchClassCount; ++i)
        if (tones[static_cast<std::size_t>(i)] != reference[static_cast<std::size_t>(i)])
            return false;

    return true;
}
}

bool NormalizedKey::hasRelativeTone(int semitones) const noexcept
{
    return valid && tones[static_cast<std::size_t>(wrap12(semitones))];
}

bool NormalizedKey::hasPitchClass(int pitchClass) const noexcept
{
    if (! valid)
        return false;

    const auto relative = wrap12(pitchClass - rootPitchClass);
    return tones[static_cast<std::size_t>(relative)];
}

NormalizedKey normalizeKey(const KeyContext& source) noexcept
{
    NormalizedKey result;
    if (! source.available || ! source.defined || ! source.intervals.any())
        return result;

    result.valid = true;
    result.rootFifths = source.root;
    result.rootPitchClass = circleOfFifthsToPitchClass(source.root);

    for (int i = 0; i < kPitchClassCount; ++i)
        result.tones[static_cast<std::size_t>(i)] = source.intervals.values[static_cast<std::size_t>(i)] != 0;

    if (equalsMask(result.tones, majorMask))
        result.mode = KeyMode::major;
    else if (equalsMask(result.tones, minorMask))
        result.mode = KeyMode::minor;
    else
        result.mode = KeyMode::custom;

    return result;
}

const char* keyModeName(KeyMode mode) noexcept
{
    switch (mode)
    {
        case KeyMode::major: return "major";
        case KeyMode::minor: return "minor";
        case KeyMode::custom: return "custom";
        case KeyMode::undefined:
        default: return "undefined";
    }
}

int scaleDegreeForPitchClass(const NormalizedKey& key, int absolutePitchClass) noexcept
{
    if (! key.valid)
        return 0;

    const auto relative = wrap12(absolutePitchClass - key.rootPitchClass);
    if (! key.tones[static_cast<std::size_t>(relative)])
        return 0;

    int degree = 0;
    for (int semitone = 0; semitone <= relative; ++semitone)
        if (key.tones[static_cast<std::size_t>(semitone)])
            ++degree;

    return degree;
}
}

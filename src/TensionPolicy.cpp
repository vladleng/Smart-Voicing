#include "TensionPolicy.h"

namespace smartvoicing::harmony
{
namespace
{
int normalizePitchClass(int pitchClass) noexcept
{
    auto value = pitchClass % kPitchClassCount;
    if (value < 0)
        value += kPitchClassCount;
    return value;
}

int melodyRelativeToRoot(int midiNote, const NormalizedChord& chord) noexcept
{
    return normalizePitchClass(normalizePitchClass(midiNote) - chord.rootPitchClass);
}

bool isExplicitTension(const NormalizedChord& chord, int relative) noexcept
{
    if (relative < 0 || relative >= kPitchClassCount || ! chord.hasTone(relative))
        return false;

    const auto degree = chord.degrees[static_cast<std::size_t>(relative)];
    return degree == 9 || degree == 11 || degree == 13;
}

bool candidateIsHalfStepAboveChordTone(const NormalizedChord& chord, int relative) noexcept
{
    const auto chordToneBelow = normalizePitchClass(relative - 1);
    return chord.hasTone(chordToneBelow);
}

bool candidateIsWholeStepAboveChordTone(const NormalizedChord& chord, int relative) noexcept
{
    const auto chordToneBelow = normalizePitchClass(relative - 2);
    return chord.hasTone(chordToneBelow);
}

bool preferredTensionForQuality(const NormalizedChord& chord, int relative) noexcept
{
    // Conservative 0.3d defaults. Explicit Chord Track tensions bypass this.
    if (relative == 2 || relative == 9) // natural 9 / 13
        return true;

    if (relative == 5) // natural 11 is especially idiomatic on minor/sus sonorities
    {
        return chord.quality == ChordQuality::minor
            || chord.quality == ChordQuality::halfDiminished
            || chord.quality == ChordQuality::suspended2
            || chord.quality == ChordQuality::suspended4
            || chord.quality == ChordQuality::noThird;
    }

    return false;
}

void addMajorCollection(std::array<bool, kPitchClassCount>& absolute,
                        int tonicPitchClass) noexcept
{
    constexpr int intervals[] = { 0, 2, 4, 5, 7, 9, 11 };
    for (const auto interval : intervals)
        absolute[static_cast<std::size_t>(normalizePitchClass(tonicPitchClass + interval))] = true;
}

void addMinorCollection(std::array<bool, kPitchClassCount>& absolute,
                        int tonicPitchClass) noexcept
{
    constexpr int intervals[] = { 0, 2, 3, 5, 7, 8, 10 };
    for (const auto interval : intervals)
        absolute[static_cast<std::size_t>(normalizePitchClass(tonicPitchClass + interval))] = true;
}

void addMixolydianCollection(std::array<bool, kPitchClassCount>& absolute,
                             int chordRootPitchClass) noexcept
{
    constexpr int intervals[] = { 0, 2, 4, 5, 7, 9, 10 };
    for (const auto interval : intervals)
        absolute[static_cast<std::size_t>(normalizePitchClass(chordRootPitchClass + interval))] = true;
}

std::array<bool, kPitchClassCount> makeInferredCollection(const NormalizedChord& chord,
                                                           const NormalizedKey& key,
                                                           const HarmonicAnalysis& harmonic,
                                                           bool& fromFunctionScale) noexcept
{
    std::array<bool, kPitchClassCount> result {};
    fromFunctionScale = false;

    // A dominant-function chord gets a conservative Mixolydian baseline. This
    // is especially important for applied dominants: D7 in C major must retain
    // F# from the explicit chord and should not inherit F-natural merely because
    // it belongs to the global key. Explicit alterations still override this
    // inferred collection.
    const auto dominantContext = chord.quality == ChordQuality::dominant
        && (! harmonic.valid || harmonic.effectiveFunction == HarmonicFunction::dominant);

    if (dominantContext)
    {
        addMixolydianCollection(result, chord.rootPitchClass);
        fromFunctionScale = true;
        return result;
    }

    // For a confirmed/candidate parallel-mode borrowing, use the parallel mode
    // as the inference collection instead of forcing the active major/minor key.
    if (harmonic.valid && harmonic.modalInterchangeCandidate && key.valid)
    {
        if (harmonic.modalInterchangeSource == KeyMode::major)
        {
            addMajorCollection(result, key.rootPitchClass);
            fromFunctionScale = true;
            return result;
        }

        if (harmonic.modalInterchangeSource == KeyMode::minor)
        {
            addMinorCollection(result, key.rootPitchClass);
            fromFunctionScale = true;
            return result;
        }
    }

    if (key.valid)
    {
        for (int pitchClass = 0; pitchClass < kPitchClassCount; ++pitchClass)
            result[static_cast<std::size_t>(pitchClass)] = key.hasPitchClass(pitchClass);
    }

    return result;
}
}

const TensionTonePolicy& TensionPolicy::tone(int relativeSemitones) const noexcept
{
    static const TensionTonePolicy unavailableTone {};
    if (relativeSemitones < 0 || relativeSemitones >= kPitchClassCount)
        return unavailableTone;
    return tones[static_cast<std::size_t>(relativeSemitones)];
}

bool TensionPolicy::isHarmonyCandidate(int relativeSemitones) const noexcept
{
    const auto role = tone(relativeSemitones).role;
    return role == TensionRole::chordTone
        || role == TensionRole::explicitTension
        || role == TensionRole::preferred
        || role == TensionRole::available
        || role == TensionRole::contextual;
}

TensionPolicy buildTensionPolicy(const NormalizedChord& chord,
                                 const NormalizedKey& key,
                                 const HarmonicAnalysis& harmonic,
                                 int melodyMidiNote) noexcept
{
    TensionPolicy result;
    if (! chord.valid)
        return result;

    result.valid = true;

    bool functionScale = false;
    const auto inferredAbsolute = makeInferredCollection(chord, key, harmonic, functionScale);

    for (int relative = 0; relative < kPitchClassCount; ++relative)
    {
        auto& tone = result.tones[static_cast<std::size_t>(relative)];
        const auto absolutePitchClass = normalizePitchClass(chord.rootPitchClass + relative);
        tone.fromActiveKey = key.valid && key.hasPitchClass(absolutePitchClass);
        tone.fromFunctionScale = functionScale
            && inferredAbsolute[static_cast<std::size_t>(absolutePitchClass)];

        if (chord.hasTone(relative))
        {
            tone.explicitFromChord = true;
            tone.role = isExplicitTension(chord, relative)
                ? TensionRole::explicitTension
                : TensionRole::chordTone;
            continue;
        }

        if (! inferredAbsolute[static_cast<std::size_t>(absolutePitchClass)])
            continue;

        // Book-derived baseline: half-step above a chord tone is generally not
        // treated as a stable harmonic tension. This is a policy weight, not a
        // ban on performer melody; explicit tensions have already bypassed it.
        if (candidateIsHalfStepAboveChordTone(chord, relative))
        {
            tone.role = TensionRole::avoidAsHarmony;
            continue;
        }

        if (preferredTensionForQuality(chord, relative))
        {
            tone.role = TensionRole::preferred;
            continue;
        }

        // Whole-step-above-chord-tone is the conservative availability rule from
        // Modern Jazz Voicings. Other in-scale colours remain contextual rather
        // than being promoted automatically.
        tone.role = candidateIsWholeStepAboveChordTone(chord, relative)
            ? TensionRole::available
            : TensionRole::contextual;
    }

    if (melodyMidiNote >= 0 && melodyMidiNote <= 127)
    {
        const auto relative = melodyRelativeToRoot(melodyMidiNote, chord);
        if (! chord.hasTone(relative))
            result.tones[static_cast<std::size_t>(relative)].melodyImposed = true;
    }

    return result;
}

const char* tensionRoleName(TensionRole role) noexcept
{
    switch (role)
    {
        case TensionRole::unavailable:       return "Unavailable";
        case TensionRole::chordTone:         return "Chord Tone";
        case TensionRole::explicitTension:   return "Explicit";
        case TensionRole::preferred:         return "Preferred";
        case TensionRole::available:         return "Available";
        case TensionRole::contextual:        return "Contextual";
        case TensionRole::avoidAsHarmony:    return "Avoid-as-harmony";
    }

    return "Unavailable";
}
}

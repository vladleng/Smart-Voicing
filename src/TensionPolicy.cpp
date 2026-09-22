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
    // Conservative non-dominant defaults. Dominants are handled by a separate
    // functional profile because 9/13/alterations depend strongly on target.
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

bool isDominantColourContext(const NormalizedChord& chord,
                             const NormalizedKey& key) noexcept
{
    // Chord quality itself is valid evidence of a dominant sonority. Function
    // and next-chord evidence refine the profile; they do not need to be known
    // before the engine may classify its tension vocabulary.
    return key.valid && chord.quality == ChordQuality::dominant;
}

bool isMinorTargetQuality(ChordQuality quality) noexcept
{
    return quality == ChordQuality::minor
        || quality == ChordQuality::halfDiminished
        || quality == ChordQuality::diminished;
}

bool isMajorTargetQuality(ChordQuality quality) noexcept
{
    // A dominant target (e.g. D7 -> G7) still has a major-third target quality
    // and behaves closer to the major-target profile than to a minor tonic.
    return quality == ChordQuality::major
        || quality == ChordQuality::dominant
        || quality == ChordQuality::augmented;
}

FunctionalTensionProfile deriveFunctionalProfile(const NormalizedChord& chord,
                                                  const NormalizedKey& key,
                                                  const HarmonicAnalysis& harmonic) noexcept
{
    if (! isDominantColourContext(chord, key))
        return FunctionalTensionProfile::neutral;

    // 0.3f deliberately uses only REAL Chord Track resolution evidence.
    // No next chord means no assumed target, even when Key/Function strongly
    // suggest a likely continuation. This keeps the engine deterministic and
    // under arranger control: A7 and A7->Dm are intentionally different input.
    if (harmonic.dominantResolutionConfirmed)
    {
        if (isMinorTargetQuality(harmonic.dominantTargetQuality))
            return FunctionalTensionProfile::dominantMinorTarget;

        if (isMajorTargetQuality(harmonic.dominantTargetQuality))
            return FunctionalTensionProfile::dominantMajorTarget;
    }

    return FunctionalTensionProfile::dominantUnresolved;
}

bool isDominantAlteredCandidate(int relative) noexcept
{
    // Generic altered-dominant pitch-class pool. Musical role/spelling is
    // assigned by the functional profile: semitone 6 is NOT automatically a
    // target-directed #11/b5, and semitone 8 may mean inferred b13 or explicit #5.
    return relative == 1 || relative == 3 || relative == 6 || relative == 8;
}

void classifyDominantTone(TensionTonePolicy& tone,
                          int relative,
                          FunctionalTensionProfile profile,
                          bool resolutionConfirmed) noexcept
{
    tone.fromFunctionScale = true;

    // Natural 11 above the dominant third forms the classic avoid relationship
    // in the conservative baseline. Explicit Chord Track material has already
    // bypassed this inference path.
    if (relative == 5)
    {
        tone.role = TensionRole::avoidAsHarmony;
        return;
    }

    if (profile == FunctionalTensionProfile::dominantMinorTarget)
    {
        // 0.4a fix2 / Modern Jazz Voicings alignment:
        // a confirmed V7 -> minor target naturally supports b13 as the primary
        // target-aware colour. In this inferred context the pitch class is a
        // b13 tension, not an automatically inferred "#5 chord alteration".
        if (relative == 8) // b13 pitch class
        {
            tone.role = TensionRole::preferred;
            tone.alteredCandidate = true;
            tone.functionallyDirected = true;
            return;
        }

        // b9 is a stronger but still target-directed minor-dominant colour.
        // It stays Rich-only in the current Clean/Color/Rich model.
        if (relative == 1)
        {
            tone.role = TensionRole::contextual;
            tone.alteredCandidate = true;
            tone.functionallyDirected = true;
            return;
        }

        // A minor target by itself does NOT justify #9 or #11/b5 as directed
        // tensions. They remain contextual altered candidates. Explicit #9/b5
        // chord symbols remain authoritative, and a future substitute-dominant
        // profile may promote #11 through its own Lydian-b7 semantics.
        if (relative == 3 || relative == 6)
        {
            tone.role = TensionRole::contextual;
            tone.alteredCandidate = true;
            tone.functionallyDirected = false;
            return;
        }

        // Natural 9 can remain a restrained Color option when the active tonal
        // context actually supports it. Natural 13 is intentionally NOT inferred
        // for a confirmed minor target: target context outranks a generic
        // Mixolydian reading. Explicit E13/A13 remains authoritative above this.
        if (relative == 2 && tone.fromActiveKey)
        {
            tone.role = TensionRole::available;
            return;
        }

        return;
    }

    // Major-target or unresolved dominant keeps the conservative Mixolydian
    // inside vocabulary for Color. With no real next chord this is explicitly
    // an unresolved/generic profile, not an inferred major resolution.
    if (relative == 2 || relative == 9)
    {
        tone.role = TensionRole::preferred;
        return;
    }

    if (isDominantAlteredCandidate(relative))
    {
        tone.role = TensionRole::contextual;
        tone.alteredCandidate = true;
        tone.functionallyDirected = profile == FunctionalTensionProfile::dominantMajorTarget
                                 && resolutionConfirmed;
    }
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

std::array<bool, kPitchClassCount> makeInferredCollection(const NormalizedChord& chord,
                                                           const NormalizedKey& key,
                                                           const HarmonicAnalysis& harmonic,
                                                           bool& fromFunctionScale) noexcept
{
    std::array<bool, kPitchClassCount> result {};
    fromFunctionScale = false;

    // Dominant harmony is classified separately by FunctionalTensionProfile.
    if (chord.quality == ChordQuality::dominant)
        return result;

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

bool TensionPolicy::isHarmonyCandidate(int relativeSemitones, TensionLevel level) const noexcept
{
    const auto& policy = tone(relativeSemitones);

    switch (policy.role)
    {
        case TensionRole::chordTone:
        case TensionRole::explicitTension:
            return true;

        case TensionRole::preferred:
        case TensionRole::available:
            return level != TensionLevel::clean;

        case TensionRole::contextual:
            return level == TensionLevel::rich;

        case TensionRole::avoidAsHarmony:
        case TensionRole::unavailable:
            return false;
    }

    return false;
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
    result.functionalProfile = deriveFunctionalProfile(chord, key, harmonic);
    result.resolutionConfirmed = harmonic.dominantResolutionConfirmed;

    bool functionScale = false;
    const auto inferredAbsolute = makeInferredCollection(chord, key, harmonic, functionScale);
    const auto dominantContext = isDominantColourContext(chord, key);

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

        if (dominantContext)
        {
            classifyDominantTone(tone,
                                 relative,
                                 result.functionalProfile,
                                 result.resolutionConfirmed);
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

const char* tensionLevelName(TensionLevel level) noexcept
{
    switch (level)
    {
        case TensionLevel::clean: return "Clean";
        case TensionLevel::color: return "Color";
        case TensionLevel::rich:  return "Rich";
    }

    return "Clean";
}

const char* functionalTensionProfileName(FunctionalTensionProfile profile) noexcept
{
    switch (profile)
    {
        case FunctionalTensionProfile::neutral:             return "Neutral";
        case FunctionalTensionProfile::dominantUnresolved:  return "Dominant / unresolved";
        case FunctionalTensionProfile::dominantMajorTarget: return "Dominant -> major target";
        case FunctionalTensionProfile::dominantMinorTarget: return "Dominant -> minor target";
    }

    return "Neutral";
}
}

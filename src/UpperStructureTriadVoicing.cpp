#include "UpperStructureTriadVoicing.h"

#include <array>
#include <climits>

namespace smartvoicing::harmony
{
namespace
{
int pitchClass(int note) noexcept { return (note % 12 + 12) % 12; }
int relative(int note, const NormalizedChord& chord) noexcept
{
    return (pitchClass(note) - chord.rootPitchClass + 12) % 12;
}
bool extendedHarmony(const NormalizedChord& chord) noexcept
{
    for (int i = 0; i < 12; ++i)
        if (chord.hasTone(i) && chord.degrees[static_cast<std::size_t>(i)] == 7)
            return true;
    return chord.hasExtension(ChordExtension::ninth)
        || chord.hasExtension(ChordExtension::eleventh)
        || chord.hasExtension(ChordExtension::thirteenth);
}
bool allowed(int pc, const VoicingContext& context) noexcept
{
    const int degree = (pc - context.chord.rootPitchClass + 12) % 12;
    return context.chord.hasTone(degree)
        || (extendedHarmony(context.chord) && context.tension.valid
            && context.tension.isHarmonyCandidate(degree, context.tensionLevel));
}
bool third(int degree, const NormalizedChord& chord) noexcept
{
    if (! chord.hasTone(degree)) return false;
    if (chord.degrees[static_cast<std::size_t>(degree)] == 3) return true;
    if (chord.degrees[static_cast<std::size_t>(degree)] == 9) return false;
    if (chord.quality == ChordQuality::major || chord.quality == ChordQuality::dominant)
        return degree == 4;
    return (chord.quality == ChordQuality::minor
            || chord.quality == ChordQuality::halfDiminished
            || chord.quality == ChordQuality::diminished) && degree == 3;
}
bool seventh(int degree, const NormalizedChord& chord) noexcept
{
    return chord.hasTone(degree)
        && (chord.degrees[static_cast<std::size_t>(degree)] == 7
            || degree == 10 || degree == 11);
}
bool protectedTone(int degree, const NormalizedChord& chord) noexcept
{
    return (degree == 6
            && (chord.quality == ChordQuality::halfDiminished
                || chord.hasAlteration(ChordAlteration::flatFifth)
                || chord.hasAlteration(ChordAlteration::sharpEleventh)))
        || (degree == 8
            && (chord.hasAlteration(ChordAlteration::sharpFifth)
                || chord.hasAlteration(ChordAlteration::flatThirteenth)))
        || (degree == 1 && chord.hasAlteration(ChordAlteration::flatNinth))
        || (degree == 3 && chord.hasAlteration(ChordAlteration::sharpNinth))
        || (degree == 2 && chord.quality == ChordQuality::suspended2)
        || (degree == 5 && chord.quality == ChordQuality::suspended4);
}
bool contains(const std::array<int,4>& notes, const NormalizedChord& chord,
              bool (*role)(int,const NormalizedChord&) noexcept) noexcept
{
    for (const auto note : notes)
        if (role(relative(note,chord),chord)) return true;
    return false;
}
int score(const std::array<int,4>& notes, const std::array<int,3>& triad,
          const VoicingContext& context) noexcept
{
    const auto& chord = context.chord;
    int value = 0;
    const int upperSpan = notes[0]-notes[2];
    if (upperSpan > 12) value += (upperSpan-12)*3;
    if (upperSpan < 5) value += (5-upperSpan)*3;
    const int supportGap = notes[2]-notes[3];
    if (supportGap < 5) value += (5-supportGap)*4;
    if (supportGap > 15) value += (supportGap-15)*3;
    const int totalSpan = notes[0]-notes[3];
    if (totalSpan < 16) value += (16-totalSpan)*3;
    if (totalSpan > 32) value += (totalSpan-32)*2;

    int represented = 0;
    for (const auto pc : triad)
    {
        for (int voice=0; voice<3; ++voice)
            if (pitchClass(notes[static_cast<std::size_t>(voice)])==pc)
            { ++represented; break; }
    }
    value += (3-represented)*18; // incomplete UST only when V1/pool prevents full triad
    if (pitchClass(notes[0]) != triad[0]
        && pitchClass(notes[0]) != triad[1]
        && pitchClass(notes[0]) != triad[2])
        value += 10; // performer melody may remain an independent lead

    bool chordThird=false, chordSeventh=false;
    for (int degree=0; degree<12; ++degree)
    {
        chordThird = chordThird || third(degree,chord);
        chordSeventh = chordSeventh || seventh(degree,chord);
        if (protectedTone(degree,chord))
        {
            bool retained=false;
            for (const auto note : notes)
                retained = retained || relative(note,chord)==degree;
            if (! retained) value += 48;
        }
    }
    const bool keptThird=contains(notes,chord,third);
    const bool keptSeventh=contains(notes,chord,seventh);
    if (chordThird && ! keptThird) value += 20;
    if (chordSeventh && ! keptSeventh) value += 20;
    if (chordThird && chordSeventh && ! keptThird && ! keptSeventh) value += 40;

    const int supportDegree=relative(notes[3],chord);
    if (supportDegree != 0 && ! third(supportDegree,chord)
        && ! seventh(supportDegree,chord))
        value += 9; // support should clarify chord identity
    for (int i=1; i<3; ++i)
    {
        const int degree=relative(notes[static_cast<std::size_t>(i)],chord);
        if (! chord.hasTone(degree) && context.tension.valid)
        {
            const auto& tone=context.tension.tone(degree);
            if (tone.role == TensionRole::preferred && tone.functionallyDirected
                && context.tension.resolutionConfirmed) value -= 6;
            else if (tone.role == TensionRole::contextual) value += 4;
        }
    }
    for (int upper=0; upper<3; ++upper)
        for (int lower=upper+1; lower<4; ++lower)
        {
            const int distance=notes[static_cast<std::size_t>(upper)]
                              -notes[static_cast<std::size_t>(lower)];
            if (distance<13 || distance%12!=1) continue;
            bool exception=false;
            for (const int index : {upper,lower})
            {
                if (index==0 || !context.tension.valid) continue;
                const auto& tone=context.tension.tone(
                    relative(notes[static_cast<std::size_t>(index)],chord));
                exception = exception || tone.role==TensionRole::explicitTension
                    || (context.tensionLevel==TensionLevel::rich
                        && context.tension.resolutionConfirmed
                        && tone.functionallyDirected);
            }
            if (!exception) value += 14;
        }
    return value;
}
}
VoiceOutput buildUpperStructureTriadVoicing(int melodyNote,
                                            const VoicingContext& context) noexcept
{
    VoiceOutput result;
    result.clear();
    if (melodyNote<0 || melodyNote>127) return result;
    result.voices[0]={true,melodyNote};
    if (!context.chord.valid) return result;

    // The Stage 4 vocabulary is fixed for this vertical. Precompute it once
    // instead of re-evaluating extension flags in the bounded realtime search.
    std::array<bool,12> legalPitches {};
    for (int pc=0; pc<12; ++pc)
        legalPitches[static_cast<std::size_t>(pc)]=allowed(pc,context);
    int best=INT_MAX;
    std::array<int,4> chosen {};
    const int floor=melodyNote>33 ? melodyNote-33 : 0;
    for (int root=0; root<12; ++root)
        for (int minor=0; minor<2; ++minor)
        {
            const std::array<int,3> triad {
                root, (root+(minor ? 3 : 4))%12, (root+7)%12
            };
            int legal=0;
            for (const auto pc : triad)
                if (legalPitches[static_cast<std::size_t>(pc)]
                    || pc==pitchClass(melodyNote)) ++legal;
            if (legal<2) continue;
            for (int v2=melodyNote-1; v2>=floor+2; --v2)
            {
                const int pc2=pitchClass(v2);
                if (!legalPitches[static_cast<std::size_t>(pc2)]) continue;
                if (pc2!=triad[0] && pc2!=triad[1] && pc2!=triad[2]) continue;
                if (pc2==pitchClass(melodyNote)) continue;
                for (int v3=v2-1; v3>=floor+1; --v3)
                {
                    const int pc3=pitchClass(v3);
                    if (pc3==pc2 || pc3==pitchClass(melodyNote)
                        || !legalPitches[static_cast<std::size_t>(pc3)]) continue;
                    if (pc3!=triad[0] && pc3!=triad[1] && pc3!=triad[2]) continue;
                    for (int v4=v3-1; v4>=floor; --v4)
                    {
                        if (context.chord.slashBass)
                        {
                            if (pitchClass(v4)!=context.chord.bassPitchClass) continue;
                        }
                        else if (!context.chord.hasTone(relative(v4,context.chord)))
                            continue;
                        const std::array<int,4> notes { melodyNote,v2,v3,v4 };
                        const int cost=score(notes,triad,context);
                        if (cost<best)
                        {
                            best=cost;
                            chosen=notes;
                        }
                    }
                }
            }
        }
    if (best==INT_MAX) return result;
    for (std::size_t i=1; i<chosen.size(); ++i)
        result.voices[i]={true,chosen[i]};
    return result;
}
}

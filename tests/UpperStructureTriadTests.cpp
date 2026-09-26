#include "ChordModel.h"
#include "HarmonicFunction.h"
#include "KeyModel.h"
#include "TensionPolicy.h"
#include "VoicingStrategy.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>

using namespace smartvoicing::harmony;
namespace
{
int failures=0;
void expect(bool ok,const char* message)
{
    if (!ok) { ++failures; std::cerr<<"FAIL: "<<message<<'\n'; }
}
ChordContext chord(int root,int bass,std::initializer_list<std::pair<int,int>> tones)
{
    ChordContext c;
    c.available=c.defined=true;
    c.root=root; c.bass=bass;
    for (const auto& [semitones,degree]:tones)
        c.intervals.values[static_cast<std::size_t>(semitones)] =
            static_cast<std::uint8_t>(degree);
    return c;
}
KeyContext cMajor()
{
    KeyContext k;
    k.available=k.defined=true;
    for (int i:{0,2,4,5,7,9,11})
        k.intervals.values[static_cast<std::size_t>(i)]=0xFFu;
    return k;
}
VoicingContext context(const NormalizedChord& c,int melody,
                       TensionLevel level=TensionLevel::clean,
                       const NormalizedChord* next=nullptr)
{
    VoicingContext v;
    v.chord=c; v.key=normalizeKey(cMajor());
    v.harmonic=next ? analyzeHarmonicFunction(c,v.key,*next)
                    : analyzeHarmonicFunction(c,v.key);
    v.tension=buildTensionPolicy(c,v.key,v.harmonic,melody);
    v.tensionLevel=level;
    return v;
}
int relative(int note,const NormalizedChord& c)
{
    return ((note%12)-c.rootPitchClass+12)%12;
}
bool has(const VoiceOutput& v,const NormalizedChord& c,int degree)
{
    for (const auto& voice:v.voices)
        if (voice.active && relative(voice.midiNote,c)==degree) return true;
    return false;
}
bool upperTriad(const VoiceOutput& v)
{
    const int p0=v.voices[0].midiNote%12;
    const int p1=v.voices[1].midiNote%12;
    const int p2=v.voices[2].midiNote%12;
    for (int root=0;root<12;++root)
        for (int minor=0;minor<2;++minor)
        {
            const int a=root,b=(root+(minor?3:4))%12,c=(root+7)%12;
            if ((p0==a||p0==b||p0==c)
                && (p1==a||p1==b||p1==c)
                && (p2==a||p2==b||p2==c)
                && p0!=p1 && p0!=p2 && p1!=p2) return true;
        }
    return false;
}
void testCompleteUpperTriad()
{
    const auto major=normalizeChord(chord(0,0,{{0,1},{4,3},{7,5},{11,7}}));
    const auto v=buildVoicing(76,VoicingType::ust,context(major,76));
    expect(static_cast<int>(VoicingType::ust)==9,"enum append");
    expect(std::string(voicingTypeName(VoicingType::ust))=="UST","name");
    expect(v.voices[0].active && v.voices[0].midiNote==76,"V1 melody");
    expect(upperTriad(v),"upper three voices form a major/minor triad");
    expect(v.voices[3].active && major.hasTone(relative(v.voices[3].midiNote,major)),
           "V4 is structural chord support");
    for (int i=1;i<4;++i)
        expect(v.voices[i].active && v.voices[i-1].midiNote>v.voices[i].midiNote,
               "strict descending order");
}
void testAuthorityAndFallback()
{
    const auto slash=normalizeChord(chord(0,1,{{0,1},{4,3},{7,5},{11,7}}));
    const auto s=buildVoicing(76,VoicingType::ust,context(slash,76));
    expect(s.voices[3].active && s.voices[3].midiNote%12==slash.bassPitchClass,
           "explicit slash bass");
    const auto halfDim=normalizeChord(chord(1,1,{{0,1},{3,3},{6,5},{10,7}}));
    const auto h=buildVoicing(72,VoicingType::ust,context(halfDim,72));
    expect(has(h,halfDim,6),"m7b5 b5 identity");
    const auto sharp11=normalizeChord(chord(0,0,{{0,1},{4,3},{6,11},{7,5},{10,7}}));
    const auto a=buildVoicing(74,VoicingType::ust,context(sharp11,74));
    expect(has(a,sharp11,6),"explicit #11 protected");
    const auto triad=normalizeChord(chord(0,0,{{0,1},{4,3},{7,5}}));
    const auto rich=buildVoicing(76,VoicingType::ust,
                                context(triad,76,TensionLevel::rich));
    for (int i=1;i<4;++i)
        if (rich.voices[i].active)
            expect(triad.hasTone(relative(rich.voices[i].midiNote,triad)),
                   "plain triad stays chord-tone-only");
    VoicingContext absent;
    const auto n=buildVoicing(72,VoicingType::ust,absent);
    expect(n.voices[0].active && !n.voices[1].active,"no chord V1 only");
    const auto low=buildVoicing(1,VoicingType::ust,context(triad,1));
    expect(low.voices[0].midiNote==1 && !low.voices[3].active,
           "MIDI underflow V1 only");
    const auto again=buildVoicing(72,VoicingType::ust,context(halfDim,72));
    for (std::size_t i=0;i<h.voices.size();++i)
        expect(h.voices[i].active==again.voices[i].active
                   && h.voices[i].midiNote==again.voices[i].midiNote,
               "same context gives deterministic output");
}
void testStage4Vocabulary()
{
    const auto dominant=normalizeChord(chord(1,1,{{0,1},{4,3},{7,5},{10,7}}));
    const auto minorTarget=normalizeChord(chord(-3,-3,{{0,1},{3,3},{7,5},{10,7}}));
    for (auto level:{TensionLevel::clean,TensionLevel::color,TensionLevel::rich})
    {
        const auto ctx=context(dominant,65,level,&minorTarget);
        const auto v=buildVoicing(65,VoicingType::ust,ctx);
        expect(v.voices[0].midiNote==65,"melody on b7 remains authoritative");
        for (int i=1;i<3;++i)
            if (v.voices[i].active)
            {
                const int degree=relative(v.voices[i].midiNote,dominant);
                expect(dominant.hasTone(degree)
                           || ctx.tension.isHarmonyCandidate(degree,level),
                       "upper structure respects Stage 4 palette");
            }
    }
}
}
int main()
{
    testCompleteUpperTriad(); testAuthorityAndFallback(); testStage4Vocabulary();
    if (failures) return EXIT_FAILURE;
    std::cout<<"UST strategy tests passed\n";
    return EXIT_SUCCESS;
}

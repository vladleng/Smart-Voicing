# Smart Voicing — Concept & Development Notes

**Status:** Concept / Pre-Prototype  
**Working name:** Smart Voicing  
**Primary goal:** Build a lightweight, host-agnostic MIDI harmonization and voice-allocation plug-in that can use the harmonic context of the DAW itself.

---

## 1. Product idea

Smart Voicing should let the musician work directly from the harmonic structure already present in the DAW.

Target workflow:

1. Create the chord progression in the DAW.
2. Set the project key / tonal context in the DAW.
3. Load Smart Voicing.
4. Choose a voicing mode or preset.
5. Arm several destination instrument tracks.
6. Press Record.
7. Play either a melody or chord input.
8. Smart Voicing generates / distributes separate musical voices in real time.
9. Edit the resulting instrument parts individually afterwards.

The long-term user experience should feel like:

```text
DAW Chord + Key context
        +
     MIDI input
        +
  voicing preset
        ↓
  independent voices
        ↓
 separate instrument tracks
```

---

## 2. Core principle: the DAW remains the source of truth

Smart Voicing should not require the user to recreate a second chord timeline inside the plug-in if the host already exposes one.

The preferred context is:

```text
KEY / TONALITY
+
CURRENT CHORD
+
TIMELINE POSITION
```

The harmonizer can then combine that context with:

- melody note;
- played chord;
- selected voicing mode;
- voice-leading rules;
- instrument ranges;
- future arrangement rules.

---

## 3. Host-agnostic architecture

Smart Voicing must not be tied specifically to Fender Studio or any single DAW.

Fender Studio can be the first reference host used for development and testing, but the internal architecture should remain host-neutral.

Conceptually:

```text
┌──────────────────────────────────────┐
│            Harmony Core              │
│                                      │
│ chord interpretation                 │
│ key context                          │
│ harmonic functions                   │
│ voicing                              │
│ guide tones                          │
│ voice leading                        │
│ instrument ranges                    │
└──────────────────┬───────────────────┘
                   │
          Host-neutral context
                   │
      ┌────────────┴────────────┐
      │                         │
┌─────▼─────────┐         ┌─────▼────────┐
│ Context       │         │ MIDI / Plug- │
│ Providers     │         │ in Adapters  │
└─────┬─────────┘         └──────────────┘
      │
 ┌────┼──────────────┐
 ▼    ▼              ▼
ARA  MIDI          Manual
```

The voicing engine should consume a neutral data structure and should not care where the harmonic context came from.

Example conceptual interface:

```cpp
struct HarmonicContext
{
    KeySignature key;
    ChordSymbol chord;
    double timelinePosition;
    bool hasKey;
    bool hasChord;
};
```

Potential providers:

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

Do not introduce DAW-specific providers unless a host requires a genuine workaround.

---

## 4. ARA 2 is the first development priority

The first milestone is **not** four-voice harmonization.

Development should begin with an **ARA Context Proof of Concept**.

The purpose of the first plug-in is only to answer:

- Is ARA available in the current host?
- Does the host expose chord data?
- Does the host expose key-signature / tonal-context data?
- Can we track that data across the project timeline?
- Can ARA context coexist cleanly with real-time MIDI processing?
- Which capabilities differ between hosts?

The first test UI can be extremely small:

```text
┌────────────────────────┐
│ Smart Voicing ARA Test │
│                        │
│ ARA:       available   │
│ Position:  Bar 23.2    │
│ Key:       A minor     │
│ Chord:     E7          │
└────────────────────────┘
```

No voicing engine is needed at this stage.

---

## 5. ARA content of interest

The project is particularly interested in ARA harmonic / musical content such as:

- key signatures / tonal context;
- sheet chords / chord data;
- tempo / musical timeline information;
- bar / signature information;
- later, possibly notes or other musical context where useful.

Important architectural assumption:

> ARA defines the content types and integration model, but an individual host may expose only a subset of those capabilities.

Therefore Smart Voicing must use **capability detection**, not assumptions about a specific DAW.

---

## 6. Capability-based behavior

The plug-in should be able to display what the current host actually provides.

Example:

```text
HOST INTEGRATION

ARA 2                 ✓
Chord context         ✓
Key signatures        ✓
Tempo / timeline      ✓

Context source:
● Host
○ MIDI
○ Manual
```

In another host:

```text
HOST INTEGRATION

ARA 2                 ✓
Chord context         —
Key signatures        —

Context source:
○ Host
● MIDI
○ Manual
```

The plug-in must remain useful even if host chord/key data is unavailable.

---

## 7. Intended modes after the ARA proof of concept

### 7.1 Direct mode

User plays four notes.

Smart Voicing distributes them across four voices.

```text
Highest note -> Voice 1
2nd          -> Voice 2
3rd          -> Voice 3
Lowest       -> Voice 4
```

---

### 7.2 Melody Harmonize mode

User plays one melodic line.

Smart Voicing uses the current chord + key context to generate the remaining voices.

Example:

```text
Key: C major
Chord: C7
Melody: D
```

Possible output:

```text
Trumpet    D   = 9
Tenor      Bb  = b7
Trombone   E   = 3
Baritone   C   = root
```

---

### 7.3 Chord mode

User plays a chord.

Smart Voicing distributes / revoices the played chord according to the selected preset and instrument ranges.

---

### 7.4 Context-aware mode

Long-term priority order:

```text
1. Melody note / played material
2. Current chord
3. Current key / tonal context
4. Harmonic function
5. Voice leading
6. Instrument ranges
7. Selected voicing rules
```

The local chord should normally override strict diatonic behavior when needed.

Example:

```text
Key: C major
Chord: A7
```

Smart Voicing should understand that C# may be required even though C# is outside the parent C-major scale.

---

## 8. Planned voicing types

Potential voicing modes:

- Close
- Drop 2
- Drop 3
- Drop 2+4
- Spread
- Unison
- Guide Tones
- Custom

A practical first set after the ARA stage:

```text
Close
Drop 2
Guide Tones
Custom
```

---

## 9. Guide-tone concept

Guide-tone mode is especially important for jazz-oriented arranging.

Possible rules:

- 3rd and 7th receive high priority;
- melody / tensions can remain in the top voice;
- the root may be omitted if bass already establishes it;
- middle and lower voices preserve chord identity with minimal movement.

Example:

```text
Key: C
Chord: G7
Melody: A
```

Possible allocation:

```text
Trumpet    A   = 9
Tenor      F   = b7
Trombone   B   = 3
Baritone   D   = 5
```

---

## 10. Voice leading

Smart Voicing should not rebuild each chord independently.

It should preserve the previous state of each voice and choose the next voicing with sensible movement.

Basic MVP cost function may begin as:

```text
cost =
|voice1_new - voice1_old| +
|voice2_new - voice2_old| +
|voice3_new - voice3_old| +
|voice4_new - voice4_old|
```

Later penalties can be added for:

- voice crossing;
- excessive leaps;
- leaving the playable / comfortable range;
- undesirable doubling;
- poor guide-tone placement;
- disruption of the melodic line.

---

## 11. Instrument ranges

A typical first use case is a four-part horn section:

```text
Voice 1 -> Trumpet
Voice 2 -> Tenor Sax
Voice 3 -> Trombone
Voice 4 -> Baritone Sax
```

But the engine should not hard-code this lineup.

Each voice should eventually support:

- minimum note;
- maximum note;
- comfortable range;
- octave displacement;
- instrument profile.

Potential profiles:

- Trumpet
- Flugelhorn
- Alto Sax
- Tenor Sax
- Baritone Sax
- Trombone
- Horn
- custom user profile

---

## 12. MIDI output and recording workflow

The target concept is one independent generated voice per destination instrument.

Initial routing concept:

```text
Voice 1 -> MIDI Channel 1
Voice 2 -> MIDI Channel 2
Voice 3 -> MIDI Channel 3
Voice 4 -> MIDI Channel 4
```

Example:

```text
Smart Voicing
    │
    ├── CH1 -> Trumpet instrument
    ├── CH2 -> Tenor instrument
    ├── CH3 -> Trombone instrument
    └── CH4 -> Baritone instrument
```

Desired user workflow:

```text
set chords + key
      ↓
choose voicing preset
      ↓
arm destination tracks
      ↓
press Record
      ↓
play melody or chords
      ↓
Smart Voicing generates separate voices
```

Important technical question to test per host:

> Can the DAW directly record the MIDI generated by the plug-in onto multiple armed tracks in one pass?

If not, a later fallback may be needed:

- internal capture;
- commit / render to MIDI;
- drag-and-drop generated MIDI parts;
- another lightweight routing mechanism.

This should not be solved before the ARA proof of concept.

---

## 13. Standalone host is not the initial architecture

Divisimate-style external routing can be useful for live MIDI distribution, but Smart Voicing's key differentiator is direct awareness of DAW harmonic context.

Therefore a standalone application / host is **not** part of the initial implementation.

It may be reconsidered later as an optional routing layer, but not as the source of harmonic truth.

---

## 14. Performance goals

Smart Voicing should be deliberately lightweight.

Early design goals:

- minimal CPU usage;
- minimal memory usage;
- minimal UI overhead;
- real-time-safe MIDI processing;
- avoid unnecessary background threads;
- avoid heavyweight external host processes;
- keep the harmony core independent from GUI and plug-in wrapper code;
- avoid external dependencies unless they clearly reduce risk or development cost.

The first ARA test plug-in should be as small as possible.

---

## 15. Reference products

### Divisimate

Useful reference for:

- live routing;
- voice distribution;
- multi-instrument workflows;
- performance-oriented MIDI splitting.

### Scaler 3

Useful reference for:

- Divisi;
- multi-channel MIDI output;
- voice grouping;
- chord-follow behavior;
- voicing concepts;
- internal harmonic context.

Smart Voicing should not try to reproduce the entire Scaler ecosystem.

The intended distinction is:

> Smart Voicing should use the DAW's own harmonic timeline whenever the host exposes it, rather than forcing the user to maintain a second independent chord timeline inside the plug-in.

---

## 16. Suggested repository architecture

Long-term structure may evolve toward:

```text
Smart-Voicing/
│
├── src/
│   ├── core/
│   │   ├── harmony/
│   │   ├── voicing/
│   │   ├── voice_leading/
│   │   └── instruments/
│   │
│   ├── context/
│   │   ├── IHarmonicContextProvider.h
│   │   ├── ARAContextProvider.cpp
│   │   ├── MidiContextProvider.cpp
│   │   └── ManualContextProvider.cpp
│   │
│   ├── plugin/
│   │   ├── processor/
│   │   └── editor/
│   │
│   └── formats/
│       ├── vst3/
│       └── clap/
│
├── tests/
├── docs/
└── third_party/
```

JUCE may be used as a plug-in / platform abstraction layer, but musical logic should remain independent of JUCE where practical.

---

## 17. Development roadmap

### Stage 0 — ARA research and feasibility

Study and verify:

- official ARA SDK;
- ARA plug-in lifecycle;
- key-signature content;
- sheet-chord content;
- timeline / musical context;
- compatibility with real-time MIDI processing;
- host capability detection.

### Stage 1 — ARA Context Proof of Concept

Build a minimal plug-in that displays only:

```text
ARA availability
Current timeline position
Current key
Current chord
```

Test first in Fender Studio, then in additional ARA-capable hosts where practical.

### Stage 2 — MIDI Router

After ARA feasibility is confirmed:

- MIDI input;
- note tracking;
- four output voices / channels;
- Note On / Note Off correctness;
- sustain handling;
- real-time safety.

### Stage 3 — Chord-aware harmonizer

- single-note melody input;
- current-chord interpretation;
- chord-tone generation;
- Close voicing.

### Stage 4 — Key-aware engine

- current key;
- harmonic function;
- secondary / chromatic dominants;
- chord-vs-key priority.

### Stage 5 — Jazz voicing tools

- Drop 2;
- Guide Tones;
- root omission;
- tensions;
- basic context-aware rules.

### Stage 6 — Voice leading

- previous-voice state;
- minimal-movement search;
- range constraints;
- voice-crossing penalties.

### Stage 7 — Instrument profiles and presets

- editable ranges;
- instrument profiles;
- custom routing / voicing presets.

---

## 18. What not to build yet

Do not expand scope prematurely with:

- standalone application;
- complex visual routing;
- huge preset libraries;
- articulation management;
- expression automation;
- vibrato automation;
- humanization systems;
- orchestral templates;
- networking;
- multi-port infrastructure;
- heavy graphics.

The immediate goal is much narrower:

> Prove that Smart Voicing can reliably obtain useful harmonic context from a host through ARA 2 while remaining architecturally host-agnostic.

---

## 19. Development philosophy

Smart Voicing should not attempt to produce a finished arrangement automatically.

Its job is to create a musically useful, editable arrangement skeleton quickly.

Target workflow after generation:

```text
1. DAW Chord / Key context
2. Melody or chord input
3. Smart Voicing generation
4. Separate MIDI voices
5. Manual note editing
6. Articulations
7. Expression / vibrato
8. Final orchestration
```

The musician remains in control of the final arrangement.

---

## 20. Immediate next task

The next development task is:

> **Build the smallest possible ARA 2 plug-in prototype that can report host capabilities and, where exposed, read the current chord and key / tonal context from the DAW.**

Do not begin the full voicing engine until this foundation has been validated.

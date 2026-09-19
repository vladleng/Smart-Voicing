# Smart Voicing

Experimental MIDI harmonization and voice-allocation plug-in by **Moon River Studio**.

Smart Voicing is intended to turn DAW harmonic context (Chord / Key / timeline) plus incoming MIDI into musically useful multi-voice MIDI output for brass, reeds and other ensemble instruments.

## Core idea

The target workflow is:

```text
DAW Chord + Key context
        +
Incoming MIDI
        +
Voicing preset
        ↓
Smart Voicing
        ↓
Independent generated voices / MIDI channels
```

The plug-in should remain host-agnostic. ARA 2 is treated as one possible source of harmonic context, not as the musical engine itself.

## First milestone

The first milestone is **not** four-part voicing.

The first milestone is an **ARA Context Proof of Concept** that answers:

1. Is ARA available in the host?
2. Can the host expose Key Signature / tonal-context data?
3. Can the host expose Sheet Chord data?
4. Can Smart Voicing follow timeline changes?
5. Can ARA coexist cleanly with real-time MIDI processing?
6. Which capabilities vary by host?

## Current Studio Pro finding

Studio Pro exposes ARA Key Signatures, Sheet Chords, Tempo Entries and Bar Signatures when the plug-in is loaded as an ARA/Event FX instance. An Instrument instance can receive an ARA binding, but Studio Pro does not attach a Musical Context to that Instrument instance.

For version **0.0c** the project therefore uses two lightweight components in one package:

```text
Smart Voicing 0.0c/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

- `Smart Voicing.vst3` — Instrument / MIDI engine.
- `Smart Voicing ARA.vst3` — ARA/Event FX context reader.

On Windows the 0.0c proof uses a tiny named shared-memory bridge between the two VST3 modules. `Smart Voicing ARA` publishes a compact context snapshot and the Instrument reads it without file I/O or host polling from the audio thread.

The first 0.0c bridge test only transfers ARA availability and event counts. Actual chord/key values are the next step after this transport is proven.

## Performance / design principles

- Keep UI minimal.
- Keep MIDI processing real-time safe.
- Avoid unnecessary polling and background work.
- Keep musical logic independent from JUCE, ARA and any specific DAW.
- Prefer capability detection over host-name checks.
- Cache host context away from the audio thread and expose immutable/lightweight state to real-time processing.

## Planned context-provider abstraction

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

Conceptually:

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

## Planned musical modes

- Direct 4 Voice
- Melody Harmonize
- Chord redistribution / revoicing
- Context-aware voicing

Initial useful voicing rules after the ARA proof:

- Close
- Drop 2
- Guide Tones
- Custom

Later:

- Drop 3
- Drop 2+4
- Spread
- Unison
- instrument-aware ranges
- voice leading
- root omission / tension policies

## Reference products

Divisimate and Scaler are useful references, but Smart Voicing is not intended to clone either product. Its differentiator is using the DAW's own harmonic context when the host exposes it.

## Status

Pre-alpha / architecture and feasibility phase. Current working build: **0.0c**.

See [`docs/CONCEPT.md`](docs/CONCEPT.md) for the full concept and roadmap.

## Язык ведения проекта

Все этапы разработки, GitHub Issues, задачи, подзадачи, roadmap и пояснения к ним создаются **на русском языке**, чтобы их было удобно отслеживать и корректировать вручную.

Английский используется только там, где это уместно технически: имена классов и методов, API, названия форматов, термины SDK и код.

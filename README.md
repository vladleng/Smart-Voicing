# Smart Voicing

Smart Voicing is an experimental MIDI harmonization and voice-allocation plug-in project.

The long-term goal is to let a musician work directly from the harmonic context of a DAW project: set the chord progression and key in the DAW, arm several instrument tracks, choose a voicing preset, press Record, and play either a melody or chords while Smart Voicing generates separate musical voices for the destination instruments.

## Core idea

Smart Voicing should not be tied to one DAW. The architecture is intended to be host-agnostic, with ARA 2 used as one possible source of harmonic context.

Conceptually:

```text
DAW harmonic context
  +
MIDI input
  +
voicing / voice-leading rules
  +
instrument ranges
  ↓
independent musical voices
```

The DAW remains the source of truth for project harmony whenever the host exposes the required information.

## Current development focus

The first milestone is **not** four-part voicing.

The project starts with an **ARA Context Proof of Concept** whose only purpose is to verify what harmonic data a host actually exposes to a third-party plug-in.

Initial questions:

- Can the plug-in detect ARA availability?
- Can it read key-signature / tonal-context data?
- Can it read chord data such as ARA sheet chords?
- Can it follow changes across the project timeline?
- Can this coexist cleanly with real-time MIDI processing?
- Which capabilities vary from host to host?

Only after this works reliably should the voicing engine be built on top.

## Design principles

- Host-agnostic core
- Capability-based host integration
- ARA 2 as a context provider, not as a DAW-specific dependency
- Minimal CPU and memory overhead
- Real-time safe MIDI processing
- No heavyweight standalone host as a requirement
- Clear separation between harmonic context, voicing logic, MIDI I/O, and UI
- Graceful fallback when a host does not expose chord or key information

## Planned context sources

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

The voicing engine should consume a neutral harmonic context structure and should not need to know whether that context came from ARA, incoming MIDI, or manual settings.

## Planned user workflow

Target workflow:

1. Create or edit the chord progression in the DAW.
2. Set the project key / tonal context in the DAW.
3. Load Smart Voicing.
4. Choose a voicing mode or preset.
5. Arm the destination instrument tracks.
6. Press Record.
7. Play a melody or chord input.
8. Smart Voicing distributes / generates the required voices in real time.
9. Edit the resulting instrument parts individually.

## Planned voicing features

Later stages may include:

- Direct note distribution
- Melody harmonization
- Close voicing
- Drop 2
- Drop 3
- Spread voicing
- Guide-tone voicing
- Root omission
- Context-aware tensions
- Voice leading
- Instrument ranges
- Octave displacement
- User presets

## Reference products

The project is conceptually adjacent to tools such as Divisimate and Scaler, but the intended distinction is simple:

> Smart Voicing should use the harmonic context of the DAW itself whenever the host exposes it, instead of requiring a second independent chord timeline inside the plug-in.

## Documentation

See [`docs/CONCEPT.md`](docs/CONCEPT.md) for the current product concept, architecture, technical hypotheses, and roadmap.

## Status

**Pre-alpha / architecture and feasibility stage.**

The immediate next step is an ARA 2 context-reading prototype with minimal UI and no voicing engine yet.

# Smart Voicing — Stage 5 concept addendum

Status: **current architecture addendum during Stage 5 / 0.4a → 0.5**

This file records architectural decisions made after the main `docs/CONCEPT.md` was written. Until the stable 0.5 concept consolidation, this addendum **supersedes conflicting preliminary text** in `docs/CONCEPT.md`, especially the old preliminary keyswitch map and the roadmap section that predates Stage 7B Performance Engine.

## 1. Current Stage boundary

```text
Stage 4 — Harmonic meaning
Chord / Key / Function / real Resolution Target / Tension vocabulary
        ↓
Stage 5 — Vertical organization
Voicing Strategy / shape
        ↓
Stage 6 — Continuity
Voice Leading / stable Voice identity
        ↓
Stage 7 — Instrument adaptation
Instrument / Ensemble Profiles / ranges / register
        ↓
Stage 7B — Performance behaviour
Humanization / Expression Distribution
        ↓
Stage 8 — Saved Performance Configurations
```

Core ownership rule:

> Сначала определить, к какому слою относится музыкальная проблема, и исправлять её только в этом слое.

Stage 5 must not re-run Stage 4 harmonic interpretation. Stage 7 must not be used to repair incorrect Stage 5 voicing logic. Stage 7B must not alter harmony/voicing/voice-leading decisions.

## 2. Stage 5 strategy families

### Closed family

```text
Stage 4 Harmonic Candidate Context
        ↓
Closed vertical selected once
        ├ Closed
        ├ Drop 2
        ├ Drop 3
        └ Drop 2+4
```

Invariant:

```text
Closed selected pitch classes
→ Drop transform changes octave/shape only
→ Function / Resolution Target / Tension Policy are not recalculated
```

### Melodic textures

```text
Unison
Octaves
Doubling
```

These organize performer-owned melody across four Voices. They can intentionally contain no harmonic information; therefore `Clean / Color / Rich` may be pitch-identical in these modes.

### Independent strategies

`Spread`, `Quartal`, `Cluster`, `UST` are **not** transformations of Closed.

- Spread = independent bottom-up strategy;
- Quartal = characteristic fourth-structure objective;
- Cluster = density/seconds objective;
- UST = upper-structure triad + support-voice objective.

## 3. Accepted Stage 5 implementation so far

```text
0.4a  Foundation                    ACCEPTED
0.4b  Drop 2                       ACCEPTED
0.4c1 Unison                       ACCEPTED
0.4c2 Octaves                      ACCEPTED
0.4c3 deterministic Doubling       ACCEPTED
0.4c4 Performance Keyswitch Layer  ACCEPTED
0.4d  Drop 3                       ACCEPTED
0.4e  Drop 2+4                     ACCEPTED (Studio Pro, 2026-09-26)
0.4f  Spread                       ACCEPTED (Studio Pro, 2026-09-26)
0.4g  Quartal                      ACCEPTED in Studio Pro (2026-09-26)
0.4h  UST                          initial Studio Pro operation confirmed; broad interval review deferred
0.4i  Cluster                      independent density strategy candidate
```

Current detailed state is maintained in `docs/START-HERE.md` and `docs/Smart-Voicing-Handoff-0.5.md`.

## 4. Stable keyswitch architecture

The preliminary keyswitch map in the original `CONCEPT.md` is historical and must not be used for implementation.

Canonical current map:

```text
MIDI 32 / G#0  → UST       ACTIVE in 0.4h candidate
MIDI 33 / A0   → Cluster   ACTIVE in 0.4i candidate
MIDI 34 / A#0  → Quartal   ACCEPTED
MIDI 35 / B0   → Spread    ACTIVE / accepted

MIDI 36 / C1   → Closed
MIDI 37 / C#1  → Drop 2
MIDI 38 / D1   → Drop 3
MIDI 39 / D#1  → Drop 2+4
MIDI 40 / E1   → Unison
MIDI 41 / F1   → Octaves
MIDI 42 / F#1  → Doubling

MIDI 43 / G1   → Clean
MIDI 44 / G#1  → Color
MIDI 45 / A1   → Rich

MIDI 46 / A#1  → Direct Router
MIDI 47 / B1   → Melody Harmonize
```

MIDI note numbers are the stable contract. DAW octave labels are host-convention labels.

Rules:

- UI, project state and keyswitch mutate the same state;
- no parallel keyswitch-only state;
- control notes are swallowed in Melody Harmonize as defined by the decoder;
- reserved future Voicing slots stay swallowed without mutating state;
- Sound Variations is a host presentation layer only;
- same-sample Voicing + Tension control updates are folded into one final musical decision.

See `docs/STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md`.

## 5. Stage 6 direction — Voice Leading

Stage 6 owns continuity between successive voicings:

```text
previous Voice state
common-tone retention
stepwise motion
Voice identity
voice-crossing penalty
large-leap penalty
strategy-specific continuity objective
register-aware scoring hooks
```

Default direction: **minimum musically necessary motion**, not mandatory parallel/block/soli motion.

See `docs/VOICE-LEADING-DIRECTION.md`.

## 6. Stage 7 direction — Instrument / Ensemble Profiles

Harmony Core does not hardcode `V1 = Trumpet`.

First practical profile can map:

```text
V1 → Trumpet
V2 → Tenor Sax
V3 → Trombone
V4 → Baritone Sax
```

Instrument profiles own:

```text
absoluteRange
practicalRange
comfortableRange
preferredRegister
registerZones
timbral / dynamic / balance metadata
```

Principle:

```text
playable(note) != desirable(note)
```

Register/range corrections belong here, not in Stage 5.

## 7. Stage 7B — Performance Engine

Stage 7B was added after the original concept roadmap and sits **after Instrument Profiles, before Presets**.

Purpose: turn four correct Voices into four slightly independent performers without changing musical decisions.

### Timing Humanization

MVP should be **delay-only**:

- per-Voice small Note On offsets;
- no negative pre-delay;
- no mandatory lookahead/plugin latency;
- Note On/Off coherence;
- deterministic behaviour;
- profile-aware defaults later.

### Expression Distribution

Smart Voicing should expose a common automatable Expression gesture and distribute it per Voice.

The fixed conceptual order is:

```text
Base Expression automation
        ↓
Expression Gain / Trim V1..V4
        ↓
per-Voice gain-adjusted baseline
        ↓
Expression Humanization / response / smoothing
        ↓
Instrument Profile mapping
        ↓
MIDI Ch1..Ch4
```

Four Expression Gains are **stable section balance**. Humanize is **variation around each Voice's already adjusted baseline**.

Example principle:

```text
Base = 100
Bari Gain = 80%
→ Bari baseline = 80
→ Humanize varies around 80, not around 100
```

Thus humanization must never cancel a deliberate per-instrument balance choice.

For the first implementation, variation should preferably be relative/proportional to the post-Gain baseline.

### Deterministic humanization

```text
same MIDI
+ same automation
+ same Performance settings
+ same Seed
= same output
```

No hidden time-seeded randomness.

Full roadmap: `docs/PERFORMANCE-ENGINE-ROADMAP.md`, tracking Issue #31.

## 8. Stage 8 — Presets / Performance Configurations

Stage 8 stores a coherent performance configuration, including as features become available:

- Harmony Mode / Distribution / Voicing Type;
- Instrument / Ensemble Profile;
- Humanize on/off;
- Timing Humanize Amount;
- Expression Humanize Amount;
- Expression Gain/Trim V1–V4;
- smoothing/attack response;
- Humanize Seed.

Example future presets `Tight Brass` vs `Loose Brass` may share harmony/voicing but differ in performance behaviour.

## 9. Realtime and determinism guardrails

- no mutex in audio callback;
- no file I/O in audio callback;
- no dynamic allocation in normal realtime path;
- same full input/state gives same output;
- musical fixes need regressions;
- host-specific acceptance is separate from CI;
- never hide timing/context errors with generic short-note suppression.

## 10. Concept consolidation at stable 0.5

Before Stage 5 is merged/finalized as stable 0.5:

1. merge the relevant parts of this addendum into `docs/CONCEPT.md`;
2. remove/replace the obsolete preliminary keyswitch map;
3. update the main roadmap with Stage 7B;
4. mark accepted Stage 5 strategies explicitly;
5. keep `START-HERE` as the quick operational entry point and `CONCEPT` as the long-term architecture model.

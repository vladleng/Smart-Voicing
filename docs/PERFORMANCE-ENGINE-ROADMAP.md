# Smart Voicing — Performance Engine Roadmap

Status: **future roadmap**  
Placement: **after Stage 7 / stable 0.7, before Stage 8 / Presets**  
Tracking issue: **#31 — Stage 7B: Performance Engine: Humanization + Expression Distribution**

## Roadmap position

```text
Stage 5 — Voicing Strategy
        ↓
Stage 6 — Voice Leading + stable Voice Identity
        ↓
Stage 7 — Instrument / Ensemble Profiles
        ↓
Stage 7B — Performance Engine
        ↓
Stage 8 — Presets / Performance Configurations
```

Performance Engine intentionally comes after Stage 7. Timing/expression variation should belong to stable Voices and real Instrument/Ensemble Profiles; before that point it would be only generic MIDI randomization.

## Goal

Turn four musically correct Smart Voicing parts into four slightly independent performers while keeping one editable source gesture.

The engine does **not** change Chord / Function / Tension / Voicing / Voice Leading decisions. It is a downstream performance layer.

```text
Musical VoiceOutput V1..V4
        +
DAW automation / performance controls
        ↓
Performance Engine
        ↓
Instrument / Ensemble Profile
        ↓
MIDI Ch1 / Ch2 / Ch3 / Ch4
```

## Timing Humanization

First implementation should be conservative and realtime-safe:

- per-Voice Note On timing offsets;
- one Voice may be the timing anchor;
- MVP uses **delay-only** offsets, therefore no negative pre-delay and no plugin lookahead/latency;
- Note On/Note Off handling must remain coherent, with no shortened or stuck notes;
- profile-aware response can later differ for Trumpet / Tenor / Trombone / Bari;
- no mutex, file I/O or dynamic allocation in the audio callback.

True `± timing` with notes arriving before the played event is a separate future decision because it requires lookahead/reported latency.

## Expression Distribution

Smart Voicing should expose a common automatable `Expression` parameter. The user draws one musical curve; Performance Engine distributes it to four independent Voice channels.

Default target can be MIDI CC11, but the mapping must not be hardcoded into Harmony Core. Instrument/Performance Profile may later define the destination control.

```text
Base Expression automation
        ↓
V1 / Ch1
V2 / Ch2
V3 / Ch3
V4 / Ch4
```

## Four Expression Gain / Trim controls

In addition to humanization, provide four independent controls for the **overall Expression strength** of each Voice/instrument:

```text
Expression Gain V1 — default 100%
Expression Gain V2 — default 100%
Expression Gain V3 — default 100%
Expression Gain V4 — default 100%
```

Purpose: section balance at sketch stage. If one instrument, for example Baritone Sax, sticks out of the ensemble, its Expression Gain can be reduced without redrawing the common Expression automation.

Contract:

- neutral/default = `100%`;
- exact min/max range is deferred to implementation tests with real instruments;
- each trim is a plugin-state parameter and should be automatable;
- generic UI labels are V1–V4;
- an active Ensemble Profile may show instrument names such as Trumpet / Tenor / Trombone / Bari while preserving core Voice identity;
- final MIDI values are clamped to `0..127`;
- Expression Gain/Trim controls **balance**, while Expression Humanize controls **variation**. They remain separate parameters.

Conceptual order:

```text
Base Expression automation
        ↓
per-Voice Expression Gain / Trim
        ↓
per-Voice gain-adjusted Expression baseline
        ↓
per-Voice humanization / response / smoothing
        ↓
Instrument Profile mapping
        ↓
MIDI Ch1..Ch4
```

### Important invariant: Humanize starts from the gain-adjusted Voice level

Expression Humanization must **not** re-center all Voices around the original common Expression curve. Each Voice first receives its own `Expression Gain / Trim`; only after that is Humanize applied around that Voice's resulting baseline.

Conceptually:

```text
VoiceExpressionBase = BaseExpression × VoiceExpressionGain
VoiceExpressionOut  = Humanize(VoiceExpressionBase)
```

Example:

```text
Base Expression = 100

V1 Gain 100% → baseline 100 → humanize around 100
V2 Gain  95% → baseline  95 → humanize around 95
V3 Gain  90% → baseline  90 → humanize around 90
V4 Gain  80% → baseline  80 → humanize around 80
```

A deliberately quieter Baritone Voice must remain quieter after Humanize. Humanization may add subtle movement, response and timing differences, but it must not cancel the section balance set by the four Gain/Trim controls.

For the first implementation, Expression variation should preferably be **relative/proportional** to the gain-adjusted baseline rather than a large fixed CC offset. This keeps variation naturally smaller at low dynamic levels and larger at high dynamic levels while preserving the shape and balance of the section.

## Expression Humanization

Future controls can include:

- `Expression Humanize Amount`;
- slight per-Voice deviation around the **post-Gain/Trim per-Voice baseline**;
- response/smoothing differences;
- Attack Variation;
- profile-aware defaults.

Avoid sample-to-sample random jitter. The musical shape of the common Expression curve remains authoritative, while the per-Voice Gain/Trim remains the authoritative balance offset for each performer.

Illustrative only:

```text
Base Expression = 100

V1 Gain 100% → baseline 100 → e.g. 102
V2 Gain  95% → baseline  95 → e.g.  92
V3 Gain  90% → baseline  90 → e.g.  93
V4 Gain  80% → baseline  80 → e.g.  78
```

Exact ranges/defaults are not fixed by this roadmap.

## Determinism

Performance Humanization must preserve the project-wide deterministic-output rule:

```text
same MIDI
+ same automation
+ same Performance settings
+ same Seed
= same output
```

No hidden time-seeded randomness. An optional `Humanize Seed` is explicit and saved in project state.

## Performance Control Bus

Architecture should be wider than CC11 from the start:

```text
Expression
Dynamics
Vibrato
Humanize Amount
        ↓
Performance Engine
        ↓
Instrument Profile
        ↓
per-Voice MIDI / control output
```

Vibrato and articulation generation are not part of the first MVP, but the Performance Engine must not block them later.

## Compatibility with current CC passthrough

Current Smart Voicing routes CC / Expression directly to downstream Voices. Managed Expression must therefore have an explicit ownership policy:

- legacy/direct CC passthrough remains available;
- when managed Expression is active, Performance Engine owns that control stream;
- incoming CC11 may become the source gesture or remain passthrough depending on explicit mode;
- never emit duplicate Expression streams to downstream instruments.

## Stage 8 preset integration

Stage 8 Performance Configurations should store:

- Humanize on/off;
- Timing Humanize Amount;
- Expression Humanize Amount;
- Expression Gain / Trim V1–V4;
- smoothing / attack response policy;
- Humanize Seed;
- Instrument / Ensemble Profile mapping.

This allows performance variants such as `Tight Brass` and `Loose Brass` to share the same harmony/voicing rules while differing only in performance behavior.

## MVP exclusions

- no new harmonic inference;
- no modification of Voicing Strategy decisions;
- no full articulation engine;
- no negative timing/lookahead in the first implementation;
- no nondeterministic playback randomness;
- no attempt to replace final manual orchestration/editing.

## Acceptance direction

A single common Expression gesture plus one melody/harmony source should produce four independent MIDI streams with stable Voice identity, controllable section balance and subtle reproducible timing/expression differences. The result should already sound less mechanical at sketch stage while staying deterministic and easy to edit later.

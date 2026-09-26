# Smart Voicing 0.4c — Unison / Octaves / Doubling test plan

Status: **ACCEPTED — 0.4c1 / 0.4c2 / 0.4c3 accepted in Studio Pro**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4b — Drop 2**

Accepted package line: **Smart Voicing 0.4c**

Next substep: **0.4c4 — Voicing Type Keyswitches** (`docs/TEST-0.4c4.md`)

> Read `docs/MUSICAL-ENGINE-GUARDRAILS.md` before changing musical logic. 0.4c is an orchestration/vertical-organization slice. It must not introduce a second Harmony Core or silently reinterpret Chord / Function / Resolution Target / Tension Policy.

## Why this slice comes now

Unison, octave layouts and simple doubling are among the most common real arranging textures. They therefore come before Drop 3 / Drop 2+4 / Spread.

Architectural distinction:

```text
Closed / Drop family
→ organize harmonic material vertically

Unison / Octaves / Doubling
→ organize performer-owned melodic material across voices
```

A pure melodic texture may intentionally express no vertical chord information. That is not a Harmony Core failure; it is the musical objective of the selected strategy.

## 0.4c logical substeps

### 0.4c1 — Unison ✅ ACCEPTED

Accepted in Studio Pro on 2026-09-23.

Contract:

```text
V1 = melody
V2 = melody
V3 = melody
V4 = melody
```

Confirmed:
- melody remains authoritative;
- four output channels remain independent even when their MIDI note number is identical;
- `Clean / Color / Rich` produce identical Unison pitches by design;
- chord/function/tension material is not invented in pure Unison;
- repeated same-note articulation uses whole-section retrigger semantics;
- result is deterministic;
- Windows Build #354 completed successfully.

### 0.4c2 — Octaves ✅ ACCEPTED

Accepted before moving to 0.4c3 on 2026-09-24.

Approved Stage 5 layout:

```text
V1 = melody
V2 = melody - 12
V3 = melody - 12
V4 = melody - 24
relative offsets = [0, -12, -12, -24]
```

Confirmed contract:
- every active voice preserves melody pitch class;
- V1 remains performer-owned;
- V2/V3 same-note duplicate remains two independent Voice slots;
- `Clean / Color / Rich` do not change Octaves pitch output;
- Chord / Key / Function do not reharmonize pure Octaves;
- out-of-range lower voices stay inactive instead of wrapping;
- register adaptation remains Stage 7;
- repeated same-note Octaves uses whole-section retrigger semantics;
- Windows Build #364 completed successfully.

The exact `[0,-12,-12,-24]` mapping is **project-defined**, not quoted as a universal four-instrument formula from *Modern Jazz Voicings*.

### 0.4c3 — Simple deterministic Doubling ✅ ACCEPTED

Accepted in Studio Pro on 2026-09-24 after user confirmation that Unison, Octaves and Doubling all work as intended.

Approved layout:

```text
V1 = melody
V2 = melody
V3 = melody - 12
V4 = melody - 12
relative offsets = [0, 0, -12, -12]
```

Confirmed contract:
- all sounding voices represent exactly the melody pitch class;
- no chord-tone/tension generation is used;
- V1 remains performer-owned and is never transposed;
- V1/V2 and V3/V4 are intentional duplicates on independent Voice slots/channels;
- `Clean / Color / Rich` do not change the layout;
- Chord / Key / Function changes do not alter pure Doubling pitches;
- no random policy selection;
- if `melody - 12` is below MIDI note 0, V3/V4 stay inactive rather than wrapping;
- instrument-specific comfortable ranges remain Stage 7;
- repeated same-note Doubling uses whole-section retrigger semantics;
- Windows Build #375 completed successfully with package upload.

## Shared invariants — accepted

- [x] prior numeric meanings remain stable: `Closed=0`, `Drop2=1`, `Unison=2`, `Octaves=3`;
- [x] `Doubling=4` is appended rather than inserted;
- [x] V1 melody is never changed by Unison/Octaves/Doubling core;
- [x] duplicate MIDI pitches remain separate VoiceOutput slots by design;
- [x] deterministic playback contract remains unchanged;
- [x] no new harmonic inference is added for melodic textures;
- [x] old Voicing Type values remain load-compatible through the existing state field;
- [x] strategy switching reuses the existing transition planner rather than a parallel engine.

## Tension Level interaction

`TensionLevel` remains one persistent project/performance state. For pure melodic textures:

```text
Unison / Octaves / Doubling
→ Clean / Color / Rich may produce identical pitches
```

This is intentional. Tension Level becomes audible again when a harmonic strategy such as Closed or Drop 2 is selected.

The engine must not invent harmony merely to make the Tension selector audibly different.

## Keyswitch follow-up

The planned follow-up now starts as **0.4c4 — Voicing Type Keyswitches**.

The complete control-map and host acceptance contract live in `docs/TEST-0.4c4.md`. Existing stable Tension keyswitches remain unchanged:

```text
MIDI 43 = Clean
MIDI 44 = Color
MIDI 45 = Rich
```

Voicing Type keyswitches must change the same persistent `VoicingType` state already used by UI/project state; no keyswitch-only state is allowed.

## Not part of accepted 0.4c1–0.4c3

- instrument-specific octave/range adaptation;
- Drop 3;
- Drop 2+4;
- Spread;
- Quartal / Cluster / UST;
- Stage 6 previous-state voice-leading scoring;
- Stage 7 instrument/range-aware placement;
- random variation.

## Acceptance boundary

0.4c1–0.4c3 are complete: Smart Voicing can intentionally choose Unison, Octaves and deterministic paired Doubling as cleanly as 0.4b can choose Drop 2, while preserving the same realtime/state architecture and without leaking orchestration decisions into Harmony Core.

# Smart Voicing 0.4c — Unison / Octaves / Doubling test plan

Status: **IN DEVELOPMENT — 0.4c1/0.4c2 accepted, 0.4c3 host-integration candidate**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4b — Drop 2**

Target build label: **Smart Voicing 0.4c**

Current substep: **0.4c3 — Simple deterministic Doubling**

> Read `docs/MUSICAL-ENGINE-GUARDRAILS.md` before changing musical logic. 0.4c is an orchestration/vertical-organization slice. It must not introduce a second Harmony Core or silently reinterpret Chord / Function / Resolution Target / Tension Policy.

## Why this slice comes now

Unison, octave layouts and simple doubling are among the most common real arranging textures. They therefore come before Drop 3 / Drop 2+4 / Spread in Stage 5.

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

### 0.4c3 — Simple deterministic Doubling — HOST CANDIDATE

Goal: add one simple, common, deterministic paired doubling texture without turning Stage 5 into an Instrument Profile engine.

Approved MVP layout:

```text
V1 = melody
V2 = melody
V3 = melody - 12
V4 = melody - 12
relative offsets = [0, 0, -12, -12]
```

Conceptually:

```text
upper unison pair
↓ one octave
lower unison pair
```

Rules:
- all sounding voices represent exactly the melody pitch class;
- no chord-tone/tension generation is used;
- V1 remains performer-owned and is never transposed;
- V1/V2 and V3/V4 are intentional duplicates on independent Voice slots/channels;
- `Clean / Color / Rich` must not change the layout;
- Chord / Key / Function changes must not alter pure Doubling pitches;
- no random policy selection;
- if `melody - 12` is below MIDI note 0, V3/V4 stay inactive rather than wrapping;
- instrument-specific comfortable ranges remain Stage 7.

Current 0.4c3 implementation:
- [x] `VoicingType::doubling = 4`, preserving prior numeric meanings;
- [x] `buildDoublingVoicing()` implements `[0,0,-12,-12]`;
- [x] Doubling bypasses Closed/Harmony Core;
- [x] duplicate pairs remain separate VoiceOutput slots;
- [x] low-MIDI safety is explicit;
- [x] regressions added for exact layout, pitch-class identity and Harmony/Tension independence;
- [x] core Windows CI #370 green;
- [x] processor/state accept and persist `VoicingType::doubling` in the existing state field;
- [x] `Doubling` exposed in Voicing Type UI;
- [x] repeated same-note Doubling uses whole-section retrigger semantics;
- [x] full-section repeated-note retrigger regression added;
- [ ] latest 0.4c3 host-integration Windows CI green;
- [ ] Studio Pro acceptance.

## Shared invariants

- [x] prior numeric meanings remain stable: `Closed=0`, `Drop2=1`, `Unison=2`, `Octaves=3`;
- [x] `Doubling=4` is appended rather than inserted;
- [x] V1 melody is never changed by Unison/Octaves/Doubling core;
- [x] duplicate MIDI pitches remain separate VoiceOutput slots by design;
- [x] deterministic playback contract remains unchanged;
- [x] no new harmonic inference is added for melodic textures;
- [x] strategy switching continues to reuse the existing transition planner rather than a parallel engine;
- [ ] old projects preserve saved Voicing Type values after Doubling integration in Studio Pro;
- [ ] strategy switching does not create stuck notes in host.

## Tension Level interaction

`TensionLevel` remains one persistent project/performance state. For pure melodic textures:

```text
Unison / Octaves / Doubling
→ Clean / Color / Rich may produce identical pitches
```

This is intentional. Tension Level becomes audible again when a harmonic strategy such as Closed or Drop 2 is selected.

The engine must not invent harmony merely to make the Tension selector audibly different.

## Keyswitch policy for 0.4c

Do **not** assign permanent Voicing Type keyswitch notes during individual mode implementation.

After 0.4c3 is accepted, the next small slice is planned to design the **whole Voicing Type keyswitch block** at once, with reserved space for future Drop 3 / Drop 2+4 / Spread / Quartal / Cluster / UST.

Current stable Tension keyswitches remain unchanged:

```text
MIDI 43 = Clean
MIDI 44 = Color
MIDI 45 = Rich
```

Canonical MIDI note numbers, UI and project state must share one internal state.

## Automated acceptance targets

- [x] Unison exact `[0,0,0,0]` layout;
- [x] Octaves exact `[0,-12,-12,-24]` layout;
- [x] Doubling exact `[0,0,-12,-12]` layout in core;
- [x] duplicate notes remain separate VoiceOutput slots;
- [x] melodic textures do not depend on Chord/Tension in core;
- [x] invalid lower notes do not wrap;
- [x] repeated Doubling notes request whole-section rearticulation for all active duplicated voices;
- [x] processor/state/UI can represent `Doubling` without renumbering prior Voicing Types;
- [ ] switching `Closed ↔ Drop 2 ↔ Unison ↔ Octaves ↔ Doubling` preserves V1 semantics in host;
- [ ] project save/reopen persists Doubling in host;
- [ ] legacy state compatibility remains green in host;
- [ ] latest host-integration CI green.

## Studio Pro acceptance targets — 0.4c3

- [ ] select `Melody Harmonize → Doubling`;
- [ ] normal-range melody records as Ch1=`0`, Ch2=`0`, Ch3=`-12`, Ch4=`-12` relative to V1;
- [ ] Ch1/Ch2 and Ch3/Ch4 duplicates remain independent parts;
- [ ] changing Chord Track while a note is held does not alter Doubling pitches;
- [ ] Clean / Color / Rich all produce the same Doubling pitches;
- [ ] repeated same-pitch melody note rearticulates all active voices cleanly;
- [ ] note-off releases every active voice; no stuck notes;
- [ ] live `Closed → Drop 2 → Unison → Octaves → Doubling → Closed` creates no stale/tiny garbage notes;
- [ ] project save/reopen restores `Doubling`;
- [ ] older projects preserve their original Voicing Type;
- [ ] Tension keyswitch notes remain swallowed;
- [ ] switching back to Closed/Drop 2 restores harmonic Tension behavior;
- [ ] repeated playback is deterministic.

## Not part of 0.4c3

- permanent Voicing Type keyswitch assignments themselves;
- multiple hidden/random doubling variants;
- instrument-specific octave/range adaptation;
- Drop 3;
- Drop 2+4;
- Spread;
- Quartal / Cluster / UST;
- Stage 6 previous-state voice-leading scoring;
- Stage 7 instrument/range-aware placement;
- random variation.

## Acceptance boundary

0.4c is complete when Smart Voicing can intentionally choose Unison, Octaves and the deterministic paired Doubling texture as cleanly as 0.4b can choose Drop 2, while preserving the same realtime/state architecture and without leaking orchestration decisions into Harmony Core.

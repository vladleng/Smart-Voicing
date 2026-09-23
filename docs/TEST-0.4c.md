# Smart Voicing 0.4c — Unison / Octaves / Doubling test plan

Status: **IN DEVELOPMENT — 0.4c1 accepted, 0.4c2 active**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4b — Drop 2**

Target build label: **Smart Voicing 0.4c**

Current substep: **0.4c2 — Octaves core**

> Read `docs/MUSICAL-ENGINE-GUARDRAILS.md` before changing musical logic. 0.4c is an orchestration/vertical-organization slice. It must not introduce a second Harmony Core or silently reinterpret Chord / Function / Resolution Target / Tension Policy.

## Why this slice comes now

Unison and octave doubling are among the most common real arranging textures. They are therefore moved ahead of Drop 3 / Drop 2+4 / Spread in Stage 5.

The important architectural distinction is:

```text
Closed / Drop family
→ organize harmonic material vertically

Unison / Octaves / Doubling
→ organize performer-owned melodic material across voices
```

A pure unison or octave texture may intentionally express little or no vertical chord information. That is not a Harmony Core failure; it is the musical objective of the selected strategy.

## 0.4c logical substeps

### 0.4c1 — Unison ✅ ACCEPTED

Accepted in Studio Pro on 2026-09-23.

Contract:

```text
V1 melody
→ V1/V2/V3/V4 same melodic pitch
```

Confirmed:
- melody remains authoritative;
- four output channels remain independent even when their MIDI note number is identical;
- `Clean / Color / Rich` produce identical Unison pitches by design;
- chord/function/tension material is not invented in pure Unison;
- repeated same-note Unison articulation has whole-section retrigger semantics;
- result is deterministic;
- Windows Build #354 completed successfully, including tests, package preparation and artifact upload.

### 0.4c2 — Octaves — ACTIVE

Goal: distribute the same performer-owned melodic pitch class across an explicit octave layout.

Approved Stage 5 default layout:

```text
V1 = melody
V2 = melody - 12
V3 = melody - 12
V4 = melody - 24

relative offsets = [0, -12, -12, -24]
```

Conceptually:

```text
upper lead
↓ one octave
middle pair in the same octave
↓ one octave
lower anchor
```

This exact mapping is **project-defined**, not quoted as a universal four-instrument formula from the Berklee reference. *Modern Jazz Voicings* supports octave doubling and register-aware orchestration as arranging concepts, but does not prescribe this exact `[0,-12,-12,-24]` mapping for Smart Voicing. The layout is therefore an explicit deterministic Stage 5 orchestration default, while concrete instrument comfortable ranges remain Stage 7.

Rules:
- all sounding voices represent exactly the melody pitch class;
- no new harmonic vocabulary is selected;
- V1 remains performer-owned and is never transposed;
- `Clean / Color / Rich` must not change Octaves pitch output;
- Chord / Key / Function changes must not reharmonize pure Octaves;
- V2 and V3 intentionally share the same MIDI pitch on separate Voice slots / channels;
- instrument-specific comfortable ranges remain Stage 7 Instrument Profiles;
- if a requested lower octave falls below MIDI note 0, that Voice slot is inactive rather than wrapped or silently moved to another octave;
- no hidden range adaptation is introduced in Stage 5;
- exact offsets must be regression-tested and deterministic.

Current 0.4c2 core implementation:
- [x] `VoicingType::octaves` appended after existing values, preserving `Closed=0`, `Drop2=1`, `Unison=2`;
- [x] `buildOctaveVoicing()` implements `[0,-12,-12,-24]`;
- [x] Octaves bypass Closed/Harmony Core exactly like Unison;
- [x] V2/V3 duplicate pitch remains represented as two independent VoiceOutput slots;
- [x] invalid lower MIDI targets remain inactive instead of wrapping;
- [x] regression tests added for layout, pitch-class identity, Tension/Harmony independence and low-range safety;
- [ ] latest 0.4c2 core Windows CI green;
- [ ] processor/state/UI host integration;
- [ ] Studio Pro acceptance.

### 0.4c3 — Simple Doubling

Goal: add a small set of deterministic doubling layouts useful in real arranging without becoming an Instrument Profile engine.

Rules:
- doubling policies operate on melody/octave identity only;
- voice count may intentionally be less than four where the selected policy requires it;
- duplicate pitch classes are intentional and must not be deduplicated by the router;
- no random choice or hidden variation;
- profile/range-specific reassignment remains Stage 7.

## Shared invariants

- [x] `VoicingType` can represent Unison/Octaves without changing old `Closed` / `Drop 2` numeric meaning;
- [x] V1 melody is never changed by Unison/Octaves core;
- [x] duplicate MIDI pitches remain separate VoiceOutput slots by design;
- [x] deterministic playback contract remains unchanged;
- [x] no new harmonic inference is added to implement Unison/Octaves;
- [ ] old 0.4a/0.4b projects continue to load as their saved `Closed` / `Drop 2` values in Studio Pro after Octaves integration;
- [ ] strategy switching does not create stuck notes in host;
- [ ] strategy switching reuses the existing transition planner rather than introducing a parallel engine.

## Tension Level interaction

`TensionLevel` remains one persistent global state because it is part of the project/performance control model. However:

```text
Unison / Octaves
→ melodic orchestration strategy
→ Clean / Color / Rich may legitimately produce identical pitches
```

This is intentional. Tension Level becomes audible again when a harmonic strategy such as Closed or Drop 2 is selected.

The engine must not invent extra harmony merely to make the Tension selector audibly different in a pure unison/octave strategy.

## Keyswitch policy for 0.4c

Do **not** assign permanent Voicing Type keyswitch notes yet.

Reason: after 0.4c the project will finally have enough real strategy types (`Closed`, `Drop 2`, `Unison`, `Octaves`, plus future Drop/Spread/Modern modes) to design the keyswitch map as a coherent block rather than accumulating arbitrary historical note assignments.

Current stable Tension keyswitches remain unchanged:

```text
MIDI 43 = Clean
MIDI 44 = Color
MIDI 45 = Rich
```

A future Voicing Type keyswitch block must use canonical MIDI note numbers and one shared state with UI/project persistence.

## Automated acceptance targets

- [x] Unison generates identical melody pitch on V1–V4;
- [x] duplicate same-note Unison voices remain separate VoiceOutput slots;
- [x] repeated Unison melody notes have a full-section retrigger path;
- [x] Clean/Color/Rich do not alter pure Unison pitch identity;
- [x] Octaves preserve melody pitch class across all active VoiceOutput slots;
- [x] octave offsets match `[0,-12,-12,-24]` exactly in core;
- [x] V2/V3 same-note duplicates remain separate VoiceOutput slots;
- [x] out-of-MIDI-range lower octave does not wrap or mutate V1;
- [x] Clean/Color/Rich and harmonic context do not alter pure Octaves pitch identity in core;
- [ ] switching `Closed ↔ Unison ↔ Octaves ↔ Drop 2` preserves V1 semantics in processor/host;
- [ ] project state persists all implemented `VoicingType` values in host;
- [ ] legacy state compatibility remains green in host;
- [ ] full current 0.4c2 CI remains green.

## Studio Pro acceptance targets

### 0.4c1 Unison ✅
- [x] Unison plays as a four-channel melodic section texture;
- [x] Clean / Color / Rich produce identical Unison pitches.

### 0.4c2 Octaves
- [ ] select `Melody Harmonize → Octaves` from Voicing Type;
- [ ] normal-range melody records as Ch1=`0`, Ch2=`-12`, Ch3=`-12`, Ch4=`-24` relative to V1;
- [ ] Ch2/Ch3 same-pitch duplicates remain independent and do not disappear;
- [ ] changing Chord Track while a note is held does not alter Octaves pitches;
- [ ] Clean / Color / Rich all produce the same Octaves pitches;
- [ ] repeated same-pitch melody note rearticulates the octave section cleanly;
- [ ] note-off releases every active octave voice; no stuck notes;
- [ ] live `Closed ↔ Drop 2 ↔ Unison ↔ Octaves` switching has no stale/tiny garbage notes;
- [ ] project save/reopen restores `Octaves`;
- [ ] old 0.4a/0.4b projects preserve their original Closed/Drop2 selection;
- [ ] Tension keyswitches remain swallowed and do not leak downstream;
- [ ] switching back to Closed/Drop 2 restores normal harmonic Tension behavior;
- [ ] repeated playback is deterministic.

## Not part of 0.4c

- permanent Voicing Type keyswitch map;
- instrument-specific octave/range adaptation;
- Drop 3;
- Drop 2+4;
- Spread;
- Quartal / Cluster / UST;
- Stage 6 previous-state voice-leading scoring;
- Stage 7 instrument/range-aware octave placement;
- random variation.

## Acceptance boundary

0.4c is complete when Smart Voicing can intentionally choose a **melodic section texture** (Unison / Octaves / simple deterministic doubling) as cleanly as 0.4b can choose a harmonic Drop 2 texture, while preserving the same realtime/state architecture and without leaking orchestration decisions into Harmony Core.

# Smart Voicing 0.4c — Unison / Octaves / Doubling test plan

Status: **IN DEVELOPMENT**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4b — Drop 2**

Target build label: **Smart Voicing 0.4c**

Current substep: **0.4c1 — Unison host-integration candidate**

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

### 0.4c1 — Unison

Goal: route the authoritative melody to the section without harmonic generation.

Contract:

```text
V1 melody
→ V1/V2/V3/V4 same melodic pitch
```

Rules:
- melody remains authoritative;
- no chord-tone/tension generation is required;
- `Clean / Color / Rich` state is preserved but pure Unison does not need to change pitch output;
- four output channels remain independent even when their MIDI note number is identical;
- no allocation/locks/I/O in realtime path;
- same input + same state = same output.

Implementation checkpoint:
- [x] `VoicingType::unison` added without changing `Closed=0` / `Drop2=1` state meaning;
- [x] pure `buildUnisonVoicing()` returns four independent active voice slots at the performer melody pitch;
- [x] dispatcher bypasses Closed/Harmony Core for Unison;
- [x] Clean/Color/Rich do not alter Unison pitch output;
- [x] repeated same-note Unison articulation can explicitly retrigger V1–V4 together;
- [x] processor accepts/persists Unison value in existing Voicing Type state field;
- [x] `Unison` exposed in Voicing Type UI;
- [x] UI/build diagnostics identify 0.4c1;
- [x] core regressions green in Windows Build #348;
- [ ] host-integration Windows build green;
- [ ] Studio Pro acceptance.

### 0.4c2 — Octaves

Goal: distribute the same melodic pitch class across octave-related voices.

Rules:
- all sounding voices represent the same melodic pitch class;
- no new harmonic vocabulary is selected;
- V1 remains performer-owned;
- octave placement is a Stage 5 layout decision;
- instrument-specific comfortable ranges are deferred to Stage 7 Instrument Profiles;
- exact default octave layout must be explicit and regression-tested, not inferred ad hoc from chord function.

### 0.4c3 — Simple Doubling

Goal: add a small set of deterministic doubling layouts useful in real arranging without becoming an Instrument Profile engine.

Rules:
- doubling policies operate on melody/octave identity only;
- voice count may intentionally be less than four where the selected policy requires it;
- duplicate pitch classes are intentional and must not be deduplicated by the router;
- no random choice or hidden variation;
- profile/range-specific reassignment remains Stage 7.

## Shared invariants

- [x] `VoicingType` can represent Unison without changing old `Closed` / `Drop 2` numeric meaning;
- [ ] old 0.4a/0.4b projects continue to load as their saved `Closed` / `Drop 2` values in Studio Pro;
- [x] V1 melody is never changed by Stage 5;
- [x] duplicate MIDI pitches remain separate VoiceOutput slots and downstream channels by design;
- [ ] strategy switching does not create stuck notes in host;
- [x] strategy switching reuses the existing transition planner rather than introducing a parallel engine;
- [x] deterministic playback contract remains unchanged;
- [x] no new harmonic inference is added to implement Unison.

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
- [x] duplicate same-note voices do not collapse inside `VoiceOutput`;
- [x] repeated melody notes have a full-section retrigger path for Unison;
- [x] Clean/Color/Rich do not alter pure Unison pitch identity;
- [ ] note-off releases every duplicated voice correctly in host/router;
- [ ] Octaves preserve pitch class exactly across all active voices;
- [ ] octave offsets match the documented default layout exactly;
- [ ] switching `Closed ↔ Unison ↔ Octaves ↔ Drop 2` preserves V1 semantics;
- [ ] project state persists all implemented `VoicingType` values in host;
- [ ] legacy state compatibility remains green in host;
- [ ] full current host-integration CI remains green.

## Studio Pro acceptance targets

### 0.4c1 Unison
- [ ] select `Melody Harmonize → Unison` from Voicing Type;
- [ ] one melody note sounds/records independently on Ch1–Ch4 at the same MIDI pitch;
- [ ] changing Chord Track while the note is held does not alter Unison pitches;
- [ ] Clean / Color / Rich all produce the same Unison pitches;
- [ ] repeated same-pitch melody note rearticulates all four channels cleanly;
- [ ] note-off releases all four duplicated voices; no stuck notes;
- [ ] live `Closed ↔ Drop 2 ↔ Unison` switching has no tiny garbage notes or stale voices;
- [ ] project save/reopen restores `Unison`;
- [ ] old 0.4a/0.4b project restores its original Closed/Drop2 selection;
- [ ] Tension keyswitches remain swallowed and do not leak downstream;
- [ ] switching back to Closed/Drop 2 restores normal harmonic Tension behavior.

### Later 0.4c
- [ ] Octaves sound/record with the documented octave layout;
- [ ] simple doubling policies match their documented layouts;
- [ ] repeated playback is deterministic.

## Not part of 0.4c

- permanent Voicing Type keyswitch map;
- Drop 3;
- Drop 2+4;
- Spread;
- Quartal / Cluster / UST;
- Stage 6 previous-state voice-leading scoring;
- Stage 7 instrument/range-aware octave placement;
- random variation.

## Acceptance boundary

0.4c is complete when Smart Voicing can intentionally choose a **melodic section texture** (Unison / Octaves / simple deterministic doubling) as cleanly as 0.4b can choose a harmonic Drop 2 texture, while preserving the same realtime/state architecture and without leaking orchestration decisions into Harmony Core.

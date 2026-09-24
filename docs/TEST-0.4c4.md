# Smart Voicing 0.4c4 — Voicing Type Keyswitches

Status: **IN DEVELOPMENT — host-integration candidate**

Stage: **5 — Jazz Voicing Engine**

Stable musical base: **0.4c3 — Unison / Octaves / Doubling accepted in Studio Pro**

## Goal

Add realtime MIDI selection of the existing shared `VoicingType` state without creating a second performance-only state system.

The selector must obey the same architecture already accepted for Tension keyswitches:

```text
UI Voicing Type
      +
Project state
      +
MIDI keyswitch
      ↓
one shared requestedVoicingType / activeVoicingType state
```

## Stable canonical MIDI map

Use MIDI note numbers as the contract. DAW octave labels are intentionally not part of the specification.

```text
VOICING TYPE — MIDI 32..42

32  Closed
33  Drop 2
34  Drop 3      RESERVED
35  Drop 2+4    RESERVED
36  Spread      RESERVED
37  Quartal     RESERVED
38  Cluster     RESERVED
39  UST         RESERVED
40  Unison
41  Octaves
42  Doubling

TENSION — existing stable block
43  Clean
44  Color
45  Rich
```

Rationale: the harmonic/modern strategies occupy the lower part of one contiguous Voicing Type block, melodic textures occupy its upper edge, and the already stable Tension block remains immediately above it without renumbering.

## Control-block semantics

In `Melody Harmonize`:

- note-on and note-off for the entire MIDI `32..42` block are control events and are swallowed;
- implemented notes change the same `VoicingType` state used by UI/project persistence;
- reserved `34..39` notes are swallowed but do not mutate state;
- Tension `43..45` remains unchanged and separate;
- a Voicing Type keyswitch never reaches downstream instrument tracks;
- `Direct Router` is not changed by this slice: the Voicing Type selector is irrelevant there, so notes `32..42` keep their ordinary musical/router meaning.

## Realtime switching contract

When a mapped Voicing Type keyswitch arrives while a melody note is held:

1. update `activeVoicingType` and `requestedVoicingType` atomically;
2. keep performer-owned V1 sounding;
3. recompute only the lower voices through the existing transition/reharmonization path;
4. do not create a second teardown/rebuild engine;
5. coalesce same-sample Tension + Voicing controls into one musical refresh;
6. if a melody Note On exists at the same sample, that new melody note must start directly with the newly selected Voicing Type, without an intermediate old-melody transition.

This preserves the existing MIDI hygiene rule established in 0.4a fix.

## Automated acceptance

- [x] current map decodes exactly: `32 Closed`, `33 Drop2`, `40 Unison`, `41 Octaves`, `42 Doubling`;
- [x] future notes `34..39` remain reserved and do not decode to fake/unimplemented enum values;
- [x] Voicing Type block does not collide with Tension `43..45`;
- [x] prior enum numeric meanings remain unchanged;
- [x] processor uses the shared Voicing Type state rather than a keyswitch-only state;
- [x] note-on/off for Voicing control block are swallowed in Melody Harmonize;
- [x] reserved slots are swallowed without state mutation;
- [x] same-sample control refresh is coalesced with existing Tension logic;
- [ ] latest Windows CI green;
- [ ] Studio Pro host acceptance.

## Studio Pro acceptance

1. Select `Melody Harmonize`.
2. Verify MIDI 32 switches UI/state to `Closed`.
3. Verify MIDI 33 switches to `Drop 2`.
4. Verify MIDI 40 switches to `Unison`.
5. Verify MIDI 41 switches to `Octaves`.
6. Verify MIDI 42 switches to `Doubling`.
7. While holding one melody note, switch repeatedly `32 → 33 → 40 → 41 → 42 → 32`; V1 must remain continuous while V2–V4 adopt the selected texture cleanly.
8. Play one mapped Voicing keyswitch at exactly the same timestamp as a new melody note; the new note must use the new mode immediately, without a tiny intermediate voicing.
9. Press reserved notes `34..39`; UI/state must not change and these notes must not appear on downstream instrument tracks.
10. Verify Tension MIDI `43/44/45` still selects `Clean/Color/Rich` and does not alter the Voicing Type selection.
11. Verify Voicing keyswitch note-on and note-off do not record on Ch1–Ch4 downstream tracks.
12. Save/reopen the project after selecting a mode by keyswitch; the same Voicing Type must be restored because the keyswitch changes the existing persistent state.
13. Confirm no stuck notes after rapid keyswitch changes and repeated playback.
14. In `Direct Router`, confirm this slice did not unexpectedly reserve MIDI `32..42` as performance controls.

## Acceptance boundary

0.4c4 is accepted when the five currently implemented Voicing Types can be selected reliably from MIDI in realtime, the UI visibly follows the selection, project persistence remains the same state, reserved future slots are stable, and the existing Tension keyswitch block is unchanged.

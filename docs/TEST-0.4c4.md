# Smart Voicing 0.4c4 fix1 — Voicing Type Keyswitches

Status: **IN DEVELOPMENT — ergonomic remap host candidate**

Stage: **5 — Jazz Voicing Engine**

Stable musical base: **0.4c3 — Unison / Octaves / Doubling accepted in Studio Pro**

## Why fix1

The first 0.4c4 host build confirmed that realtime Voicing Type keyswitches, UI/state synchronization, swallowing and combined Voicing+Tension switching work as intended.

On a 49-key controller, however, placing the most frequently used harmonic voicings below C1 is inconvenient. fix1 keeps the same contiguous canonical MIDI control block `32..42`, but moves the everyday voicings so they begin at C1 in the Studio Pro octave convention used for host testing.

The existing Tension keyswitches remain unchanged.

## Shared-state contract

```text
UI Voicing Type
      +
Project state
      +
MIDI keyswitch
      ↓
one shared requestedVoicingType / activeVoicingType state
```

## Canonical MIDI map — fix1

**MIDI note numbers are authoritative.** The note names below correspond to the current Studio Pro octave convention used in testing and are included only for ergonomics/reference.

```text
VOICING TYPE — MIDI 32..42

32  G#0  UST         RESERVED
33  A0   Cluster     RESERVED
34  A#0  Quartal     RESERVED
35  B0   Spread      RESERVED

36  C1   Closed
37  C#1  Drop 2
38  D1   Drop 3      RESERVED
39  D#1  Drop 2+4    RESERVED
40  E1   Unison
41  F1   Octaves
42  F#1  Doubling

TENSION — unchanged
43  G1   Clean
44  G#1  Color
45  A1   Rich
```

Ergonomic logic:

```text
C1 ... F#1
→ primary/high-frequency Voicing Type controls

G1 ... A1
→ existing Tension block

below C1
→ less frequently used modern/future voicing families
```

Thus the most-used block becomes chromatically contiguous on a 49-key controller:

```text
C1    Closed
C#1   Drop 2
D1    Drop 3
D#1   Drop 2+4
E1    Unison
F1    Octaves
F#1   Doubling
G1    Clean
G#1   Color
A1    Rich
```

## Control-block semantics

In `Melody Harmonize`:

- note-on and note-off for the entire MIDI `32..42` block are control events and are swallowed;
- implemented notes change the same `VoicingType` state used by UI/project persistence;
- reserved notes `32..35` and `38..39` are swallowed but do not mutate state;
- Tension `43..45` remains unchanged and separate;
- a Voicing Type keyswitch never reaches downstream instrument tracks;
- `Direct Router` is not changed by this slice: notes `32..42` retain their ordinary musical/router meaning there.

## Realtime switching contract

When a mapped Voicing Type keyswitch arrives while a melody note is held:

1. update `activeVoicingType` and `requestedVoicingType` atomically;
2. keep performer-owned V1 sounding;
3. recompute lower voices through the existing transition/reharmonization path;
4. do not create a second teardown/rebuild engine;
5. coalesce same-sample Tension + Voicing controls into one musical refresh;
6. if a melody Note On exists at the same sample, that new melody note starts directly with the new Voicing Type.

## Automated acceptance — fix1

- [x] `36 Closed`, `37 Drop2`, `40 Unison`, `41 Octaves`, `42 Doubling` decode exactly;
- [x] `32 UST`, `33 Cluster`, `34 Quartal`, `35 Spread`, `38 Drop3`, `39 Drop2+4` remain reserved and do not decode prematurely;
- [x] whole `32..42` block remains swallowed in Melody Harmonize;
- [x] Tension block remains exactly `43/44/45`;
- [x] prior `VoicingType` enum numeric meanings remain unchanged;
- [x] processor still uses shared Voicing Type state rather than a keyswitch-only state;
- [ ] fix1 Windows CI green;
- [ ] Studio Pro ergonomic remap acceptance.

## Studio Pro acceptance — fix1

1. Select `Melody Harmonize`.
2. Verify `MIDI 36 / C1` → `Closed`.
3. Verify `MIDI 37 / C#1` → `Drop 2`.
4. Verify `MIDI 40 / E1` → `Unison`.
5. Verify `MIDI 41 / F1` → `Octaves`.
6. Verify `MIDI 42 / F#1` → `Doubling`.
7. While holding melody, switch `36 → 37 → 40 → 41 → 42 → 36`; no stuck/tiny notes.
8. Verify reserved `32..35` and `38..39` neither change UI/state nor reach downstream tracks.
9. Verify `43/44/45` still select `Clean/Color/Rich` exactly as before.
10. Put one Voicing keyswitch and one Tension keyswitch at the same timestamp; only one final musical refresh should occur.
11. Save/reopen after a mapped Voicing keyswitch selection and verify the same Voicing Type is restored.
12. Confirm `Direct Router` behavior remains unchanged.

## Acceptance boundary

0.4c4 fix1 is accepted when the five currently implemented Voicing Types are reliably selected from the new C1-centered ergonomic map, the reserved future slots stay inert/swallowed, and the existing Tension block remains unchanged.

# Smart Voicing 0.4b — Drop 2 test plan

Status: **ACCEPTED / COMPLETED**

Accepted in Studio Pro: **2026-09-23**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4a — Stage 5 foundation**

Accepted build label: **Smart Voicing 0.4b**

Next slice: **0.4c — Unison / Octaves / simple doubling**

> Read `docs/MUSICAL-ENGINE-GUARDRAILS.md` before changing harmonic or voicing logic. Drop 2 belongs to Stage 5 and transforms already-correct Stage 4 / Closed material rather than inventing a new harmonic interpretation.

## 0.4b result

The first real alternative `Voicing Type`, **Drop 2**, is accepted.

The implemented contract is:

```text
Stage 4 harmonic interpretation
        ↓
Closed selects pitch classes / tensions once
        ↓
Drop 2 lowers the second voice from the top by one octave
        ↓
no new Function / Target / Tension decision
```

Strong invariant confirmed:

```text
same melody V1
+ same selected pitch classes as Closed
+ same explicit / characteristic / tension semantics
= only octave / vertical-shape change
```

## Accepted implementation contract

- [x] add `VoicingType::drop2`;
- [x] build Closed exactly once inside the Stage 5 dispatcher;
- [x] transform the selected Closed second voice down by exactly 12 semitones;
- [x] preserve performer-owned V1 unchanged;
- [x] keep the transformed lower output slots ordered by sounding pitch from top to bottom;
- [x] use fixed-size storage only; no allocations/locks/I/O in realtime path;
- [x] add regression that Drop 2 preserves the Closed pitch-class multiset;
- [x] add regression that `G7 -> Cm`, Rich keeps the exact Closed tension vocabulary after Drop 2;
- [x] add slash-bass safety regression;
- [x] expose `Drop 2` in `Voicing Type` UI/state;
- [x] package/build as `Smart Voicing 0.4b`.

## Slash-bass safety

An explicit slash bass has higher authority than voicing shape. A literal Drop 2 can place the dropped second voice below the explicit bass, which would violate the accepted Stage 4/0.4a contract.

The accepted 0.4b policy is conservative:

```text
explicit slash bass
+ Drop 2 would introduce a new lower voice
→ keep the accepted Closed vertical unchanged
```

This remains a safe fallback, not a final universal slash/drop policy. A dedicated strategy can be designed later if real arranging cases require Drop 2 around a structural slash bass.

## Sounding voice-slot order

After the octave drop, V2–V4 are assigned in sounding top-down order:

```text
V1 / Ch1 = highest / melody
V2 / Ch2 = next sounding voice
V3 / Ch3 = next sounding voice
V4 / Ch4 = lowest sounding voice
```

This is shape organization only; Stage 6 voice identity/continuity remains deferred.

## Automated acceptance

`SmartVoicingVoicingStrategyTests` verifies:

- [x] Closed dispatcher remains identical to the accepted Closed engine;
- [x] Drop 2 preserves V1;
- [x] Drop 2 preserves exactly the same pitch classes as Closed;
- [x] the original Closed V2 pitch class appears exactly one octave lower in the Drop 2 vertical;
- [x] lower output slots remain sounding top-down;
- [x] target-aware Rich vocabulary is unchanged by the Drop transform;
- [x] explicit slash bass uses the safe Closed fallback;
- [x] `voicingTypeName(Drop 2)` is stable;
- [x] full existing CI remains green.

Final verification build: **Windows Build #339 — success**.

## Studio Pro acceptance

User host testing confirmed:

- [x] Drop 2 produces the expected classic second-voice drop shape across ordinary seventh chords;
- [x] Clean / Color / Rich preserve the same harmonic vocabulary as the accepted Closed material;
- [x] `G7 -> Cm7` Rich retains the corrected `b9 / b13` semantics from 0.4a fix2;
- [x] characteristic `m7b5 b5` remains intact;
- [x] explicit alterations remain authoritative;
- [x] non-chord melody remains performer-owned V1;
- [x] slash-bass cases use the safe Closed fallback;
- [x] repeated results are deterministic;
- [x] 0.4b is explicitly accepted by the user and closed for further development.

## Not part of 0.4b

- Unison / Octaves / simple doubling;
- Drop 3;
- Drop 2+4;
- Spread;
- Stage 6 previous-state voice-leading scoring;
- instrument/range-aware reassignment;
- voicing-type keyswitch map;
- new Harmonic Function or Tension Policy rules.

## Acceptance boundary

0.4b is accepted because Drop 2 demonstrably changes **shape**, not harmonic meaning. Any future case where a Drop-family transform chooses a different tension vocabulary than Closed must be treated as a regression at the strategy boundary rather than retuning Harmony Core.

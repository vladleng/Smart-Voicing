# Smart Voicing 0.4b — Drop 2 test plan

Status: **IN DEVELOPMENT**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4a — Stage 5 foundation**

Target build label: **Smart Voicing 0.4b**

> Read `docs/MUSICAL-ENGINE-GUARDRAILS.md` before changing harmonic or voicing logic. Drop 2 belongs to Stage 5 and must transform already-correct Stage 4 / Closed material rather than inventing a new harmonic interpretation.

## 0.4b goal

Add the first real alternative `Voicing Type`: **Drop 2**.

The core contract is:

```text
Stage 4 harmonic interpretation
        ↓
Closed selects pitch classes / tensions once
        ↓
Drop 2 lowers the second voice from the top by one octave
        ↓
no new Function / Target / Tension decision
```

Strong invariant:

```text
same melody V1
+ same selected pitch classes as Closed
+ same explicit / characteristic / tension semantics
= only octave / vertical-shape change
```

## Initial implementation contract

- [x] add `VoicingType::drop2`;
- [x] build Closed exactly once inside the Stage 5 dispatcher;
- [x] transform the selected Closed second voice down by exactly 12 semitones;
- [x] preserve performer-owned V1 unchanged;
- [x] keep the transformed lower output slots ordered by sounding pitch from top to bottom;
- [x] use fixed-size storage only; no allocations/locks/I/O in realtime path;
- [x] add regression that Drop 2 preserves the Closed pitch-class multiset;
- [x] add regression that `G7 -> Cm`, Rich keeps the exact Closed tension vocabulary after Drop 2;
- [x] add slash-bass safety regression.

## Slash-bass safety in the 0.4b MVP

An explicit slash bass has higher authority than voicing shape. A literal Drop 2 can place the dropped second voice below the explicit bass, which would violate the accepted Stage 4/0.4a contract.

Therefore the initial 0.4b policy is conservative:

```text
explicit slash bass
+ Drop 2 would introduce a new lower voice
→ keep the accepted Closed vertical unchanged
```

This is a safe fallback, not a final universal slash/drop policy. A dedicated strategy can be designed later if real arranging cases require Drop 2 around a structural slash bass.

## Sounding voice-slot order

After the octave drop, V2–V4 are assigned in sounding top-down order. This keeps the existing channel contract coherent:

```text
V1 / Ch1 = highest / melody
V2 / Ch2 = next sounding voice
V3 / Ch3 = next sounding voice
V4 / Ch4 = lowest sounding voice
```

This is shape organization only; Stage 6 voice identity/continuity is still deferred.

## Automated acceptance

The `SmartVoicingVoicingStrategyTests` suite must verify:

- [x] Closed dispatcher remains identical to the accepted Closed engine;
- [x] Drop 2 preserves V1;
- [x] Drop 2 preserves exactly the same pitch classes as Closed;
- [x] the original Closed V2 pitch class appears exactly one octave lower in the Drop 2 vertical;
- [x] lower output slots remain sounding top-down;
- [x] target-aware Rich vocabulary is unchanged by the Drop transform;
- [x] explicit slash bass uses the safe Closed fallback;
- [x] `voicingTypeName(Drop 2)` is stable.

Full existing CI must remain green.

## Host integration still required

Before accepting 0.4b in Studio Pro:

- [ ] expose `Drop 2` in the existing `Voicing Type` UI/state;
- [ ] save/reload `Drop 2` project state while old 0.4/0.4a projects still default to `Closed`;
- [ ] switch `Closed ↔ Drop 2` while a melody is held: V1 must not retrigger; only changed lower voices move;
- [ ] verify Clean / Color / Rich use the same harmonic material in Closed and Drop 2;
- [ ] verify the real `G7 -> Cm7` fix2 case has the same pitch classes in both voicing types;
- [ ] verify characteristic `m7b5 b5`, explicit alterations and non-chord melody remain authoritative;
- [ ] verify slash-bass fallback in Studio Pro;
- [ ] verify no stuck notes or redundant note fragments during live type switching;
- [ ] verify deterministic repeated playback.

## Not part of 0.4b

- Drop 3;
- Drop 2+4;
- Spread;
- Stage 6 previous-state voice-leading scoring;
- instrument/range-aware reassignment;
- voicing-type keyswitch map;
- new Harmonic Function or Tension Policy rules.

## Acceptance boundary

0.4b is accepted only when Drop 2 demonstrably changes **shape**, not harmonic meaning. Any case where Drop 2 chooses a different tension vocabulary than Closed is a regression and must be fixed at the strategy boundary rather than by retuning Harmony Core.

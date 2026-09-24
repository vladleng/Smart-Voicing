# Smart Voicing 0.4e — Drop 2+4

Status: **IN DEVELOPMENT — host candidate integration**

Stage: **5 — Jazz Voicing Engine**

Stable input: **0.4d — Drop 3 ACCEPTED / COMPLETED**

## Goal

Add `Drop 2+4` as the next Closed-family strategy without changing Stage 4 harmonic interpretation or the established Sound Variations map.

```text
Stage 4 harmonic material
        ↓
Closed vertical selected once
        ↓
Drop 2+4 octave/shape transform
        ↓
Stage 6 voice leading later
```

## Musical contract

For a complete four-voice Closed vertical ordered top-down:

```text
Closed:
V1 = top / performer melody
V2 = second voice from top
V3 = third voice from top
V4 = bottom voice

Drop 2+4:
V2 → V2 - 12
V4 → V4 - 12
V1 remains unchanged
V2..V4 are re-sorted into actual sounding top-down order
```

Abstract form:

```text
input  = [V1, V2, V3, V4]
lower  = sortDescending(V2-12, V3, V4-12)
output = [V1, lower[0], lower[1], lower[2]]
```

## Guardrails

- Drop 2+4 does not select new chord tones or tensions.
- Clean / Color / Rich affect the Closed material before this transform.
- V1 melody is immutable.
- Exact Closed pitch-class multiset must be preserved.
- Existing `VoicingType` numeric meanings 0..5 remain frozen; `drop24 = 6` is appended.
- `stateVersion` remains 5 because the serialized layout is unchanged; only the accepted VoicingType range is extended.
- Instrument-specific ranges/register correction remain Stage 7.
- Incomplete four-voice input or MIDI underflow falls back to Closed rather than wrapping or inventing notes.
- Explicit slash bass remains authoritative. Unlike Drop 2/Drop 3, Drop 2+4 lowers Closed V4 itself, so the slash-bass pitch class remains the lowest member and no automatic slash-bass fallback is required when the transform is otherwise valid.

## Keyswitch contract

The accepted Sound Variations map uses:

```text
MIDI 39 / D#1 → Drop 2+4
```

0.4e activates exactly this slot. No remap is introduced.

## Implementation checkpoint

- [x] append `VoicingType::drop24 = 6` without renumbering existing values;
- [x] add pure `transformClosedToDrop24()`;
- [x] dispatcher integration;
- [x] host-neutral regressions for exact V2/V4 octave drops, V1 preservation, pitch-class preservation, sorted lower slots, slash-bass preservation, incomplete/underflow fallback and stable enum values;
- [x] dedicated `SmartVoicingDrop24StrategyTests` target added to CMake/ctest;
- [x] core Windows CI **#411 green**;
- [x] processor/state accepted range extended through `drop24`;
- [x] UI item + diagnostics integrated;
- [x] activate MIDI 39 / D#1 decoder and remove Drop 2+4 from reserved list;
- [x] keyswitch regression updated;
- [x] package renamed to `Smart Voicing 0.4e` / `Smart-Voicing-0.4e-Windows`;
- [ ] latest full Windows host-candidate CI green;
- [ ] Studio Pro acceptance.

## Studio Pro acceptance

1. `D#1 / MIDI 39` selects **Drop 2+4** and UI follows.
2. Closed vs Drop 2+4 on the same melody/chord keeps V1 and exact pitch classes while lowering Closed V2 and V4 one octave.
3. Clean / Color / Rich keep existing Stage 4 harmonic vocabulary.
4. confirmed V7→minor remains target-aware before the Drop transform.
5. explicit slash bass remains the lowest authoritative pitch class after a valid Drop 2+4 transform.
6. switching `Closed ↔ Drop 2 ↔ Drop 3 ↔ Drop 2+4 ↔ Unison ↔ Octaves ↔ Doubling` produces no stuck/tiny notes.
7. `D#1 / MIDI 39` is swallowed as a performance control and never reaches downstream instruments.
8. save/reopen preserves Drop 2+4.
9. existing keyswitch blocks 32..47 keep the accepted map.
10. repeated playback with the same input/state is deterministic.

## Acceptance boundary

0.4e is accepted when Drop 2+4 is a deterministic, host-confirmed Closed-family shape transform, the existing `D#1 / MIDI 39` Sound Variation is functional, and no Stage 4 harmonic semantics or previously accepted performance controls regress.

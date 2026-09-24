# Smart Voicing 0.4d — Drop 3

Status: **ACCEPTED / COMPLETED**

Stage: **5 — Jazz Voicing Engine**

Stable input: **0.4c4 — performance keyswitch layer ACCEPTED / COMPLETED**

Accepted in Studio Pro: **2026-09-24**

## Goal

Add `Drop 3` as the next Closed-family voicing strategy without re-running harmonic interpretation.

Architecture remains:

```text
Stage 4 harmonic material
        ↓
Closed vertical selected once
        ↓
Drop 3 octave/shape transform
        ↓
Stage 6 voice leading later
```

## Musical contract

For a complete four-voice Closed vertical ordered top-down:

```text
Closed:
V1 = first/top voice (performer melody)
V2 = second voice from top
V3 = third voice from top
V4 = fourth/bottom voice

Drop 3:
→ lower the Closed third voice from top (V3) by exactly one octave
→ preserve V1 exactly
→ preserve the exact Closed pitch-class multiset
→ re-sort V2..V4 into sounding top-down order
```

In abstract form:

```text
input  = [V1, V2, V3, V4]
drop   = V3 - 12
lower  = sortDescending(V2, V4, V3-12)
output = [V1, lower[0], lower[1], lower[2]]
```

## Guardrails

- Drop 3 does not select new chord tones or tensions.
- Clean / Color / Rich affect the Closed material before the Drop 3 transform.
- Same complete input/state produces the same result.
- V1 melody is immutable.
- Existing `VoicingType` numeric values 0..4 remain unchanged for project compatibility; `drop3 = 5` is appended.
- `stateVersion` remains 5 because the serialized field layout is unchanged; only the accepted VoicingType range is extended.
- Explicit slash bass keeps higher priority. For the current MVP, Drop 3 falls back to the accepted Closed vertical in slash-bass cases.
- Incomplete four-voice Closed material or MIDI underflow also falls back to Closed rather than inventing/register-wrapping notes.
- Instrument ranges/register adaptation remain Stage 7.

## Keyswitch contract

The established Sound Variations map uses:

```text
MIDI 38 / D1 → Drop 3
```

0.4d activates exactly this slot. No keyswitch remap was introduced.

## Acceptance record

- [x] `VoicingType::drop3 = 5` appended without renumbering existing values;
- [x] pure `transformClosedToDrop3()` implemented;
- [x] dispatcher integration;
- [x] host-neutral regressions: V1 preservation, exact pitch-class vocabulary, exact octave drop, sorted lower slots, slash-bass fallback, underflow/incomplete fallback;
- [x] core Windows CI **#395 green**;
- [x] processor/state accepted range extended through `drop3`;
- [x] UI item + diagnostics integrated;
- [x] MIDI 38 / D1 decoder activated; only Drop 3 removed from reserved list;
- [x] keyswitch regression updated;
- [x] package `Smart Voicing 0.4d` / `Smart-Voicing-0.4d-Windows` built by full Windows CI **#404 green**;
- [x] Studio Pro host acceptance confirmed by user.

## Host-confirmed behaviour

Studio Pro acceptance confirmed:

- `D1 / MIDI 38` selects Drop 3 and UI follows;
- Closed ↔ Drop 3 preserves V1 and the Closed pitch-class vocabulary while applying the Drop 3 shape;
- Clean / Color / Rich continue to use Stage 4 harmonic material;
- existing Voicing, Tension and Harmony Mode keyswitches remain functional;
- switching among accepted voicing types produces no observed stuck/tiny-note problems;
- the established Sound Variations map remains unchanged.

## Acceptance boundary

0.4d is accepted as a deterministic, host-confirmed Closed-family transformation with no observed harmonic-policy regression. The previously reserved `D1 / MIDI 38` Sound Variation is now a stable working Drop 3 control.

Next Stage 5 slice: **0.4e — Drop 2+4** using the already reserved `D#1 / MIDI 39` slot.

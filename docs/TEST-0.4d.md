# Smart Voicing 0.4d — Drop 3

Status: **IN DEVELOPMENT — core first**

Stage: **5 — Jazz Voicing Engine**

Stable input: **0.4c4 — performance keyswitch layer ACCEPTED / COMPLETED**

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
- Clean / Color / Rich continue to affect the Closed material before the Drop 3 transform.
- Same complete input/state must produce the same result.
- V1 melody is immutable.
- Existing `VoicingType` numeric values 0..4 remain unchanged for project compatibility; `drop3` is appended as a new value.
- Explicit slash bass keeps higher priority. If a literal Drop 3 would place another voice below the authoritative slash bass, use the accepted Closed vertical as a safe fallback for this MVP.
- Incomplete four-voice Closed material or MIDI underflow also falls back to Closed rather than inventing/register-wrapping notes.
- Instrument ranges/register adaptation remain Stage 7.

## Keyswitch contract

The existing Sound Variations map already reserves:

```text
MIDI 38 / D1 → Drop 3
```

0.4d activates this existing slot; no keyswitch remap is allowed.

## Implementation order

- [ ] append `VoicingType::drop3` without renumbering existing values;
- [ ] add pure `transformClosedToDrop3()`;
- [ ] dispatcher integration;
- [ ] host-neutral regressions: V1 preservation, exact pitch-class vocabulary, exact octave drop, sorted lower slots, slash-bass fallback, underflow/incomplete fallback;
- [ ] core CI green;
- [ ] processor/state max-range integration;
- [ ] UI item + diagnostics;
- [ ] activate MIDI 38 / D1 decoder, remove only Drop 3 from reserved list;
- [ ] keyswitch regression update;
- [ ] Windows host candidate;
- [ ] Studio Pro acceptance.

## Host acceptance later

At minimum verify:

1. `D1 / MIDI 38` selects Drop 3 and UI follows.
2. Compare Closed vs Drop 3 on the same melody/chord: same pitch classes, third Closed voice lowered one octave, V1 unchanged.
3. Clean / Color / Rich retain their harmonic vocabulary through Drop 3.
4. major/minor II–V–I and confirmed V7→minor retain Stage 4 tension semantics.
5. explicit slash bass safely falls back to Closed for the current MVP.
6. switching `Closed ↔ Drop 2 ↔ Drop 3 ↔ Unison/Octaves/Doubling` causes no stuck/tiny notes.
7. save/reopen preserves Drop 3.

## Acceptance boundary

0.4d is accepted when Drop 3 is a deterministic, host-confirmed Closed-family transformation with no harmonic-policy regressions and the already-reserved `D1 / MIDI 38` Sound Variation becomes functional without changing the established performance map.

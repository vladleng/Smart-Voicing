# Smart Voicing 0.4g — Quartal

Status: **HOST CANDIDATE — local host-neutral tests pass; Windows CI and Studio Pro pending**.

Stable input: **0.4f Spread — accepted in Studio Pro 2026-09-26**.

## Musical contract

Quartal is an independent Stage 5 harmonic strategy, not a Closed or Spread transform.

1. V1 stays exactly as played, even for a non-chord melody note.
2. V2–V4 use only the existing Stage 4 candidate pool. An ordinary triad does not acquire inferred extensions just to make a fourth stack.
3. Adjacent perfect fourths are the primary vertical shape; an augmented fourth or a contextual major third above V2 may be used when the pool permits it. If no pure fourth stack is legal, preserve chord authority instead of inventing notes.
4. Rootless voicings and incomplete chord identity are possible, but losing both 3 and 7 is strongly discouraged. The m7b5 b5, sus identity, altered fifth and explicit b9/#9/#11/b13 are protected. An explicit slash bass occupies the lowest voice.
5. A minor ninth is a strong soft negative, with the accepted Stage 4 explicit/directed exceptions. No previous-voice movement or instrument-specific range adaptation belongs to this slice.
6. Search is bounded, deterministic and allocation-free; if no safe vertical fits the MIDI domain, output V1 only.

These are scoring preferences under the higher-priority Chord and Stage 4 Tension Policy; Clean/Color/Rich need not change every vertical. The separate accepted Spread colour refinement remains tracked in Issue #9.

## Implementation

- `VoicingType::quartal = 8` appended without renumbering 0..7.
- Dedicated `buildQuartalVoicing()`; dispatcher does not call Closed.
- Existing `MIDI 34 / A#0` reservation becomes the active shared-state selector.
- UI/project state clamp extended to 8; earlier projects retain their old enum meanings.
- `SmartVoicingQuartalStrategyTests` covers fourth shape, V1, explicit/slash material, m7b5, Stage 4 vocabulary, triads, fallback and determinism.
- Existing Spread, Drop 3, Drop 2+4, VoicingStrategy and VoicingKeyswitch host-neutral tests pass locally.
- Windows CI: pending.
- Studio Pro: pending `docs/0.4g-HOST-CHECKLIST.md`.

The score makes no guarantee of smooth successive lines: that is Stage 6 Voice Leading. Instrument-specific comfort remains Stage 7.

# Smart Voicing 0.4f — Spread

Status: **HOST CANDIDATE — full Windows CI green; Studio Pro test pending**

Stable input: **0.4e Drop 2+4 — ACCEPTED / COMPLETED** (Studio Pro, 2026-09-26).

## Musical contract

Spread is an independent bottom-up Stage 5 strategy. It is not an octave-expanded Closed voicing and does not transform Closed's selected notes.

1. V1 is the exact played melody, including non-chord notes.
2. V4 is a structural root anchor 17–29 semitones below V1; an explicit slash bass replaces the root as the anchor. If this anchor cannot fit safely in MIDI, output V1 only.
3. V3 and V2 are chosen above V4 from the existing Stage 4 chord/tension candidate pool. Plain triads remain chord-tone-only unless explicitly extended.
4. A whole-vertical score prefers both 3 and 7 when present, protects characteristic chord tones such as m7b5 b5, respects the Stage 4 target-directed tension metadata, and prefers an open but balanced spacing. Minor ninth is a strong soft negative with explicit/directed exceptions.
5. All generated notes descend in sounding order. Same complete input/state produces the same output. No previous-voice movement or instrument range optimization belongs to this slice.

## Core checkpoint

- [x] `VoicingType::spread = 7` appended without changing 0..6 values.
- [x] independent `buildSpreadVoicing()` and dispatcher; no Closed transform or Function/Tension re-analysis.
- [x] tests for root/slash anchor, 3/7, characteristic b5, Stage 4 pool, safe fallback and determinism.
- [x] local host-neutral compilation and tests.
- [x] Windows core CI **#435 / run 36234854971 green**.

Core CI #435 ran before host activation. The previous `B0 / MIDI 35` reservation is activated only in the following host-integration commit; the older #435 artifact is not a Spread host candidate.

## Host checkpoint (after core CI)

- [x] processor/state/UI integrated; prior enum meanings unchanged;
- [x] `B0 / MIDI 35` activated and decoder regression updated;
- [x] full Windows Build **#436 / run 36235433989 green**: VST3 + ARA VST3 + ctest;
- [x] artifact `Smart-Voicing-0.4f-Windows` uploaded (SHA-256 `728ea09669694156fe534a78680ce4aacc8b4d2f049e790355d22ae61cc148b6`);
- [ ] Studio Pro acceptance per `docs/0.4f-HOST-CHECKLIST.md`.

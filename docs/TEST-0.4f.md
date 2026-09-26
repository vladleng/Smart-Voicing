# Smart Voicing 0.4f — Spread

Status: **ACCEPTED — Studio Pro, 2026-09-26**. Clean/Color/Rich refinement deferred.

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
- [x] user supplied a Studio Pro four-part score, marked Clean/Color/Rich, and confirmed the Spread mode itself works (2026-09-26);
- [x] user confirmed MIDI 35 selection/swallowing, live switching hygiene, existing controls, save/reopen and deterministic repeat playback in Studio Pro on 2026-09-26;
- [x] low-note MIDI-domain fallback is covered by core tests; the user waived the extreme-low-register host demonstration because the trumpet reaches its lower playable limit before this artificial MIDI edge case.

## Score review and deferred refinement

The supplied score repeats the same material in Clean, Color and Rich on Cmaj7, major and minor cadences, chromatic dominants and slash chords. The open bottom-up layout and bass support are visible. The three tension levels produce almost identical pitches across these repetitions. This does not invalidate the working Spread shape, but leaves the intended colour contrast unverified.

Potential cause to test: V1 melody plus V4 bass leave two inner voices, and the present spread score strongly favours both 3 and 7. Available/contextual Stage 4 colours often lose to that structural choice. Do not alter Stage 4 meaning or force every chord to differ. Add an explicit regression where a guide is already present in V1 (e.g. confirmed G7 → Cm7 with melody on b7): compare Clean, Color b13 and Rich contextual b9 while preserving chord identity and the root anchor. If the candidate vocabulary is correct but the selected vertical stays identical where colour is musically feasible, refine Spread's vertical objective in a later Stage 5 slice.

User decision: 0.4f host check passed; proceed with the next development item and revisit Clean/Color/Rich differentiation in the next refinement. The extreme-low-register host exercise was waived explicitly; the underlying MIDI safety regression passed in core tests.

# Smart Voicing — Handoff 0.4

## 1. Current state

Stable: **Smart Voicing 0.4**  
Development: **next = 0.4a**  
Stage: **Stage 4 CLOSED → Stage 5 Jazz Voicing Engine**  
Branch at Stage 4 close: `stage-4-key-aware-engine`  
Main Stage 4 Issue: #8  
Next Stage Issue: #9  
Stage 4 PR: #22  
Last accepted 0.3f code/docs HEAD before stable rename: `40922a936e725c297009cb38620a0cd098c0aad7`  
Last 0.3f CI: Windows Build #302 / run `35682905650` — **success**.  
Reference host: Studio Pro.

> После финального 0.4 commit/merge GitHub является источником истины для exact stable commit/CI SHA.

## 2. Confirmed

### Implemented
- ARA → shared harmonic context → Instrument architecture;
- Chord / Key / Tempo / Time Signature / transport;
- Direct Router, 4 voices Ch1–Ch4, sustain/ownership/state;
- ChordModel + slash bass;
- Melody Harmonize = candidate-based Closed Voicing;
- KeyModel + harmonic functions;
- applied/secondary dominant candidate + confirmation;
- modal-interchange candidate MVP;
- exact realtime chord boundaries;
- TensionPolicy + Harmonic Candidate Pool;
- Clean / Color / Rich UI/state;
- Functional Tension Profiles;
- characteristic-tone protection.

### CI confirmed
0.3f Windows Build #302 green, including:
- VST3 + ARA VST3 build;
- Core tests;
- Closed Harmonizer tests;
- Live Reharmonizer tests;
- Key-aware tests;
- Tension Policy tests;
- package upload.

### Studio Pro confirmed
- major `Dm7 | G7 | Cmaj7`;
- minor `Bm7b5 | E7 | Am`;
- `Bm7b5` keeps characteristic b5;
- no-target `A7` differs intentionally from `A7 | Dm`;
- Color uses target-aware inside colour;
- Rich uses stronger functionally-directed tension;
- explicit `Db7b13`, `B7#11` remain authoritative in regression progression;
- user explicitly accepted Stage 4 as complete on 2026-09-22.

## 3. Architecture decisions from this chat

Final Stage 4 pipeline:

```text
Played / Melody
>
Explicit Chord Track
>
Chord identity / characteristic tones
>
Current Key
>
Harmonic Function + REAL next-chord target evidence
>
Functional Tension Profile
>
Tension Level / Tension Policy
>
Voicing Strategy
>
Voice Leading
```

Critical target rule:

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Smart Voicing does **not** infer a future target in 0.4. The arranger writes the progression in Chord Track.

## 4. Musical / arranging decisions

### Tension Level

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour for the real target

Rich
→ functionally intensified tension / altered colour
```

Important:
- level is not a fixed extension list;
- flat/sharp does not automatically mean Rich;
- for `V -> minor`, b13 can be Color;
- b9/#9/#11 can be stronger Rich material;
- explicit chord symbol always outranks inference.

### Characteristic tones

```text
ordinary perfect 5th → often expendable
m7b5 b5 / augmented #5 / sus identity / explicit altered fifth → protected
```

### Voice Leading boundary

Stage 4 chooses **musically justified pitch classes**.  
Stage 5 chooses **vertical strategy/shape**.  
Stage 6 chooses **continuity between successive voicings**.

Default future Voice Leading direction:

```text
minimum musically necessary motion
```

Parallel / Block / Soli is a separate musical policy, not default Closed behavior.

## 5. GitHub / docs synchronized at Stage 4 close

- Issue #8 — Stage 4 completion;
- Issue #25 — Tension Level accepted contract;
- Issue #28 — 0.3e foundation / Functional Tension Profiles;
- Issue #29 — 0.3f final target-aware Color iteration;
- PR #22 — Stage 4 / stable 0.4;
- `docs/CONCEPT.md` — stable 0.4 / current Stage 5;
- `docs/TENSION-LEVELS.md`;
- `docs/FUNCTIONAL-TENSION-PROFILES.md`;
- `docs/VOICE-LEADING-DIRECTION.md`;
- `docs/TEST-0.3f.md` — accepted;
- `docs/TEST-0.4.md` — stable checkpoint;
- `docs/VERSIONING.md`.

## 6. Stage 5 contract already prepared in Issue #9

Start version: **0.4a**. Final Stage 5 checkpoint: **0.5**.

Architecture:

```text
Stage 4 Harmonic Candidate Context
        ↓
VoicingStrategy
├ Closed family
│  ├ Closed
│  ├ Drop 2
│  ├ Drop 3
│  └ Drop 2+4
├ Spread   (independent bottom-up strategy)
├ Quartal
├ Cluster
└ Upper Structure Triad
        ↓
Stage 6 Voice Leading later
```

Important Stage 5 rule:

> VoicingStrategy changes organization/shape of already-correct harmonic material. It must not re-run or reinterpret Stage 4 function/tension logic independently.

## 7. Known bugs / limitations

No blocking Stage 4 bug remains by user acceptance.

Intentional limitations:
- no inferred future target;
- no full previous-state Voice Leading yet;
- no temporal Melodic Context classification yet;
- no instrument-range adaptation yet;
- only Closed is implemented as a full voicing strategy at 0.4;
- enharmonic note naming in Studio Pro piano roll is not harmonic spelling logic.

## 8. Open questions for Stage 5

- exact `VoicingStrategy` interface / context object;
- whether first 0.4a should implement only `Voicing Type + Closed/Drop2` or establish all strategy contracts first;
- exact transformation semantics for Drop 3 / Drop 2+4 with melody-led 4 voices;
- UI naming: `Voicing Type` vs `Voicing Strategy` (UI likely Type, engine Strategy);
- how slash bass should interact with Drop transformations;
- what continuity hints Stage 5 should expose to Stage 6 without implementing Voice Leading itself.

## 9. Deferred ideas

- Stage 6: previous-state Voice Leading;
- Stage 7: Instrument/Ensemble Profiles;
- #23: Melodic Context Engine / approach notes;
- #20/#21: performance profiles and brass keyswitch layer;
- Stage 11: Panic/final hardening.

## 10. Next action

NEXT ACTION:
1. Проверить `main` после merge PR #22 и убедиться, что stable 0.4 CI green.
2. Создать ветку `stage-5-jazz-voicing-engine` от stable 0.4 `main`.
3. Обновить Issue #9 фактическим стартом Stage 5.
4. Начать **0.4a** с `VoicingStrategy` contract + parameter/state `Voicing Type`, оставив `Closed` default strategy.
5. Первый практический новый strategy после contract — **Drop 2**; Spread остаётся отдельным bottom-up strategy, не stretched Closed.

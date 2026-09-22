# Smart Voicing

Smart Voicing — ARA-aware MIDI harmonizer / arranger engine от Moon River Studio.

Текущий стабильный checkpoint: **0.4 — Stage 4: Key-aware Engine + Functional Tensions**.

Основная архитектура и roadmap: [`docs/CONCEPT.md`](docs/CONCEPT.md).

## Компоненты

```text
Smart Voicing.vst3
Smart Voicing ARA.vst3
```

`Smart Voicing ARA` читает harmonic context DAW и публикует его через shared bridge. `Smart Voicing` принимает MIDI, использует Harmony Core и выдаёт четыре независимых MIDI Voice по каналам 1–4.

## Stable 0.4

В 0.4 подтверждены:
- Chord / Key / Tempo / Time Signature через ARA;
- Direct Router и четыре независимых Voice;
- Melody Harmonize / candidate-based Closed Voicing;
- Key-aware harmonic function analysis;
- resolution-aware applied/secondary dominants;
- Functional Tension Profiles;
- Clean / Color / Rich Tension Levels;
- characteristic-tone protection;
- target-aware dominant colour только по реальному следующему Chord Track event;
- explicit Chord Track material выше inference.

Финальное правило Stage 4:

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Следующий этап: **Stage 5 / 0.4a — Jazz Voicing Engine** (`VoicingStrategy`, Closed/Drop family, Spread, Quartal, Cluster, UST).

## Документация

- [`docs/CONCEPT.md`](docs/CONCEPT.md) — canonical architecture / roadmap;
- [`docs/TENSION-LEVELS.md`](docs/TENSION-LEVELS.md) — accepted Clean / Color / Rich semantics;
- [`docs/FUNCTIONAL-TENSION-PROFILES.md`](docs/FUNCTIONAL-TENSION-PROFILES.md) — target-aware functional colour;
- [`docs/VOICE-LEADING-DIRECTION.md`](docs/VOICE-LEADING-DIRECTION.md) — направление Stage 6;
- [`docs/TEST-0.4.md`](docs/TEST-0.4.md) — stable Stage 4 checkpoint;
- [`docs/Smart-Voicing-Handoff-0.4.md`](docs/Smart-Voicing-Handoff-0.4.md) — handoff в следующий чат.

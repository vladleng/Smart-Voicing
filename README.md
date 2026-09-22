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

Подтверждены Key-aware harmonic interpretation, candidate-based Closed Voicing, Functional Tension Profiles, Clean/Color/Rich, characteristic-tone protection и target-aware colour только по реальному следующему Chord Track event.

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Следующий этап: **Stage 5 / 0.4a — Jazz Voicing Engine**.

Документы: `docs/CONCEPT.md`, `docs/TEST-0.4.md`, `docs/Smart-Voicing-Handoff-0.4.md`.

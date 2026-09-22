# Smart Voicing 0.4 — Stable Stage 4 Checkpoint

Статус: **STABLE / ACCEPTED — 2026-09-22**.

0.4 не добавляет новую музыкальную функцию относительно принятой 0.3f. Это стабильный checkpoint, который фиксирует завершение **Stage 4 — Key-aware Engine**.

## 1. Что входит в 0.4

### Harmonic interpretation
- `KeyModel / NormalizedKey`;
- scale degree текущего chord root;
- `Tonic / Predominant / Dominant / Other`;
- `Diatonic / Chromatic`;
- applied/secondary dominant candidate vs confirmed;
- real next Chord Track event как resolution evidence;
- target chord root + quality;
- modal-interchange candidate MVP.

### Closed Voicing
- V1 = performer melody;
- candidate-based V2–V4;
- guide-tone priority;
- contextual root/fifth omission;
- characteristic-tone protection;
- soft Upper Voice Spacing;
- slash bass authority;
- fixed-size realtime-safe search.

### Tension architecture
- `TensionPolicy`;
- `Explicit / Preferred / Available / Contextual / Avoid-as-harmony / Melody-imposed`;
- Harmonic Candidate Pool;
- minor-ninth soft penalty + contextual exceptions;
- `Tension Level`: Clean / Color / Rich;
- `FunctionalTensionProfile`: Neutral / Dominant Unresolved / Dominant→Major / Dominant→Minor.

## 2. Final Stage 4 priority

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

## 3. Final target rule

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Аранжировщик задаёт progression в Chord Track; Smart Voicing интерпретирует её и не сочиняет следующий аккорд.

## 4. Final Tension Level semantics

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour for the real target

Rich
→ functionally intensified tension / altered colour
```

Пример `V -> minor`:
- b13 может быть естественной Color-краской;
- b9/#9/#11 могут быть более напряжёнными Rich candidates;
- natural 13 не становится automatic default только из-за dominant quality;
- explicit `E13`, `E7b9`, `E7#5`, `E7b5` всегда выше inference.

## 5. Characteristic tone policy

```text
ordinary perfect 5th
→ often expendable

m7b5: b5
augmented: #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / protected
```

## 6. Acceptance evidence

Stage 4 подтверждён несколькими итерациями CI + Studio Pro.

Ключевой final pre-release gate:
- 0.3f Windows Build #302 — success;
- major `Dm7 | G7 | Cmaj7` — accepted;
- minor `Bm7b5 | E7 | Am` — accepted;
- `A7` vs `A7 | Dm` — accepted target-evidence behavior;
- user progression `Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7` — accepted;
- пользователь подтвердил завершение Stage 4.

0.4 package должен пройти тот же automated suite без новой musical logic.

## 7. Known boundaries / deferred

Не являются багами 0.4:
- no future-target inference;
- no full previous-state Voice Leading;
- no temporal Melody Role classifier;
- only Closed as current voicing strategy;
- enharmonic spelling в MIDI piano roll не является отдельным notation layer.

Перенесено:
- Jazz Voicing Engine → Stage 5 / #9;
- Voice Leading → Stage 6 / #10;
- Melodic Context Engine → #23;
- Instrument Profiles → Stage 7 / #11.

## 8. Exit

```text
Stage 4 CLOSED
Smart Voicing 0.4 STABLE
PR #22 → merge to main
NEXT: Stage 5 / Smart Voicing 0.4a
```

# Smart Voicing — Functional Tension Profiles

**Status:** Accepted in Smart Voicing 0.4 / Stage 4  
**Related Issues:** #8, #24, #25, #28, #29  
**Related docs:** `TENSION-LEVELS.md`, `VOICE-LEADING-DIRECTION.md`, `TEST-0.3f.md`, `TEST-0.4.md`

## 1. Назначение

Одного правила `Chord + Key + Function → tensions` недостаточно. Две доминанты одинакового chord quality могут требовать разной окраски в зависимости от **реального следующего аккорда**:

```text
G7 -> Cmaj
E7 -> Am
```

Functional Tension Profile определяет смысл inferred colour простого chord symbol до того, как Tension Level и Voicing Strategy выберут конкретную вертикаль.

## 2. Архитектура

```text
Current Chord
 + Current Key
 + Harmonic Function
 + REAL Next Chord / Resolution Target
        ↓
Functional Tension Profile
        ↓
Tension Level
        ↓
Harmonic Candidate Pool / weights
        ↓
Voicing Strategy
        ↓
Stage 6: previous-state Voice Leading
```

Профиль не переписывает Chord Track и применяется только к inferred colour.

## 3. Final target-evidence rule

```text
No next chord
→ no assumed resolution target

Real next chord on expected target root
→ confirmed target root + target quality
→ target-aware profile
```

Smart Voicing интерпретирует progression, заданную аранжировщиком. Stage 4 не сочиняет будущую гармонию.

## 4. Профили

```text
Neutral
Dominant / unresolved
Dominant -> major target
Dominant -> minor target
```

### Neutral
Обычная tonal/modal inference для non-dominant harmony.

### Dominant / unresolved
Dominant chord без подтверждённого real target.

- natural 9/13 могут быть conservative generic Color vocabulary;
- altered candidates могут существовать для Rich;
- altered candidates не получают functionally-directed reward;
- major/minor target не объявляется.

### Dominant -> major target

```text
G7 -> Cmaj7
D7 -> G7
```

Natural 9/13 — типичная inside Color. Rich может использовать более напряжённые alterations, когда real resolution подтверждён и vertical scoring это оправдывает.

### Dominant -> minor target

```text
E7 -> Am
A7 -> Dm
```

Generic Mixolydian не является универсальным default.

```text
b13
→ Preferred / Color candidate
→ functionallyDirected

b9
→ Contextual / Rich candidate
→ functionallyDirected

#9 / #11(b5)
→ Contextual / Rich candidates

natural 13
→ не inferred Color при confirmed minor target
```

Natural 9 может оставаться restrained Color option при поддержке tonal context.

Важно: `b13` является Color не из-за абсолютной «мягкости», а потому что она естественна для данного `V -> minor` context.

## 5. Resolution evidence

Для dominant-quality chord expected target root:

```text
current root + perfect fourth
= +5 semitones
```

Если реальный next Chord Track event имеет этот root:

```text
dominantResolutionConfirmed = true
```

и сохраняются:

```text
dominantTargetPitchClass
dominantTargetQuality
```

Примеры:

```text
D7 -> G     = confirmed target
D7 -> Am    = next chord есть, но это не target D7 → unresolved
A7          = no next chord → unresolved
A7 -> Dm    = confirmed minor target
```

## 6. Tension Level поверх профиля

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour именно данного real target

Rich
→ functionally intensified tension
```

```text
Rich != more random alterations
```

Сначала Function + Real Target определяют смысл tension; затем Level определяет интенсивность.

## 7. Explicit chord выше inference

```text
E7b9
E7#5
E7b5
E13
```

— разные explicit instructions.

```text
Explicit Chord Track
>
Functional Tension Profile
```

Поэтому explicit E13 остаётся E13 даже перед minor target.

## 8. Characteristic tones

```text
ordinary perfect 5th
→ often expendable

half-diminished b5
augmented #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / protected
```

Regression:

```text
Bm7b5 = B D F A
```

`F = b5` сохраняется как identity tone.

## 9. Связь с Voice Leading

Stage 4 формирует правильный functional vocabulary. Stage 6 позже добавит:

```text
common tone
stepwise motion
voice identity
leap penalty
tendency-tone resolution
```

Stage 4 не обязан уже сейчас выбирать лучший temporal path — он обязан дать правильный candidate material.

## 10. Accepted regression cases

```text
Dm7 | G7 | Cmaj7
Bm7b5 | E7 | Am
A7
A7 | Dm
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Studio Pro acceptance 2026-09-22 подтвердил target-aware distinction Color/Rich и отсутствие inferred future target.

## 11. Граница ответственности

0.4 не включает:
- inferred future target;
- full previous-state Voice Leading;
- approach/passing-note classification;
- новые VoicingStrategy beyond Closed.

Фундаментальный вопрос Stage 4 закрыт:

> Какой harmonic colour имеет смысл в реально записанном обороте до того, как Voicing Strategy и Voice Leading выберут конкретное расположение и движение голосов?

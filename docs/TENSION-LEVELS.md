# Smart Voicing — Tension Level

**Status:** Accepted in Smart Voicing 0.4 / Stage 4  
**Related Issues:** #8, #10, #24, #25, #28, #29  
**Reference:** Ted Pease / Ken Pullig — *Modern Jazz Voicings*  
**Functional profile contract:** `docs/FUNCTIONAL-TENSION-PROFILES.md`  
**Voice Leading contract:** `docs/VOICE-LEADING-DIRECTION.md`

`Tension Level` — степень гармонической насыщенности. Конкретный смысл tension определяется harmonic context, а не самим номером уровня.

Это UI/engine abstraction Smart Voicing, а не буквальная терминология книги.

## 1. Архитектура

```text
Chord
 + Key
 + Harmonic Function
 + REAL Next Chord / Resolution Target
        ↓
Functional Tension Profile
        ↓
Tension Level
        ↓
Harmonic Candidate Pool / scoring
        ↓
Voicing Strategy
        ↓
Stage 6: Voice Leading
```

Final Stage 4 rule:

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Smart Voicing не додумывает будущий аккорд. Если нужен target-aware dominant colour, target должен реально присутствовать в Chord Track.

## 2. Три уровня

```text
Level 1 — Clean
structural chord identity first

Level 2 — Color
functionally natural / inside harmonic colour
именно для данного real target

Level 3 — Rich
functionally intensified tension / altered colour,
если это оправдано real target / resolution context
```

Главный принцип:

> Tension Level регулирует интенсивность, а Function + Real Resolution Target определяют смысл цвета.

## 3. Приоритет данных

```text
Played / Melody
>
Explicit Chord Track
>
Chord identity / characteristic tones
>
Key / Function / REAL Resolution Target
>
Functional Tension Profile
>
Tension Level / Tension Policy
>
Voicing Strategy
```

Следствия:
- melody-imposed остаётся V1;
- explicit tension/alteration сохраняется при любом Level;
- `E7b9`, `E7#5`, `E7b5`, `E13` — разные explicit instructions;
- inferred colour не разрушает chord identity;
- отсутствие next chord не даёт права предполагать target.

## 4. Characteristic chord tones

```text
ordinary perfect 5th
→ often expendable

m7b5: b5
augmented: #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / protected
```

Пример:

```text
Bm7b5 = B D F A
```

`F = b5` — structural identity, а не обычная expendable fifth.

## 5. Clean

Цель: ясная chord identity.

- structural chord tones имеют максимальный приоритет;
- guide tones 3/7 защищены;
- characteristic tones защищены;
- ordinary root/fifth могут опускаться по правилам voicing;
- inferred tensions не являются целью;
- explicit tensions authoritative.

## 6. Color

Цель: естественная окраска данной функции и **реального target**, без искусственного повышения tension.

Major target:

```text
G7 -> Cmaj7
```

Natural 9/13 могут быть inside Color.

Minor target:

```text
E7 -> Am
A7 -> Dm
```

Accepted 0.4 contract:

```text
b13
→ Preferred / Color candidate
→ functionallyDirected

natural 13
→ не inferred Color при confirmed minor target
```

Natural 9 может оставаться restrained Color option, если поддерживается tonal context.

Ключевой вывод: знак `b/#` сам по себе не означает Rich. `b13` на `V -> minor` может быть более естественной Color-краской, чем natural 13.

## 7. Rich

Цель: осмысленно усилить напряжение, а не просто открыть больше pitch classes.

Для confirmed minor target:

```text
b9
→ Contextual / Rich candidate
→ functionallyDirected

#9 / #11(b5)
→ Contextual / Rich candidates
```

Для confirmed major target Rich тоже может использовать altered dominant colours, но они не становятся automatic winner только из-за Level 3.

```text
Rich != always altered
```

Если реальный target отсутствует, Rich не притворяется, что знает разрешение.

## 8. Unresolved dominant

```text
A7
```

без следующего chord event:

```text
Functional Profile = Dominant / unresolved
```

- natural 9/13 могут оставаться conservative generic Color vocabulary;
- altered candidates могут быть доступны Rich;
- altered candidates не получают functionallyDirected reward;
- profile не объявляет major/minor target.

Только реальный:

```text
A7 | Dm
```

переводит A7 в `Dominant -> minor target`.

## 9. Explicit chord выше inference

```text
E7b9
E7#5
E7b5
E13
```

Даже если simple `E7 -> Am` не получает inferred natural 13, explicit `E13` остаётся authoritative.

## 10. Internal contract

```text
enum class TensionLevel
{
    clean = 1,
    color = 2,
    rich = 3
};

enum class FunctionalTensionProfile
{
    neutral,
    dominantUnresolved,
    dominantMajorTarget,
    dominantMinorTarget
};
```

`TensionTonePolicy` хранит evidence:

```text
alteredCandidate
functionallyDirected
fromActiveKey
fromFunctionScale
```

## 11. Связь с Voice Leading

Stage 4 отвечает:

> какие pitch classes музыкально оправданы в реально записанном harmonic turn?

Stage 6 / #10 отвечает:

> какие из этих правильных нот дают лучший переход из previous V1–V4?

Accepted future direction:

```text
minimum musically necessary motion
```

## 12. Acceptance references

```text
Dm7 | G7 | Cmaj7
Bm7b5 | E7 | Am
A7
A7 | Dm
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Studio Pro acceptance 2026-09-22 подтвердил:
- Bm7b5 сохраняет b5;
- Color на confirmed minor target использует target-aware vocabulary, включая b13;
- natural 13 не продвигается как inferred Color при confirmed minor target;
- Rich получает более напряжённые directed candidates;
- без next chord target не угадывается;
- explicit altered chords остаются authoritative.

**Итог:** уровень управляет интенсивностью, а реальная progression управляет смыслом tension.

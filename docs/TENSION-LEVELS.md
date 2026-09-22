# Smart Voicing — Tension Level

**Status:** Accepted concept, revised for 0.3f / Stage 4  
**Related Issues:** #8, #10, #24, #25, #28, #29  
**Reference:** Ted Pease / Ken Pullig — *Modern Jazz Voicings*  
**Functional profile contract:** `docs/FUNCTIONAL-TENSION-PROFILES.md`  
**Voice Leading contract:** `docs/VOICE-LEADING-DIRECTION.md`

`Tension Level` — это **степень гармонической насыщенности**, но смысл конкретной tension определяется не самим уровнем, а реальным harmonic context.

Это UI/engine abstraction Smart Voicing, а не буквальная терминология книги.

---

## 1. Архитектура

```text
Chord
 + Key
 + Harmonic Function
 + Real Next Chord / Resolution Target
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

Главное правило 0.3f:

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Smart Voicing не додумывает будущий аккорд. Если аранжировщику нужен target-aware dominant colour, target должен реально присутствовать в Chord Track.

---

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

> `Tension Level` регулирует интенсивность, но **Function + Real Resolution Target определяют смысл цвета**.

---

## 3. Приоритет данных

```text
Played / Melody
    >
Explicit Chord Track
    >
Chord identity / characteristic tones
    >
Key / Function / Real Resolution Target
    >
Functional Tension Profile
    >
Tension Level / Tension Policy
    >
Voicing Strategy
```

Следствия:

- `Melody-imposed` всегда сохраняется как V1;
- explicit tension/alteration из Chord Track сохраняется при любом Level;
- `E7b9`, `E7#5`, `E7b5`, `E13` не сводятся к одному generic `E7 rich`;
- inferred colour не имеет права разрушать chord identity;
- absence of next chord не даёт движку права предположить target.

---

## 4. Characteristic chord tones

```text
ordinary perfect 5th
→ часто expendable

m7b5: b5
augmented: #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / identity tone
```

Пример:

```text
Bm7b5 = B D F A
```

`F = b5` — structural identity, а не обычная expendable fifth.

---

## 5. Level 1 — Clean

Цель: ясная chord identity.

- structural chord tones имеют максимальный приоритет;
- guide tones 3/7 защищены;
- characteristic tones защищены;
- ordinary root/fifth могут опускаться по правилам voicing;
- inferred tensions не являются целью;
- explicit tensions остаются authoritative.

---

## 6. Level 2 — Color

Цель: естественная окраска **данной функции и данного реального target**, без искусственного повышения tension.

### Major-target dominant

```text
G7 -> Cmaj7
```

Natural 9/13 могут быть inside Color.

### Minor-target dominant

```text
E7 -> Am
A7 -> Dm
```

0.3f фиксирует:

```text
b13
→ Preferred / Color candidate
→ functionallyDirected

natural 13
→ не inferred Color при confirmed minor target
```

Natural 9 может оставаться restrained Color option, если его поддерживает текущий tonal context.

Ключевой вывод:

> знак `b/#` сам по себе не означает Rich.

`b13` на `V -> minor` может быть более естественной Color-краской, чем natural 13.

Color не должен автоматически превращать каждый seventh chord в максимально extended harmony.

---

## 7. Level 3 — Rich

Цель: **осмысленно усилить напряжение**, а не просто открыть больше pitch classes.

Для confirmed minor target:

```text
b9
→ Contextual / Rich candidate
→ functionallyDirected

#9 / #11(b5)
→ Contextual / Rich candidates
```

`b13` остаётся доступна, потому что уже является function-aware Color-кандидатом, но Rich получает возможность выбрать более напряжённую tension, если vertical scoring это оправдывает.

Для confirmed major target Rich может использовать altered dominant colours, но они не становятся автоматическим winner только из-за Level 3.

```text
Rich != always altered
```

---

## 8. Unresolved dominant

Если реального следующего chord event нет:

```text
A7
```

получает:

```text
Functional Profile = Dominant / unresolved
```

Движок не выводит Dm из Key/Function.

В unresolved profile:
- natural 9/13 могут оставаться conservative generic Color vocabulary;
- altered candidates могут быть доступны Rich;
- altered candidates не получают `functionallyDirected` reward;
- profile не объявляет major/minor target.

Если пользователь добавляет:

```text
A7 | Dm
```

тогда только этот real target переводит A7 в `Dominant -> minor target`.

---

## 9. Explicit chord всегда выше inference

```text
E7b9
E7#5
E7b5
E13
```

Это разные explicit descriptions.

Даже если simple `E7 -> Am` не получает inferred natural 13, explicit `E13` остаётся authoritative.

---

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

`functionallyDirected` означает, что pitch поддерживается реальным function + resolution target, а не просто является допустимой dominant colour.

---

## 11. Связь с Voice Leading

Stage 4 отвечает:

> какие pitch classes музыкально оправданы в реально записанном harmonic turn?

Stage 6 / #10 отвечает:

> какие из этих правильных нот дают лучший переход из previous V1–V4?

Pipeline:

```text
Functional Tension Profile
+ Tension Level
        ↓
Candidate Pool
        ↓
Voicing Strategy
        ↓
Previous Voice State / Voice Leading
```

Accepted future direction:

```text
minimum musically necessary motion
```

---

## 12. Regression references

### Major

```text
Dm7 | G7 | Cmaj7
```

### Minor

```text
Bm7b5 | E7 | Am
```

### Target evidence

```text
A7
```

vs

```text
A7 | Dm
```

### User progression

```text
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Критерии 0.3f:

- Bm7b5 сохраняет `b5` на всех уровнях;
- Color на confirmed minor target может использовать `b13`;
- natural 13 не продвигается как inferred Color при confirmed minor target;
- Rich получает более напряжённые directed candidates, прежде всего `b9`;
- без next chord target не угадывается;
- explicit altered chords остаются authoritative.

Главный итог: **уровень управляет интенсивностью, а реальная progression управляет смыслом tension**.

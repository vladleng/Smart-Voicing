# Smart Voicing — Tension Level

**Status:** Accepted concept, revised for 0.3e / Stage 4  
**Related Issues:** #8, #10, #24, #25, #28  
**Reference:** Ted Pease / Ken Pullig — *Modern Jazz Voicings*  
**Voice Leading contract:** `docs/VOICE-LEADING-DIRECTION.md`

`Tension Level` — это **степень гармонической насыщенности**, но начиная с 0.3e она не рассматривается отдельно от функции и resolution target.

Это UI/engine abstraction Smart Voicing, а не буквальная терминология книги. Книга даёт музыкальную основу: chord-scale thinking, available tensions, avoid-note semantics, function-aware colour и smooth voice leading.

---

## 1. Почему 0.3d потребовал пересмотра

Studio Pro test 0.3d показал два принципиальных ограничения ранней модели:

1. `Rich` технически открывал altered candidates, но не понимал **зачем** выбирать их в конкретном harmonic turn;
2. generic rule `fifth is expendable` мог удалить `b5` из `m7b5`, хотя эта нота определяет chord identity.

Следствие:

```text
Rich != Color + больше разрешённых pitch classes
```

Правильная архитектура:

```text
Chord
 + Key
 + Harmonic Function
 + Resolution Target / Next Chord
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

---

## 2. Три уровня

```text
Level 1 — Clean
structural chord identity first

Level 2 — Color
functionally natural / inside harmonic colour
без искусственного увеличения tension

Level 3 — Rich
functionally intensified tension / altered colour,
особенно на dominant-function harmony,
если это оправдано target / resolution context
```

Главный принцип:

> `Tension Level` регулирует интенсивность, но **Function + Resolution Target определяют смысл цвета**.

---

## 3. Приоритет данных

```text
Played / Melody
    >
Explicit Chord Track
    >
Chord identity / characteristic tones
    >
Key / Function / Resolution Target
    >
Tension Level / Tension Policy
    >
Voicing Strategy
```

Следствия:

- `Melody-imposed` всегда сохраняется как V1;
- explicit tension/alteration из Chord Track сохраняется при любом Level;
- `E7b9`, `E7#5`, `E7b5`, `E13` не сводятся к одному generic `E7 rich`;
- `Avoid-as-harmony` не означает forbidden melody note;
- Level управляет прежде всего **inferred** colour;
- inferred colour не имеет права разрушать chord identity.

---

## 4. Characteristic chord tones

Ранняя формулировка «fifth обычно первая на omission» слишком общая.

Новая policy:

```text
ordinary perfect 5th
→ часто expendable

m7b5: b5
augmented: #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / identity tone
```

Такая нота получает сильную защиту в scoring и не должна исчезать только потому, что nearby tension делает вертикаль компактнее.

Пример:

```text
Bm7b5 = B D F A
```

`F = b5` — не обычная fifth; удаление F превращает аккорд в другое/неясное звучание.

---

## 5. Level 1 — Clean

Цель: ясная chord identity.

- structural chord tones имеют максимальный приоритет;
- guide tones 3/7 защищены;
- characteristic tones защищены;
- ordinary root/fifth могут опускаться по правилам voicing;
- inferred tensions не являются целью;
- explicit tensions остаются authoritative.

Пример:

```text
Dm7 | G7 | Cmaj7
```

---

## 6. Level 2 — Color

Цель: добавить естественную окраску, **не увеличивая функциональное напряжение без причины**.

- `Preferred / Available` tensions становятся кандидатами;
- natural 9/11/13/6 и modal colours могут заменять менее важные structural tones;
- Color не должен автоматически превращать каждый seventh chord в максимально extended harmony;
- Color должен учитывать реальный target: generic Mixolydian 13 не считается автоматически правильной только потому, что chord = dominant.

Пример major II–V–I:

```text
Dm7/9/11 | G9/G13 | Cmaj9/13
```

Но в minor II–V–I:

```text
Bm7b5 | E7 | Am
```

`E7` не обязан получать `C# = natural 13`, если actual target = `Am` и активный minor context этого не поддерживает.

---

## 7. Level 3 — Rich

Цель: **осмысленно усилить tension**, а не просто открыть список alterations.

Особенно важно для dominant -> target:

```text
G7 -> Cmaj
E7 -> Am
```

Эти доминанты имеют одну dominant function, но не обязаны использовать один tension vocabulary.

### Major-target dominant

Inside Color может включать natural 9/13. Rich получает доступ к более напряжённым `b9/#9/#11/b13`, но они должны иметь resolution evidence.

### Minor-target dominant

Natural 13 не должна автоматически продвигаться из generic Mixolydian baseline.

Направленные Rich candidates могут включать:

```text
b9
b13
#9 / #11 — более контекстно
```

Например:

```text
E7(b9,b13) -> Am

F  -> E
G# -> A
D  -> C
```

То есть Rich усиливает **направление разрешения**.

Важно:

```text
Rich != always altered
```

Если target/function не дают достаточного evidence, Rich может совпасть с Color.

---

## 8. Functional Tension Profile

0.3e вводит allocation-free profile layer:

```text
Neutral
Dominant / unresolved
Dominant -> major target
Dominant -> minor target
```

Профиль выводится из:

- current chord quality;
- active Key;
- Harmonic Function;
- actual next Chord Track event;
- confirmed dominant resolution;
- target chord quality.

Primary и secondary dominant используют один общий resolution mechanism: если следующий chord действительно находится на ожидаемом dominant target root, его quality становится evidence для tension profile.

---

## 9. Explicit chord всегда выше inference

Примеры:

```text
E7b9
E7#5
E7b5
E13
```

Это разные explicit descriptions. Smart Voicing не должен заменять их своим inferred profile.

`Functional Tension Profile` используется прежде всего для **простого chord symbol**, например `E7`, когда нужно решить, какой colour уместен в данном обороте.

---

## 10. Связь с Voice Leading

0.3e отвечает на вопрос:

> какие pitch classes музыкально оправданы в этом harmonic turn?

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

Future Voice Leading должен учитывать common tones, stepwise motion и tendency-tone resolution, но это не блокирует functional candidate selection 0.3e.

---

## 11. Default Voice Leading остаётся отдельным вопросом

Accepted direction:

```text
not maximum parallel motion
not maximum static motion

→ minimum musically necessary motion
```

`Parallel / Block / Soli` — отдельная musical strategy/policy (#23), а не автоматическое следствие Rich.

---

## 12. Internal contract

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

`TensionTonePolicy` дополнительно хранит evidence:

```text
alteredCandidate
functionallyDirected
fromActiveKey
fromFunctionScale
```

`functionallyDirected` означает, что pitch не просто допустима, а поддерживается current function + resolution target.

---

## 13. Regression references

### Major context

```text
Dm7 | G7 | Cmaj7
```

### Minor context

```text
Bm7b5 | E7 | Am
```

Критерии:

- Bm7b5 сохраняет `b5` на всех уровнях;
- Color не вставляет natural 13 в E7 механически из generic Mixolydian;
- Rich может предпочесть functionally directed `b9 / b13` перед Am;
- explicit altered chords остаются authoritative;
- одна и та же simple dominant получает разный inferred colour при разных resolution targets.

Главный итог: **уровень управляет интенсивностью, функция управляет смыслом tension**.

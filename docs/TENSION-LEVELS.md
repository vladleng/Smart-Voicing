# Smart Voicing — Tension Level

**Status:** Accepted concept for 0.3d / Stage 4  
**Related Issues:** #8, #10, #24, #25  
**Reference:** Ted Pease / Ken Pullig — *Modern Jazz Voicings*

`Tension Level` — это не набор обязательных надстроек, а **степень гармонической насыщенности**, регулирующая свободу `Tension Policy` и `Harmonic Candidate Pool`.

Это UI/engine abstraction Smart Voicing, а не буквальная терминология книги. Книга даёт музыкальную основу: chord-scale thinking, available tensions, avoid-note semantics, function-aware color и smooth voice leading.

## Три уровня

```text
Level 1 — Clean
предпочитай chord tones и ясную harmonic identity

Level 2 — Color
разрешай tensions, когда они дают лучший voicing / spacing / voice leading

Level 3 — Rich
цветные и altered candidates получают значительно больше свободы,
если они допустимы Function / Mode / Chord context
```

Ключевой принцип:

> Чем выше Tension Level, тем шире допустимый harmonic candidate pool. Конкретный color выбирается не ради самого факта «сделать аккорд богаче», а если он улучшает общий музыкальный результат.

## Что Tension Level НЕ означает

Неправильно:

```text
Level 2 = обязательно добавить 9
Level 3 = обязательно добавить b9/#9/#11/b13
```

Правильно:

```text
Tension Level
    ↓
регулирует eligibility / weights harmonic color
    ↓
Harmonic Candidate Pool
    ↓
Voicing Strategy
    ↓
Voice Leading выбирает конкретные ноты
```

`Preferred` означает «хороший кандидат», а не «обязательно вставить».

## Приоритет данных

```text
Played / Melody
    >
Explicit Chord Track
    >
Key / Function
    >
Tension Level / Tension Policy
```

Следствия:

- `Melody-imposed` всегда сохраняется как V1;
- explicit tension из Chord Track сохраняется при любом Level;
- Level не имеет права упрощать явно заданный `E7b9` до `E7`;
- `Avoid-as-harmony` не означает forbidden melody note;
- Level управляет прежде всего **inferred** harmonic color.

## Level 1 — Clean

Цель: максимально ясная harmonic identity и чистая вертикаль.

- chord tones получают максимальный вес;
- guide tones 3/7 имеют высокий структурный приоритет;
- root/fifth могут опускаться по обычным правилам voicing;
- inferred tensions обычно не вытесняют structural chord tones только ради color;
- plain triads остаются conservative;
- explicit tensions и melody authority сохраняются.

Концептуальный пример:

```text
Dm7 | E7 | Am7
```

## Level 2 — Color

Цель: умеренный harmonic color, оправданный общим voicing и continuity.

- `Preferred / Available` tensions становятся полноценными кандидатами;
- tension может вытеснить root/fifth, если итоговая вертикаль лучше;
- полезными причинами считаются compactness, spacing, common tone, stepwise movement и harmonic clarity;
- не существует правила «каждый seventh chord превратить в 9/13»;
- chromatic/altered color остаётся консервативным без достаточного Function/Mode evidence.

Концептуальный пример:

```text
Dm6 | E9 / E7 color | Am7
```

## Level 3 — Rich

Цель: расширенный harmonic color с сохранением функции.

- увеличивается доступность/вес `Contextual` candidates;
- functionally justified alterations получают больше свободы;
- b9/#9/#11/b13/#5 и другие colors допустимы только если поддерживаются Chord / Function / Mode / Resolution;
- altered candidate не используется автоматически только потому, что Level = 3;
- guide tones и harmonic identity важнее количества tensions.

Концептуальный пример:

```text
Dm6/9 | E7alt | Am7/9...
```

## Связь с Voice Leading

Tension Level проектируется сразу с учётом Stage 6 Voice Leading.

Принцип:

```text
Tension Policy + Tension Level
        ↓
Harmonic Candidate Pool
        ↓
Voicing Strategy
        ↓
Voice Leading
```

Level 2 особенно полезен, когда tension создаёт более плавную линию или common tone. Level 3 расширяет выбор altered tendency tones и разрешений.

Пример `E7 -> Am`:

```text
F  -> E
G# -> A
D  -> C
```

Если b9 `F` допустима контекстом, Level 3 может предпочесть её именно потому, что она естественно разрешается в `E`.

В 0.3d допустимо provisional static weighting внутри Closed. Полноценный выбор tension с учётом `previous Voice state` относится к Stage 6 / Issue #10.

## Internal contract

Предпочтительная модель:

```text
enum class TensionLevel
{
    clean = 1,
    color = 2,
    rich = 3
};
```

Ориентировочный scoring contract:

```text
Role                 L1        L2        L3
Chord Tone           strong    strong    strong
Guide Tone           v.strong  v.strong  v.strong
Preferred            weak      medium    strong
Available            weak      medium    medium+
Contextual            off/low   low       medium+
Altered contextual    off       very low  enabled by context
Avoid-as-harmony      reject    reject    reject*
Explicit              always    always    always
Melody-imposed        preserve  preserve  preserve
```

`*` кроме explicitly/functionally defined exceptions, например dominant b9.

## Acceptance direction

Одна и та же progression должна давать различную степень harmonic color без смены `VoicingStrategy`:

```text
Level 1: чистая функциональная вертикаль
Level 2: умеренный color при хорошем voicing/continuity
Level 3: rich/altered color при сохранении функции
```

Главный критерий: Level расширяет музыкальную свободу, а не механически увеличивает количество tensions.

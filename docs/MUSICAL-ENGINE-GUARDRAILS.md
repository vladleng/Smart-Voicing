# Smart Voicing — Musical Engine Guardrails

**Status:** обязательные правила дальнейшей разработки  
**Applies to:** Harmony Core, Voicing Strategy, Voice Leading, MIDI transition logic, UI/performance control  
**Stable reference:** 0.4 / Stage 4  
**Current line:** Stage 5 / 0.4a… → 0.5

Этот документ нужен для того, чтобы последующие чаты, агенты и разработчики не ломали уже выстроенную логику Smart Voicing локальными «улучшениями». Музыкальный движок будет ещё многократно уточняться по реальному репертуару, но исправления должны происходить в правильном слое и сопровождаться regression-тестами.

Перед любым изменением музыкальной логики нужно прочитать:

1. `docs/CONCEPT.md`;
2. этот файл;
3. текущий `docs/TEST-<version>.md`;
4. Issue текущего Stage;
5. затрагиваемый код и существующие тесты.

---

## 1. Главный принцип

> Сначала определить, к какому слою относится музыкальная проблема, и исправлять её только в этом слое.

Не использовать локальный symptom-fix, если реальная причина находится выше или ниже по pipeline.

Базовая цепочка:

```text
DAW / Played Material
        ↓
Harmonic Context
        ↓
Harmonic Function + Real Resolution Target
        ↓
Functional Tension Profile / Tension Policy
        ↓
Harmonic Candidate Pool
        ↓
Voicing Strategy
        ↓
Voice Leading
        ↓
Instrument / Ensemble Profile
        ↓
Performance / MIDI Routing / UI
```

Каждый следующий слой может организовывать данные предыдущего слоя, но не должен заново переопределять его ответственность.

---

## 2. Ответственность слоёв

### ARA / Harmonic Context Provider

Отвечает за получение фактов от DAW:

- Current Chord;
- Current Key;
- Tempo / Time Signature;
- transport / PPQ;
- реальные chord boundaries.

Не должен содержать musical voicing logic.

### Stage 4 — Harmonic Interpretation

Отвечает на вопрос:

> Какие pitch classes и tensions функционально оправданы в текущем реально записанном harmonic turn?

Здесь находятся:

- ChordModel;
- KeyModel;
- Harmonic Function;
- real Resolution Target;
- characteristic-tone identity;
- Functional Tension Profile;
- TensionPolicy;
- Clean / Color / Rich;
- Harmonic Candidate Pool.

Stage 4 не должен выбирать Drop 2, Spread, Quartal и т. п.

### Stage 5 — Voicing Strategy

Отвечает на вопрос:

> Как вертикально организовать уже гармонически корректный материал?

Здесь находятся:

- Closed;
- Drop 2 / Drop 3 / Drop 2+4;
- Spread;
- Quartal;
- Cluster;
- UST.

Voicing Strategy не должна заново определять Harmonic Function, Resolution Target или собственную независимую TensionPolicy.

### Stage 6 — Voice Leading

Отвечает на вопрос:

> Как минимально и музыкально оправданно перейти от предыдущего voicing к следующему?

Здесь находятся:

- common-tone retention;
- stepwise rewards;
- leap/crossing penalties;
- Voice identity;
- continuity;
- minimum musically necessary motion.

Не исправлять плохой Voice Leading изменением harmonic vocabulary Stage 4, если гармонический материал сам по себе корректен.

### Stage 7 — Instrument / Ensemble Profiles

Отвечает за:

- practical/comfortable range;
- register zones;
- balance;
- timbral suitability;
- ensemble-specific preferences.

Harmony Core не должен hardcode-ить конкретные инструменты.

### Performance / UI / Keyswitch / Routing

Отвечает за управление уже существующим state и безопасную MIDI-доставку.

Правила:

- UI и keyswitch должны менять один и тот же internal state;
- не создавать parallel state systems;
- UI не определяет harmonic logic;
- keyswitch notes должны быть swallowed там, где являются control events;
- MIDI/router fixes не должны незаметно менять музыкальный смысл Harmony Core.

---

## 3. Неприкосновенные архитектурные инварианты

Без отдельного архитектурного решения не менять следующие правила:

```text
Played Melody
> Explicit Current Chord
> Chord identity / characteristic tones
> Current Key
> Harmonic Function + REAL Resolution Target
> Functional Tension Profile / Tension Level
> Voicing Strategy
> Voice Leading
> Instrument / Ensemble Profile
```

Дополнительно:

- Melody в melody-led mode остаётся авторитетным V1;
- Key и Function интерпретируют Chord, но не переписывают explicit Chord;
- explicit chord tensions выше inferred tensions;
- no next chord = no assumed future resolution target;
- target-aware profile опирается только на реальный следующий Chord Track event;
- characteristic tone нельзя выбрасывать как обычную expendable fifth;
- melodic tension != harmonic tension;
- Stage 5 меняет shape/organization, а не смысл Chord/Function/Tension;
- Stage 6 не должен заново выбирать harmonic function;
- одинаковый полный musical input + одинаковый state должны давать одинаковый результат; скрытая случайность в Harmony Core / Voicing Strategy / Voice Leading запрещена;
- ARA является provider, а не музыкальным фундаментом;
- realtime path: без mutex, file I/O и динамических allocation в audio callback.

---

## 4. Какие изменения допустимы как маленький fix

### 4.1 Local musical tuning

Обычно безопасный класс правок:

- candidate weights;
- soft penalties;
- preference конкретной tension;
- characteristic-tone weight;
- spacing preference;
- contextual omission weight.

Такие изменения допустимы небольшими fix-коммитами, если они не меняют ответственность слоя.

### 4.2 Musical rule change

Требует отдельного regression case и проверки соседних сценариев:

- новая трактовка dominant resolution;
- изменение characteristic-tone policy;
- modal interchange semantics;
- изменение роли explicit tensions;
- изменение Clean / Color / Rich semantics.

Такую правку нельзя считать «просто коэффициентом», если она меняет смысл множества progression.

### 4.3 Architectural contract change

Не делать как маленький fix без отдельного обсуждения/Issue:

- strategy сама заново определяет Function;
- Voice Leading меняет TensionPolicy;
- UI создаёт отдельный musical state;
- ARA-specific код проникает в Harmony Core;
- один слой начинает компенсировать баг другого;
- общий `buildVoicing()` превращается в набор несвязанных исключений для разных режимов.

---

## 5. Обязательный workflow для любого musical fix

```text
Real musical case
        ↓
Reproduce minimally
        ↓
Bug or correct response to input/context?
        ↓
Find owning layer
        ↓
Minimal local change
        ↓
Regression test
        ↓
Full existing test suite / CI
        ↓
Studio Pro host check when relevant
        ↓
Update TEST / Issue / handoff
```

Перед исправлением обязательно отличить:

1. настоящий bug;
2. ожидаемую реакцию на реально полученный Chord/Key/MIDI timing;
3. новую желаемую musical feature.

Не превращать пункт 2 в bugfix только потому, что записанный MIDI выглядит неожиданно.

---

## 6. Не маскировать симптом фильтром

Нельзя автоматически добавлять фильтр вроде:

```text
remove notes shorter than X ms
```

только потому, что в DAW появились микроноты.

Сначала нужно понять источник событий. Фильтр может удалить музыкально валидные staccato/grace/approach notes и скрыть архитектурную проблему.

Предпочтение:

```text
найти причину
→ исправить ownership / transition / harmonic decision
→ сохранить валидный MIDI
```

---

## 7. Regression cases — часть музыкальной спецификации

Каждый найденный важный музыкальный нюанс после исправления должен становиться test case.

Пример:

```text
Key = A minor
Progression = E7 → Am
Tension Level = Color
Melody = C5

Expected:
- E7 recognised as dominant with real Am target;
- target quality = minor;
- appropriate minor-directed colour remains available;
- explicit material remains authoritative;
- V1 melody is not rewritten.
```

Regression test фиксирует не конкретную реализацию алгоритма, а музыкальный контракт, который нельзя случайно потерять при следующем refactor.

---

## 8. 0.4a fix — важный пример правильного разделения причин

Во время теста пользователь увидел короткие MIDI-ноты около chord boundary. Проверка показала, что исходная melody note начиналась немного раньше нового Chord Track event.

Фактическая последовательность была корректной:

```text
Melody starts before boundary
→ previous chord is still current
→ Smart Voicing builds previous-chord voicing

Real Chord Track boundary arrives
→ new chord becomes current
→ V2–V4 reharmonize
```

Это **не bug гармонического движка**. Плагин правильно следовал реальному timeline.

Не вводить автоматически:

- chord lookahead;
- anticipation window;
- boundary tolerance;
- «прилипание» ранней melody к будущему chord.

Если такая функциональность понадобится для живой игры, она должна быть оформлена как отдельная осознанная feature с собственным musical contract.

При этом сделанный `0.4a fix` остаётся полезным, потому что исправляет независимую проблему MIDI-transition hygiene:

- соседние melody events на одной sample-position обрабатываются атомарно;
- новый melody note не требует полного teardown quartet;
- общие V2–V4 не получают бессмысленный Off → On;
- меняются только реально изменившиеся voices;
- V1 остаётся performer-owned.

То есть найденный визуальный артефакт оказался не исходным bug, но refactor устранил реальный будущий класс проблем.

---

## 9. Детерминированность музыкального результата

Smart Voicing должен оставаться детерминированным arranger engine.

Базовый contract:

```text
одинаковая полная последовательность MIDI
+ одинаковые Chord / Key / timeline данные
+ одинаковые Tension / Voicing / Profile settings
+ одинаковое начальное состояние
= одинаковый результат
```

Для текущего Closed Engine одинаковый input/context должен приводить к одному и тому же выбранному voicing. При равном score tie-break также должен быть стабильным и воспроизводимым.

После появления Stage 6 один и тот же изолированный chord/melody event может законно получить другой voicing, если отличается предыдущий Voice State. Это не нарушает детерминированность: одинаковая **полная последовательность** и одинаковое начальное состояние всё равно должны давать одинаковый MIDI.

Нельзя незаметно добавлять:

- random choice между равными кандидатами;
- time-based seed;
- скрытую вариативность между playback/render passes;
- непредсказуемый tie-break, зависящий от порядка контейнера или platform-specific iteration.

Если в будущем понадобится художественная вариативность, она должна быть отдельной явной feature (`Variation`, `Seed`, preset/profile option) с сохраняемым state. Одинаковый seed + одинаковый input должен оставаться воспроизводимым.

Это особенно важно для editable arranger workflow: понравившийся результат должен повторяться при следующем playback, render или записи, пока пользователь сам не изменил входной контекст или musical state.

---

## 10. Правило для следующих чатов / агентов

Перед изменением Harmony Core или Voicing Engine новый чат должен ответить себе на пять вопросов:

```text
1. Что именно музыкально неправильно?
2. Какие фактические Chord / Key / Melody / timeline данные получил engine?
3. Какой слой владеет этим решением?
4. Можно ли исправить только этот слой, не меняя соседние contracts?
5. Какой regression test не даст снова сломать это поведение?
```

Если на вопрос 3 нет однозначного ответа — не начинать широкий refactor. Сначала уточнить архитектурную границу.

Если fix требует менять два или больше соседних слоя, это признак, что изменение уже не является маленьким musical tuning и должно быть оформлено как отдельное архитектурное решение.

---

## 11. Основной цикл развития музыкального движка

Smart Voicing не предполагает, что Harmony Core однажды будет «идеально закончен» и больше не изменится.

Нормальный цикл проекта:

```text
реальная песня / аранжировка
→ странный или слабый результат
→ музыкальный анализ причины
→ минимальная корректировка правильного слоя
→ regression test
→ CI
→ host test
→ следующий реальный кейс
```

Цель — не запретить изменения, а сделать их локальными, объяснимыми и обратимо проверяемыми.

Стабильными должны оставаться архитектурные границы; музыкальные policies внутри своих слоёв могут постепенно шлифоваться по реальному репертуару.

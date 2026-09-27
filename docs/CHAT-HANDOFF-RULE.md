# Smart Voicing — правило переноса контекста между чатами

**Назначение:** единый workflow завершения рабочего чата и передачи актуального состояния следующему чату без потери решений, тестов и незавершённых задач.

## 1. Главный принцип

Каждый основной рабочий чат Smart Voicing должен завершаться актуальным **handoff-файлом** в Markdown.

Handoff — не архив переписки, а сжатый snapshot состояния проекта на момент перехода.

Новый чат начинает работу с:

```text
1. docs/START-HERE.md
2. последнего Smart-Voicing-Handoff-<version>.md
3. актуального GitHub: code / Issues / PR / CI / TEST docs
4. docs/CONCEPT.md и профильных документов
```

После чтения документов новый чат **обязан перепроверить GitHub**, а не принимать handoff как более свежий источник.

## 2. Источник истины

Приоритет:

```text
1. GitHub code / Issues / PR / CI / docs/TEST-*.md
2. docs/CONCEPT.md
3. профильные docs/*.md / Project Sources
4. последний handoff
5. история переписки
```

Если handoff расходится с GitHub — верить GitHub.

Особенно важно разделять:

```text
Implemented
CI confirmed
Studio Pro confirmed
```

Host-specific функцию нельзя помечать accepted только по successful build/tests.

## 3. Рекомендуемая гранулярность чатов

```text
один основной Stage / stable checkpoint = один основной рабочий чат
```

Пример:

```text
0.4 chat → Stage 4 → stable 0.4
0.5 chat → Stage 5 slices 0.4a... → stable 0.5
0.6 chat → Stage 6 → stable 0.6
```

Исследовательские параллельные чаты допустимы, но итоговые решения должны вернуться в GitHub Issues/docs, иначе они не считаются частью архитектуры проекта.

## 4. Перед завершением чата

Проверить:

- [ ] актуальный код находится в GitHub;
- [ ] профильный Issue обновлён;
- [ ] Issue checklist соответствует реальной реализации;
- [ ] текущий PR не содержит заведомо устаревшего статуса;
- [ ] текущая версия/package названы корректно;
- [ ] TEST-файл актуален;
- [ ] последний релевантный CI проверен;
- [ ] Studio Pro acceptance записан отдельно от CI;
- [ ] незакрытые баги не выданы за завершённые;
- [ ] архитектурные решения отражены в `docs/CONCEPT.md` или профильном canonical doc;
- [ ] долгосрочные идеи получили Issue/roadmap doc;
- [ ] `docs/START-HERE.md` не вводит следующий чат в заблуждение;
- [ ] создан/обновлён handoff текущего Stage.

## 5. Обязательная структура handoff

### 5.1 Current state

```text
Stable version:
Current development version:
Current Stage:
Working branch:
Main Issue:
Current PR:
Implementation baseline:
Latest verified CI:
Host artifact:
```

### 5.2 Confirmed

Раздельно:

```text
Implemented
CI confirmed
Studio Pro confirmed
```

### 5.3 Current work

Указать:

- текущий slice/version;
- точный musical/technical contract;
- что уже реализовано;
- что осталось;
- какой test/checklist ожидается;
- известные баги/ограничения.

### 5.4 Architecture decisions

Только решения, влияющие на будущую разработку. Например:

```text
Stage 4 = harmonic meaning
Stage 5 = vertical strategy
Stage 6 = continuity / Voice identity
Stage 7 = Instrument Profile
Stage 7B = performance behaviour
```

### 5.5 Musical rules

Для нового arranging/theory rule указывать:

- краткую формулировку;
- источник/обоснование;
- owning layer/Stage;
- Issue/test/doc, где rule закреплён.

### 5.6 Open questions

Не смешивать гипотезы с принятыми решениями.

### 5.7 NEXT ACTION

Handoff всегда заканчивается конкретным следующим действием, например:

```text
NEXT ACTION:
1. Проверить Build #...
2. Если green — запустить Studio Pro checklist ...
3. После host acceptance обновить TEST/Issue/PR.
```

## 6. Что не переносить

Не нужен полный пересказ чата, весь код, все failed builds, старые отвергнутые гипотезы, дублирование README/Issues и длинные дискуссии после того, как решение уже формализовано.

Цель:

> новый чат должен за 1–3 минуты понять состояние проекта и продолжить работу.

## 7. Правило синхронизации архитектуры

Решение, меняющее общую модель Smart Voicing, фиксируется минимум в двух местах:

```text
1. профильный GitHub Issue
2. docs/CONCEPT.md или отдельный canonical architecture roadmap, на который CONCEPT ссылается
```

Handoff только суммирует решение и даёт ссылки.

К решениям такого уровня относятся:

- изменение authority priority Melody / Chord / Key / Function;
- новая VoicingStrategy family;
- новая Tension/Function policy;
- изменение Stage boundary;
- новая Voice Leading модель;
- Instrument / Ensemble Profile architecture;
- новый Performance layer/mode;
- изменение стабильной keyswitch/state architecture.

## 8. Работа с литературой / источниками

При добавлении книги, статьи, видео или партитуры:

```text
source material
→ извлечь полезный музыкальный принцип
→ формализовать как Policy / Strategy / Constraint
→ Issue / regression / architecture doc
→ code only when implementation Stage arrives
```

Не переносить литературу в runtime logic напрямую и не копировать учебный материал вместо формализации.

Основной текущий arranging reference: Ted Pease / Ken Pullig — *Modern Jazz Voicings: Arranging for Small and Medium Ensembles*.

## 9. Guardrail музыкальных исправлений

Перед fix определить owning layer:

```text
Stage 4 — что означает гармония / какие pitch classes оправданы
Stage 5 — как material организован по вертикали
Stage 6 — как голоса переходят между verticals
Stage 7 — где это удобно/характерно инструментам
Stage 7B — как исполнители различаются по timing/expression
```

Ключевое правило:

> Сначала определить слой музыкальной проблемы и исправлять её только в этом слое.

После каждого musical fix:

```text
local regression
→ full CI
→ host test when applicable
→ TEST doc
→ Issue/PR/handoff sync
```

Не маскировать проблемы timing/context generic short-note filters.

## 10. Realtime / determinism

Сохранять общепроектные требования:

- no mutex in audio callback;
- no file I/O in audio callback;
- no dynamic allocation in normal realtime path;
- same complete input + same state → same output;
- future randomness/humanization only through explicit deterministic Seed.

## 11. Именование handoff

Основной формат:

```text
Smart-Voicing-Handoff-0.4.md
Smart-Voicing-Handoff-0.5.md
Smart-Voicing-Handoff-0.6.md
```

Если чат меняется внутри Stage до stable checkpoint, допускается более точное имя, но canonical handoff Stage должен оставаться понятным.

## 12. Как начинать следующий чат

Достаточно сказать:

```text
Продолжаем Smart Voicing.
Сначала прочитай docs/START-HERE.md и последний handoff,
затем проверь GitHub Issue/PR/CI и продолжай с NEXT ACTION.
```

Следующий чат должен сам:

1. проверить branch/Issue/PR/CI;
2. сопоставить их с handoff;
3. отметить расхождения;
4. продолжить с NEXT ACTION;
5. не просить пользователя пересказывать то, что уже есть в repo docs.

## 13. Ключевая формула

> Переписка — рабочая среда. GitHub — источник фактического состояния. `CONCEPT` — архитектурная модель. `START-HERE` — быстрый вход. Handoff — мост между чатами.

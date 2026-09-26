# Smart Voicing — START HERE

**Назначение:** это короткая точка входа для нового чата/разработчика. Перед продолжением разработки прочитать этот файл, затем проверить актуальное состояние GitHub.

## 1. Источник истины

Приоритет источников проекта:

```text
1. GitHub code / Issues / PR / CI / docs/TEST-*.md
2. docs/CONCEPT.md + docs/CONCEPT-STAGE5-ADDENDUM.md
3. профильные docs/*.md
4. последний handoff-файл
5. история чатов / Project Sources
```

Если документы расходятся с кодом, Issue, PR или CI — актуальным считать GitHub. Никогда не считать host-specific поведение подтверждённым только по green CI: отдельно различать **implemented**, **CI-confirmed** и **Studio Pro confirmed**.

`CONCEPT-STAGE5-ADDENDUM.md` временно supersedes конфликтующие preliminary sections основного `CONCEPT.md` (в первую очередь старую keyswitch map и roadmap до появления Stage 7B). При stable 0.5 addendum должен быть слит обратно в основной CONCEPT.

Подробное правило переноса контекста: `docs/CHAT-HANDOFF-RULE.md`.

## 2. Текущий checkpoint

```text
Stable:          0.4 — Stage 4 complete
Development:     0.4g — Quartal host candidate; Windows CI and Studio Pro pending
Current Stage:   Stage 5 — Jazz Voicing Engine → 0.5
Working branch:  stage-5-jazz-voicing-engine
Main Issue:      #9 — Этап 5 — Jazz Voicing Engine
Current PR:      #30 — draft/open, tracks whole Stage 5 line
Reference host:  Studio Pro
```

Текущий implementation baseline перед этой документационной синхронизацией:

```text
commit 69406d2881525652e8c70b5a8bbaf510ca9cbdd3
Windows Build #421 / run 35959470042 — SUCCESS
artifact: Smart-Voicing-0.4e-Windows
```

Важно: documentation-only commits после этого baseline не меняют музыкальную реализацию.

## 3. Что уже принято в Stage 5

### 0.4a — Foundation ✅ Studio Pro accepted

- общий `VoicingStrategy` / `VoicingContext` / `buildVoicing()` contract;
- persistent `Voicing Type`, `Closed` default;
- Stage 4 harmonic context передаётся strategy без повторного harmonic analysis;
- `Clean / Color / Rich` как единый UI/project/keyswitch state;
- realtime keyswitches Tension `43/44/45`;
- collapsible Diagnostics;
- transition hygiene;
- corrected `V7 → minor` tension semantics;
- deterministic musical-output contract.

Подробности: `docs/TEST-0.4a.md`.

### 0.4b — Drop 2 ✅ Studio Pro accepted

- Drop 2 = transformation уже выбранного Closed material;
- V1 melody immutable;
- те же pitch classes / tensions, меняется только register/shape;
- slash-bass safe Closed fallback;
- regressions + host acceptance.

Подробности: `docs/TEST-0.4b.md`.

### 0.4c — Melodic textures ✅ Studio Pro accepted

- `0.4c1` Unison;
- `0.4c2` Octaves;
- `0.4c3` deterministic Doubling;
- `0.4c4` Performance Keyswitch Layer + Harmony Mode keyswitches.

Melodic textures организуют performer-owned melody, а не harmonic candidate pool. Поэтому `Clean / Color / Rich` может не менять pitch output в pure Unison/Octaves/Doubling.

Подробности: `docs/TEST-0.4c.md`, `docs/TEST-0.4c4.md`.

### 0.4d — Drop 3 ✅ Studio Pro accepted

- Drop 3 = Closed V3 down one octave;
- V1 и Closed pitch-class vocabulary сохраняются;
- slash bass использует safe Closed fallback;
- `D1 / MIDI 38` активен как Drop 3.

Подробности: `docs/TEST-0.4d.md`.

## 4. Принято — 0.4e Drop 2+4

**Implemented + CI-confirmed + Studio Pro accepted (2026-09-26).**

Контракт:

```text
Closed [V1, V2, V3, V4]
V2 → V2 - 12
V4 → V4 - 12
V1 unchanged
V2..V4 re-sort by sounding pitch
```

Готово:

- `VoicingType::drop24` добавлен без перенумерации старых enum values;
- pure transform + dispatcher;
- dedicated regressions;
- processor/state/UI integration;
- `D#1 / MIDI 39` активирован;
- Windows Build #421 green;
- artifact `Smart-Voicing-0.4e-Windows` существует.

Пользователь подтвердил успешный тест Drop 2+4 в Studio Pro. Следующая работа: 0.4f Spread как самостоятельная bottom-up strategy.

### 0.4f — Spread host candidate

- Independent bottom-up strategy; V1 melody unchanged, V4 root or explicit slash-bass anchor, V3/V2 from Stage 4 candidate pool.
- Guide 3/7 and characteristic chord tones take priority over optional colour; spacing is open and deterministic.
- Core Windows Build #435 green; processor/state/UI and `B0 / MIDI 35` activation added after that build.
- Full Windows Build #436 green with `Smart-Voicing-0.4f-Windows`. User confirmed the Spread mode and remaining technical host checks on 2026-09-26; weak differentiation of Clean/Color/Rich is deferred. Extreme-low MIDI host demonstration was waived because the trumpet range ends first; the underlying MIDI safety is covered by core tests. See `docs/TEST-0.4f.md` and `docs/0.4f-HOST-CHECKLIST.md`.

## 5. Финальная performance keyswitch map

Canonical contract — **MIDI note numbers**. Названия нот соответствуют octave naming Studio Pro, использованному в тестах.

```text
32 / G#0  UST             RESERVED
33 / A0   Cluster         RESERVED
34 / A#0  Quartal         ACTIVE in 0.4g host candidate
35 / B0   Spread          ACTIVE in 0.4f host candidate

36 / C1   Closed
37 / C#1  Drop 2
38 / D1   Drop 3
39 / D#1  Drop 2+4
40 / E1   Unison
41 / F1   Octaves
42 / F#1  Doubling

43 / G1   Clean
44 / G#1  Color
45 / A1   Rich

46 / A#1  Direct Router
47 / B1   Melody Harmonize
```

Sound Variations — presentation layer над тем же plugin state, а не отдельная state system. Подробности и рекомендуемый порядок: `docs/STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md`.

## 6. Главные архитектурные границы

```text
Stage 4 — harmonic meaning
Chord / Key / Function / real Resolution Target / Tension vocabulary
        ↓
Stage 5 — vertical organization
Voicing Strategy / shape
        ↓
Stage 6 — continuity
Voice Leading / stable Voice identity
        ↓
Stage 7 — playable/desirable register
Instrument / Ensemble Profiles
        ↓
Stage 7B — performance behaviour
Humanization / Expression Distribution
        ↓
Stage 8 — saved Performance Configurations
```

Ключевой guardrail Stage 5:

> VoicingStrategy организует уже музыкально корректный Stage 4 material. Strategy не должна независимо переопределять Function, Resolution Target или Tension Policy.

Drop-family invariant:

```text
Closed selected pitch classes
→ Drop transform changes only octave/shape
→ harmonic vocabulary is not recalculated
```

Spread — отдельная bottom-up strategy, не stretched Closed. Quartal, Cluster и UST — отдельные strategy objectives.

## 7. Stage 4 harmonic contract, который нельзя ломать

Приоритет:

```text
Played / Melody
> Explicit Current Chord
> chord identity / characteristic tones
> Current Key
> Harmonic Function + REAL next-chord target
> Functional Tension Profile / Tension Level
> Melodic Role / Tension Policy
> Voicing Strategy
> Voice Leading
> Instrument Profile
```

Критические правила:

- no next chord → no assumed resolution target;
- real next chord root+quality → target-aware dominant profile;
- explicit Chord Track material authoritative;
- ordinary perfect fifth often expendable, characteristic b5/#5/sus/explicit altered fifth protected;
- melodic tension ≠ harmonic tension;
- avoid-as-harmony не запрещает melody;
- minor ninth = strong soft negative, not universal hard ban;
- confirmed `V7 → minor`: b13 naturally target-directed; b9 contextual/strong; #9/#11 не выводятся автоматически только из факта minor target.

Подробнее: `docs/FUNCTIONAL-TENSION-PROFILES.md`, `docs/TENSION-LEVELS.md`, `docs/MUSICAL-ENGINE-GUARDRAILS.md`.

## 8. Realtime / determinism guardrails

- no mutex / file I/O / dynamic allocation in realtime audio callback;
- same complete input + same state → same musical output;
- future humanization also deterministic: same MIDI + automation + settings + Seed → same output;
- V1 melody authoritative в melody-led strategies;
- не маскировать timing/context bugs generic short-note filters;
- каждый музыкальный fix должен получить regression;
- после локального fix: regressions → full CI → Studio Pro host test → docs/Issue/handoff.

## 9. Дальнейший roadmap

### Stage 5 → 0.5

После принятия Drop 2+4 остаются:

- Spread bottom-up strategy;
- Quartal strategy contract/implementation;
- Cluster strategy contract/implementation;
- UST strategy contract/implementation;
- strategy-specific vertical objective + continuity hints;
- cross-strategy regressions preserving Stage 4 colour semantics.

### Stage 6 → 0.6 — Voice Leading

Previous Voice state, common-tone retention, stepwise movement, stable Voice identity, crossing/leap penalties, strategy objectives, register-aware scoring hooks.

### Stage 7 → 0.7 — Instrument / Ensemble Profiles

Brass Profile first; absolute/practical/comfortable ranges; register zones; balance/timbre scoring; octave displacement. Harmony Core не hardcode-ит `V1=Trumpet`.

### Stage 7B — Performance Engine

Tracking Issue #31, details: `docs/PERFORMANCE-ENGINE-ROADMAP.md`.

Основной pipeline Expression:

```text
Base Expression automation
→ per-Voice Expression Gain / Trim
→ gain-adjusted per-Voice baseline
→ Expression Humanization / response / smoothing
→ Instrument Profile mapping
→ Ch1..Ch4
```

Четыре Expression Gain — баланс, Humanize — вариативность. Humanize всегда работает **вокруг уже gain-adjusted baseline конкретного Voice**, поэтому, например, Bari с 80% Gain не возвращается к общей кривой остальных инструментов. Timing MVP — delay-only; randomness только deterministic/saved Seed.

### Stage 8 → 0.8 — Presets / Performance Configurations

Сохранять unified Mode / Distribution / Voicing state, Instrument Profile и будущие Performance Engine settings (Humanize, four Expression Gains, Seed и т. д.).

Дальше: Stage 9 Host Compatibility, Stage 10 Recording/Capture, Stage 11 Stability/Panic/Final UI → 1.0.

## 10. Musical knowledge base

Главный reference проекта сейчас:

- Ted Pease / Ken Pullig — *Modern Jazz Voicings: Arranging for Small and Medium Ensembles*.

Из источника в проект уже перенесены формализованные принципы: melodic vs harmonic tensions, candidate-pool/chord-scale thinking, avoid-note semantics, minor-ninth exceptions, Closed/Drop family, Spread, Quartal, Cluster, UST и register/timbre-aware instrument thinking.

Литература и Project Sources не являются runtime source of truth. Полезные выводы должны быть формализованы в Policies / Strategies / Constraints и перенесены в GitHub Issue/tests/docs/code.

## 11. Что читать дальше

Для быстрого старта достаточно следующего порядка:

```text
1. docs/START-HERE.md              ← этот файл
2. docs/Smart-Voicing-Handoff-0.5.md
3. Issue #9 + PR #30 + latest CI
4. docs/TEST-0.4e.md / host checklist
5. docs/CONCEPT.md + docs/CONCEPT-STAGE5-ADDENDUM.md
6. docs/MUSICAL-ENGINE-GUARDRAILS.md
```

Профильно:

- keyswitch workflow → `docs/STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md`;
- future humanization/expression → `docs/PERFORMANCE-ENGINE-ROADMAP.md`;
- Stage 6 direction → `docs/VOICE-LEADING-DIRECTION.md`.

## 12. NEXT ACTION

```text
1. Проверить полный Windows CI коммита 0.4g и наличие пакета `Smart-Voicing-0.4g-Windows`.
2. После green CI выполнить `docs/0.4g-HOST-CHECKLIST.md` в Studio Pro; до этого 0.4g остаётся candidate.
3. Сохранить отдельную задачу на различие Clean/Color/Rich в Spread, начиная с G7 → Cm7 и мелодии на b7; не переопределять Stage 4 vocabulary.
```

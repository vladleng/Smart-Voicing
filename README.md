# Smart Voicing

**Smart Voicing** — MIDI-плагин Moon River Studio для распределения независимых голосов и гармонизации с использованием гармонического контекста DAW.

Главная схема:

```text
Chord Track + Key + transport
        +
входящий MIDI
        +
voicing / voice-leading rules
        ↓
Smart Voicing
        ↓
Voice 1 / Voice 2 / Voice 3 / Voice 4
        ↓
отдельные instrument tracks
```

DAW остаётся **источником гармонической истины**. Smart Voicing не требует отдельной копии Chord Track / Key Track внутри плагина.

## Текущий статус

**Smart Voicing 0.3 — завершённый Этап 3: Chord-aware Harmonizer + Harmonic Context.**

Завершено:

- Этап 0 — каркас и базовая VST3-сборка;
- Этап 1 — ARA Context Proof of Concept → `0.1`;
- Этап 2 — рабочий четырёхголосный MIDI Router → `0.2`;
- Этап 3 — Chord-aware Harmonizer + Harmonic Context → `0.3`.

Следующий этап: **Этап 4 — Key-aware Engine и гармонические функции**, рабочие версии `0.3a ...`, финал этапа `0.4`.

## Что подтверждено в Studio Pro

- `Smart Voicing ARA` как ARA/Event FX получает Key Signatures, Sheet Chords, Tempo Entries и Bar Signatures;
- изменения Chord / Key / Time Signature приходят live;
- Audio Event является ARA-якорем, но не ограничивает диапазон Musical Context;
- основной Instrument получает harmonic context и transport через shared-memory bridge;
- MIDI output Smart Voicing используется как источник downstream Instrument Tracks;
- Voice 1–4 разведены через MIDI Channels/Input 1–4 на четыре отдельных SWAM-инструмента;
- CC / automation / Pitch Bend проходят downstream;
- routed MIDI записывается на отдельные downstream-дорожки;
- Voice Stack, legato/portamento gesture и Sustain Chord Morph работают в реальной игре;
- три Distribution Modes работают корректно;
- Gesture Classifier разделяет Chord Gesture и Voice Gesture;
- 5+ нот аккордового жеста не создают пятый независимый Voice;
- ARA Context и MIDI Router работают параллельно;
- Normalized Chord Model корректно читает quality / extensions / alterations / slash bass;
- `Melody Harmonize` строит базовый Close voicing по текущему Chord Track;
- сыгранная melody note остаётся V1 даже когда она не является chord tone;
- удержанная melody автоматически reharmonize-ится на сменах Chord Track без нового Note On;
- chord boundaries планируются внутри текущего audio block без добавления plugin latency;
- Sustain-owned melody продолжает реагировать на Chord Track после отпускания физической клавиши;
- pedal-up корректно освобождает sustain-owned voicing;
- `(no chord)` оставляет V1 и выключает V2–V4 до следующего валидного chord;
- Harmony Mode / Distribution Mode сохраняются в состоянии проекта;
- полный regression Direct Router / Sustain / State / Stop / CC120/123 / stuck notes пройден перед 0.3.

## Пакет 0.3

```text
Smart Voicing 0.3/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной Instrument / MIDI engine:

- принимает и выдаёт MIDI;
- получает harmonic-context snapshot от ARA-компонента;
- содержит Context Monitor, MIDI Router и Harmony Core;
- поддерживает `Direct Router` и `Melody Harmonize`;
- выдаёт четыре независимых Voice через MIDI Channels 1–4.

### Smart Voicing ARA.vst3

Служебный ARA/Event FX reader:

- устанавливается на Audio Event;
- пропускает аудио без изменений;
- читает Musical Context через ARA;
- публикует harmonic context и transport для основного Instrument.

## Этап 1 — ARA Context

Финальная версия: **0.1**.

Архитектура:

```text
Studio Pro Chord / Key / Tempo / Signature
        ↓ ARA
Smart Voicing ARA
        ↓ shared context + transport
Smart Voicing Instrument
```

Bridge ABI v3 разделяет harmonic snapshot и transport snapshot. Transport обновляется change-driven и не использует mutex / file I/O в audio callback.

Boundary semantics для Chord / Key / Time Signature:

```text
до границы    → предыдущий контекст
на границе    → новый контекст
после границы → новый контекст
```

Используется tolerance `0.0001 PPQ`.

## Этап 2 — MIDI Router

Финальная версия: **0.2**.

### 0.1a — MIDI Router Probe

- pass-through и MIDI diagnostics;
- подтверждение downstream routing.

### 0.1b — Direct 4 Voice Router

- Voice 1 → Ch1;
- Voice 2 → Ch2;
- Voice 3 → Ch3;
- Voice 4 → Ch4.

### 0.1c — Stable Voice Ownership + Sustain

- Voice сохраняет channel identity;
- движение одного голоса не пересортировывает соседние Voice;
- базовая sustain-aware модель.

### 0.1d — Voice Stack / Legato + Sustain Chord Morph

- у каждого Voice собственный фиксированный note stack;
- overlap `Note On` на том же Voice/channel даёт mono-инструментам legato/portamento cue;
- новый аккорд может быть сыгран при pedal-down;
- pedal-up удаляет старые отпущенные ноты, сохраняя физически зажатый новый аккорд.

### 0.1e — Distribution Modes + Gesture Classifier

Режимы:

```text
1. Сверху вниз
2. Снизу вверх
3. Заполнить 4 голоса
```

`Заполнить 4 голоса`:

```text
1 нота → V1=V2=V3=V4
2 ноты → V1=V2=верхняя, V3=V4=нижняя
3 ноты → V1=верхняя, V2=средняя, V3=V4=нижняя
4 ноты → по одной ноте на V1–V4
```

Ownership использует `note → Voice mask`, поэтому одна входная нота может управлять несколькими Voice.

Gesture Classifier различает:

- **Chord Gesture** — короткая группа нот, максимум четыре независимых Voice;
- **Voice Gesture** — отдельная поздняя нота, continuation ближайшего Voice с same-channel legato;
- **Sustain Chord Morph** — новый chord frame может войти при удержанной педали.

Порог classifier около 45 ms считается tuning-параметром и будет меняться только по реальным сценариям игры.

### 0.2 — завершение Этапа 2

0.2 фиксирует текущий рабочий Router как базовую платформу для следующих этапов. Новых музыкальных функций относительно подтверждённой 0.1e не добавляется.

Отдельная кнопка `Panic / All Notes Off` и финальная эксплуатационная полировка сознательно перенесены ближе к релизному этапу. Текущий MIDI path уже обрабатывает стандартные CC120/123.

Контрольный документ: [`docs/TEST-0.2.md`](docs/TEST-0.2.md).

## Этап 3 — Chord-aware Harmonizer + Harmonic Context

Финальная версия: **0.3**.

Архитектура:

```text
ARA / другой provider
        ↓
IHarmonicContextProvider
        ↓
HarmonicContext / ChordContext
        ↓
Chord-aware Harmonizer
        ↓
VoiceOutput[4]
        ↓
Router / Voice Stack / Sustain
```

### 0.2a — Harmonic Context Core

- host-neutral `HarmonicContext` / `ChordContext`;
- `IHarmonicContextProvider`;
- `ARAContextProvider`;
- `VoiceOutput[4]`.

### 0.2b — Chord Model

- root / bass из ARA circle-of-fifths;
- chord quality;
- extensions / alterations;
- slash bass;
- host-neutral automated tests.

### 0.2c — Melody Harmonize

- сыгранная melody note = V1;
- V2–V4 строятся из текущего chord;
- первый базовый Close voicing;
- при отсутствии chord остаётся только V1.

### 0.2d — Live Chord Reharmonization

- V2–V4 перестраиваются на сменах Chord Track без нового Note On;
- общий тон в том же Voice slot не retrigger-ится;
- sample-accurate scheduling chord boundary внутри audio block;
- lookahead и reported plugin latency не используются.

### 0.2e — Sustain / Voice Stack / State integration

- Melody Harmonize использует общий Voice Stack / Router path;
- Sustain-owned melody продолжает reharmonize-иться после release физической клавиши;
- pedal-up корректно освобождает sustain-owned voicing;
- State сохраняет Harmony Mode / Distribution Mode;
- выполнена полная регрессия Direct Router 0.2 и harmonic path.

### 0.3 — завершение Этапа 3

0.3 — стабильный checkpoint поверх подтверждённой 0.2e без новых музыкальных изменений.

Контрольные документы: [`docs/TEST-0.2e.md`](docs/TEST-0.2e.md), [`docs/TEST-0.3.md`](docs/TEST-0.3.md).

## Архитектурные принципы

- musical core не должен зависеть от конкретной DAW;
- host capabilities определяются по возможностям, а не имени приложения;
- ARA context кэшируется вне real-time audio thread;
- MIDI processing остаётся real-time safe;
- Router state использует фиксированные структуры без mutex/file I/O;
- итог Smart Voicing — редактируемый MIDI-скелет;
- current chord имеет приоритет над общим Key context; полноценная Key-aware логика начинается на Этапе 4;
- voicing type и Voice Leading развиваются как отдельные слои.

## Актуальный Roadmap

- **Этап 0** — каркас проекта и базовая сборка. ✅
- **Этап 1** — ARA Context Proof of Concept. ✅ → `0.1`
- **Этап 2** — MIDI Router. ✅ → `0.2`
- **Этап 3** — Chord-aware Harmonizer + Harmonic Context. ✅ → `0.3`
- **Этап 4** — Key-aware Engine и гармонические функции. 🚧 следующий → `0.3a ... 0.4`
- **Этап 5** — Jazz Voicing Engine. → `0.4a ... 0.5`
- **Этап 6** — Voice Leading. → `0.5a ... 0.6`
- **Этап 7** — Instrument Profiles и диапазоны. → `0.6a ... 0.7`
- **Этап 8** — Presets и Performance Configurations. → `0.7a ... 0.8`
- **Этап 9** — Host Compatibility и fallback-провайдеры. → `0.8a ... 0.9`
- **Этап 10** — Recording / Capture результатов в DAW. → `0.9a ... 0.10`
- **Этап 11** — Оптимизация, стабильность, Panic и финальный UI. → `0.10a ... 1.0`

Post-1.0 идеи Performance Profiles / Ensemble Modes отслеживаются отдельно в Issue #20 и не расширяют текущий core roadmap.

## Версионирование

```text
0.1 → завершённый Этап 1
0.1a ... 0.1e → разработка Этапа 2
0.2 → завершённый Этап 2
0.2a ... 0.2e → разработка Этапа 3
0.3 → завершённый Этап 3
0.3a ... → разработка Этапа 4
```

Внутренний CMake version для 0.3: `0.3.0`.

Подробно: [`docs/VERSIONING.md`](docs/VERSIONING.md).

## Язык проекта

README, концепция, Issues, roadmap и пояснения ведутся на русском языке. Английский используется для API, SDK, идентификаторов, кода и общепринятых технических терминов.

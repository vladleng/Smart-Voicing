# Smart Voicing

**Smart Voicing** — экспериментальный MIDI-плагин для гармонизации и распределения голосов, разрабатываемый **Moon River Studio**.

Цель проекта — использовать гармонический контекст самой DAW (`Chord Track`, `Key`, позицию на таймлайне) вместе с входящим MIDI и на его основе формировать отдельные музыкальные голоса для духовых, струнных и других ансамблевых инструментов.

## Основная идея

```text
Chord Track + Key в DAW
        +
входящий MIDI
        +
правила / preset voicing
        ↓
Smart Voicing
        ↓
независимые MIDI-голоса
        ↓
отдельные инструментальные дорожки
```

Главный принцип: **DAW остаётся источником гармонической истины**. Если хост уже содержит Chord Track и Key Track, пользователь не должен создавать вторую независимую гармоническую карту внутри Smart Voicing.

Музыкальное ядро при этом должно оставаться независимым от конкретной DAW. ARA 2 используется как один из способов получения гармонического контекста, а не как часть самого алгоритма гармонизации.

## Текущий статус

Проект находится на стадии **pre-alpha**.

Последняя завершённая версия: **Smart Voicing 0.1**.  
Текущая рабочая версия: **Smart Voicing 0.1a**.  
**Этап 1 — ARA Context Proof of Concept завершён.**  
**Текущий этап: Этап 2 — MIDI Router.**

Версия 0.1 фиксирует полностью проверенный ARA-контекст и двухкомпонентную архитектуру после последовательных рабочих сборок 0.0b–0.0g.

Подтверждено в Fender Studio / Studio Pro:

- `Smart Voicing ARA` как ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- Instrument role не получает `Musical Context` напрямую;
- изменения Chord Track, Key Track и Bar / Time Signature приходят live без Reload;
- длина Audio Event с ARA-компонентом не ограничивает считываемый диапазон — Event используется как **ARA-якорь**;
- реальные карты Chord / Key / Tempo / Time Signature считываются с позициями на таймлайне;
- основной Instrument получает эти карты через shared-memory bridge;
- Context Monitor показывает текущий аккорд, тональность, размер, темп и позицию;
- transport publication работает change-driven: в STOP revision остаётся стабильным;
- сохранение и повторное открытие проекта проверено;
- границы Chord / Key / Time Signature обрабатываются как start-inclusive с малым floating-point tolerance, поэтому визуальная граница в Studio Pro соответствует новому событию.

## Архитектура 0.1

Пакет состоит из двух лёгких VST3-компонентов:

```text
Smart Voicing 0.1/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной компонент:

- отображается как Instrument;
- принимает и выдаёт MIDI;
- получает harmonic-context snapshot от `Smart Voicing ARA`;
- содержит Context Monitor;
- на Этапе 2 получает MIDI Router;
- далее получит harmonizer, voicing и voice leading.

### Smart Voicing ARA.vst3

Служебный компонент:

- загружается как ARA/Event FX на любой Audio Event;
- пропускает аудио без изменений;
- читает `Musical Context` проекта через ARA;
- публикует harmonic-context snapshot;
- публикует transport position для основного Instrument.

Подтверждённая схема:

```text
Studio Pro Chord / Key / Tempo / Signature
        ↓ ARA
Smart Voicing ARA
        ↓ shared context + transport
Smart Voicing Instrument
        ↓ MIDI Router / дальнейшая обработка
Destination instruments
```

## Shared bridge

Текущий Windows-прототип использует named shared memory без файлового I/O.

Bridge ABI v3 разделяет:

- harmonic map + harmonic revision;
- transport position + transport revision;
- отдельные seqlock-счётчики для безопасного lock-free чтения.

Транспорт публикуется только при фактическом изменении PPQ / seconds / PLAY-STOP и остаётся real-time safe: без mutex, allocation и файлового I/O в audio callback.

## Boundary semantics

Для Chord / Key / Time Signature действует единое правило:

```text
до границы    → предыдущий контекст
на границе    → новый контекст
после границы → новый контекст
```

Для компенсации микроскопического floating-point расхождения между host transport и ARA event position используется небольшой `boundary tolerance = 0.0001 PPQ`.

## Принципы архитектуры

- музыкальная логика не зависит от JUCE, ARA и конкретной DAW;
- хостовые возможности определяются через capability detection, а не по имени DAW;
- ARA-контекст кэшируется вне real-time audio thread;
- MIDI-обработка должна быть real-time safe;
- UI остаётся минимальным;
- CPU и память используются максимально экономно;
- никаких тяжёлых фоновых процессов без необходимости;
- при отсутствии ARA в будущем должны быть возможны альтернативные источники контекста.

Планируемая абстракция:

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

## Этап 2: MIDI Router

Этап 2 превращает основной `Smart Voicing` из Context Monitor в первый реально работающий MIDI-маршрутизатор.

Стартовая рабочая версия этапа: **0.1a**.  
Целевая завершённая версия этапа: **0.2**.

Базовая цель:

```text
MIDI input
   ↓
Smart Voicing
   ↓
Voice 1 / Voice 2 / Voice 3 / Voice 4
   ↓
отдельные целевые инструменты / дорожки
```

На этом этапе гармонизация ещё не нужна. Сначала требуется надёжно определить модель распределения и маршрутизации независимых MIDI-голосов в Studio Pro и сделать её пригодной для дальнейшего harmonizer engine.

Текущие задачи ведутся в [Issue #17 — Этап 2: MIDI Router](https://github.com/vladleng/Smart-Voicing/issues/17).

## Roadmap

- **Этап 0** — каркас проекта и базовая сборка. ✅
- **Этап 1** — ARA Context Proof of Concept. ✅ → `0.1`
- **Этап 2** — MIDI Router. 🚧 → `0.1a ... 0.2`
- **Этап 3** — Chord-aware harmonizer.
- **Этап 4** — Key-aware engine.
- **Этап 5** — Jazz voicing.
- **Этап 6** — Voice leading.
- **Этап 7** — Instrument profiles и presets.

## Философия проекта

Smart Voicing не должен автоматически создавать «готовую аранжировку».

Его задача — быстро получить музыкально осмысленный и редактируемый MIDI-скелет.

Полная концепция и технические заметки находятся в [`docs/CONCEPT.md`](docs/CONCEPT.md).

## Язык ведения проекта

README, концепция, этапы разработки, GitHub Issues, задачи, подзадачи, roadmap и пояснения ведутся **на русском языке**.

Английский используется там, где он необходим технически: имена классов и методов, API, SDK, форматы, идентификаторы, код и общепринятые технические термины.

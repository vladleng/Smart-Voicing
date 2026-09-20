# Smart Voicing

**Smart Voicing** — экспериментальный MIDI-плагин для гармонизации и распределения голосов, разрабатываемый **Moon River Studio**.

Цель проекта — использовать гармонический контекст самой DAW (`Chord Track`, `Key`, позицию на таймлайне) вместе с входящим MIDI и на его основе формировать отдельные музыкальные голоса для духовых, струнных и других ансамблевых инструментов.

## Основная идея

Целевой рабочий процесс:

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

Музыкальное ядро при этом должно оставаться независимым от конкретной DAW. ARA 2 рассматривается как один из способов получения гармонического контекста, а не как часть самого алгоритма гармонизации.

## Текущий статус

Проект находится на стадии **pre-alpha / архитектурного прототипа**.

Текущая рабочая версия: **Smart Voicing 0.0g**.  
Текущий этап: **Этап 1 — ARA Context Proof of Concept**.

Уже подтверждено в Fender Studio / Studio Pro:

- ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- Instrument role не получает `Musical Context` напрямую;
- изменения Chord Track, Key Track и Bar / Time Signature приходят без Reload;
- длина Audio Event с ARA-компонентом не ограничивает считываемый диапазон — Event используется как **ARA-якорь**;
- реальные карты Chord / Key / Tempo / Time Signature считываются с позициями на таймлайне;
- основной Instrument получает эти карты через shared-memory bridge;
- Context Monitor показывает текущий аккорд, тональность, размер, темп и позицию;
- `Transport revision` в STOP остаётся стабильным и меняется только при фактическом изменении транспорта;
- сохранение и повторное открытие проекта успешно проверено.

## Архитектура 0.0g

Из-за поведения Studio Pro прототип разделён на два лёгких VST3-компонента:

```text
Smart Voicing 0.0g/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной компонент:

- отображается как Instrument;
- принимает и выдаёт MIDI;
- получает harmonic-context snapshot от `Smart Voicing ARA`;
- содержит Context Monitor;
- в следующих этапах получит MIDI Router, harmonizer, voicing и voice leading.

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
        ↓ MIDI processing
Destination instruments
```

## Shared bridge

Текущий Windows PoC использует named shared-memory bridge без файлового I/O.

Bridge ABI v3 разделяет:

- harmonic map + harmonic revision;
- transport position + transport revision;
- отдельные seqlock-счётчики для безопасного lock-free чтения.

Начиная с 0.0f transport publication работает change-driven:

- одинаковый transport snapshot повторно не публикуется;
- `Transport revision` не растёт из-за повторных вызовов `processBlock()` в STOP;
- revision меняется только при фактическом изменении PPQ / seconds / PLAY-STOP;
- публикация остаётся real-time safe: без mutex, allocation и файлового I/O в audio callback.

## Что исправляет 0.0g

Во время полного теста 0.0f обнаружен небольшой boundary-баг: Studio Pro иногда визуально ставит курсор точно на начало нового аккорда, но transport PPQ и ARA event position могут отличаться на микроскопическую величину floating-point. В результате при почти одинаковых значениях, например около `PPQ 12.0`, мог выбираться предыдущий аккорд.

В 0.0g:

- для start-inclusive Chord / Key / Time Signature введён единый `boundary tolerance = 0.0001 PPQ`;
- Context Monitor показывает PPQ с повышенной точностью;
- добавлена строка `Boundary diag` с точной позицией курсора, ближайшим chord event и delta;
- позиции harmonic maps в debug UI выводятся с большей точностью.

Ожидаемое правило:

```text
до границы    → предыдущий контекст
на границе    → новый контекст
после границы → новый контекст
```

Контрольный тест: [`docs/TEST-0.0g.md`](docs/TEST-0.0g.md).

После успешного контрольного теста границ можно готовить **0.1**, закрывать Этап 1 и переходить к MIDI Router.

## Принципы архитектуры

- музыкальная логика не зависит от JUCE, ARA и конкретной DAW;
- хостовые возможности определяются через capability detection, а не по имени DAW;
- ARA-контекст кэшируется вне real-time audio thread;
- MIDI-обработка должна быть real-time safe;
- transport-публикация из audio thread не использует mutex, allocation или файловый I/O;
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

## Планируемые музыкальные режимы

После завершения ARA Proof of Concept:

- Direct 4 Voice;
- Melody Harmonize;
- Chord redistribution / revoicing;
- Context-aware voicing.

Первый практический набор voicing:

- Close;
- Drop 2;
- Guide Tones;
- Custom.

Позже:

- Drop 3;
- Drop 2+4;
- Spread;
- Unison;
- instrument-aware ranges;
- voice leading;
- root omission;
- tension policies.

Первый основной сценарий — квартет:

```text
Voice 1 → Trumpet
Voice 2 → Tenor Sax
Voice 3 → Trombone
Voice 4 → Baritone Sax
```

Но этот состав не должен быть жёстко зашит в движок.

## Roadmap

- **Этап 0** — каркас проекта и базовая сборка.
- **Этап 1** — ARA Context Proof of Concept.
- **Этап 2** — MIDI Router.
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

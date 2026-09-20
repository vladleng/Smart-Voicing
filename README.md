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

При этом музыкальное ядро должно оставаться независимым от конкретной DAW. ARA 2 рассматривается как один из способов получения гармонического контекста, а не как часть самого алгоритма гармонизации.

## Текущий статус

Проект находится на стадии **pre-alpha / архитектурного прототипа**.

Текущая рабочая версия: **Smart Voicing 0.0d**.  
Текущий этап: **Этап 1 — ARA Context Proof of Concept**.

На этом этапе уже подтверждено в Fender Studio / Studio Pro:

- ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- инструментальный экземпляр может получить ARA binding, но Studio Pro не прикрепляет к нему `Musical Context`;
- изменения Chord Track приходят через ARA без перезапуска плагина;
- изменение или добавление аккорда увеличивает `Bridge revision` в основном инструментальном экземпляре;
- длина аудио Event, на котором установлен ARA-компонент, не ограничивает доступ к гармоническому контексту: playhead может находиться за пределами Event, а изменения Chord Track продолжают поступать;
- следовательно, Audio Event используется как **ARA-якорь**, а не как временное окно действия гармонии.

## Архитектура 0.0d

Из-за поведения Studio Pro текущий прототип разделён на два очень лёгких VST3-компонента в одном пакете:

```text
Smart Voicing 0.0d/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной компонент:

- отображается как Instrument;
- принимает MIDI;
- выдаёт MIDI;
- в дальнейшем будет содержать MIDI Router, harmonizer, voicing и voice leading;
- не запрашивает ARA-контекст напрямую;
- получает готовый harmonic-context snapshot от `Smart Voicing ARA`.

### Smart Voicing ARA.vst3

Служебный компонент:

- загружается как ARA/Event FX на любой Audio Event;
- пропускает аудио без изменений;
- читает `Musical Context` проекта через ARA;
- отслеживает изменения Chord Track;
- публикует harmonic-context snapshot для основного `Smart Voicing`.

В 0.0d snapshot расширен: через bridge передаются уже не только counts, но и реальные карты событий `Chord / Key / Tempo / Time Signature` с позициями на музыкальном таймлайне.

Текущий Windows PoC использует минимальный named shared-memory bridge без файлового I/O. Это транспорт между двумя VST3-модулями, а не часть музыкальной логики.

Подтверждённая схема:

```text
Studio Pro Chord / Key Track
        ↓ ARA
Smart Voicing ARA
        ↓ shared context
Smart Voicing Instrument
        ↓ MIDI processing
Destination instruments
```

## Что проверяет 0.0d

Следующий тест Этапа 1:

- совпадают ли реальные названия аккордов и их PPQ-позиции с Chord Track;
- корректно ли передаются смены тональности как отдельные `Key Signature` events;
- корректно ли передаются смены размера как отдельные `Bar Signature` events;
- выбирает ли основной Instrument активные `Chord / Key / Time Signature` по текущей PPQ-позиции;
- меняется ли `Bridge revision` при редактировании Key Track и Time Signature;
- корректно ли работает контекст точно на границе смены события;
- корректно ли обрабатываются несколько смен тональности и размера в пределах одной аранжировки.

После подтверждения этих пунктов останется довести Stage 1 до версии **0.1**.

## Принципы архитектуры

- музыкальная логика не зависит от JUCE, ARA и конкретной DAW;
- хостовые возможности определяются через capability detection, а не по имени DAW;
- ARA-контекст кэшируется вне real-time audio thread;
- MIDI-обработка должна быть real-time safe;
- UI остаётся минимальным;
- CPU и память должны использоваться максимально экономно;
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

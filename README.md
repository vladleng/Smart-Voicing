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

Текущая рабочая версия: **Smart Voicing 0.0e**.  
Текущий этап: **Этап 1 — ARA Context Proof of Concept**.

На этом этапе уже подтверждено в Fender Studio / Studio Pro:

- ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- инструментальный экземпляр может получить ARA binding, но Studio Pro не прикрепляет к нему `Musical Context`;
- изменения Chord Track приходят через ARA без перезапуска плагина;
- изменение или добавление аккорда увеличивает harmonic `Bridge revision`;
- длина аудио Event, на котором установлен ARA-компонент, не ограничивает доступ к гармоническому контексту: playhead может находиться за пределами Event, а изменения Chord Track продолжают поступать;
- Audio Event используется как **ARA-якорь**, а не как временное окно действия гармонии;
- 0.0d подтвердил чтение реальных карт Chord / Key / Tempo / Time Signature с позициями на таймлайне.

## Архитектура 0.0e

Из-за поведения Studio Pro текущий прототип разделён на два очень лёгких VST3-компонента в одном пакете:

```text
Smart Voicing 0.0e/
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
- получает готовый harmonic-context snapshot от `Smart Voicing ARA`;
- в 0.0e содержит первый рабочий **Context Monitor** с крупным отображением текущего аккорда, тональности, размера, темпа и позиции.

### Smart Voicing ARA.vst3

Служебный компонент:

- загружается как ARA/Event FX на любой Audio Event;
- пропускает аудио без изменений;
- читает `Musical Context` проекта через ARA;
- отслеживает изменения Chord Track;
- публикует harmonic-context snapshot для основного `Smart Voicing`;
- в 0.0e дополнительно публикует текущую transport-позицию для Instrument.

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

Текущий Windows PoC использует минимальный named shared-memory bridge без файлового I/O.

В 0.0e используется bridge ABI v3:

- harmonic maps и transport position имеют отдельные revision/seqlock;
- изменение позиции транспорта не увеличивает harmonic revision;
- ARA model updates по-прежнему хорошо видны через отдельный harmonic revision;
- transport обновляется lock-free из `processBlock` ARA/Event FX;
- основной Instrument выбирает shared ARA transport как приоритетный источник позиции, а собственный host playhead использует как fallback.

Это позволяет Instrument определять активные `Chord / Key / Time Signature / Tempo`, даже если его собственный `processBlock` в данный момент не получает позицию от хоста.

## Что проверяет 0.0e

Следующий тест Этапа 1:

- совпадает ли Context Monitor Instrument с фактическим Chord Track на текущей позиции;
- обновляется ли позиция Instrument при playback;
- обновляется ли позиция после перемещения курсора при остановленном транспорте;
- корректно ли выбирается аккорд точно на границе его смены;
- корректно ли переключаются Key Signature и Time Signature;
- увеличивается ли harmonic revision только при изменении карты, а transport revision — при движении транспорта;
- корректно ли восстанавливается контекст после сохранения и повторного открытия проекта.

После подтверждения этих пунктов останется довести Этап 1 до версии **0.1**.

## Принципы архитектуры

- музыкальная логика не зависит от JUCE, ARA и конкретной DAW;
- хостовые возможности определяются через capability detection, а не по имени DAW;
- ARA-контекст кэшируется вне real-time audio thread;
- MIDI-обработка должна быть real-time safe;
- transport-публикация из audio thread не должна использовать mutex, allocation или файловый I/O;
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

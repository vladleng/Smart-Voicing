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

Текущая рабочая версия: **Smart Voicing 0.0c**.  
Текущий этап: **Этап 1 — ARA Context Proof of Concept**.

На этом этапе уже подтверждено в Fender Studio / Studio Pro:

- ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- инструментальный экземпляр может получить ARA binding, но Studio Pro не прикрепляет к нему `Musical Context`;
- изменения Chord Track приходят через ARA без перезапуска плагина;
- изменение или добавление аккорда увеличивает `Bridge revision` в основном инструментальном экземпляре;
- длина аудио Event, на котором установлен ARA-компонент, не ограничивает доступ к гармоническому контексту: playhead может находиться за пределами Event, а изменения Chord Track продолжают поступать;
- следовательно, Audio Event используется как **ARA-якорь**, а не как временное окно действия гармонии.

## Архитектура 0.0c

Из-за поведения Studio Pro текущий прототип разделён на два очень лёгких VST3-компонента в одном пакете:

```text
Smart Voicing 0.0c/
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
- публикует компактный harmonic-context snapshot для основного `Smart Voicing`.

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

## Что проверяется дальше

Следующий шаг Этапа 1:

- читать реальные значения `Sheet Chords`, а не только количество событий;
- читать реальные значения `Key Signatures`;
- передавать через bridge аккорды и тональности вместе с позициями на таймлайне;
- определять текущие Chord и Key по позиции транспорта;
- отдельно подтвердить live update Key Track;
- проверить сохранение и восстановление состояния проекта.

После этого Этап 1 можно будет завершить версией **0.1**.

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

Условная структура данных:

```cpp
struct HarmonicContext
{
    KeySignature key;
    ChordSymbol chord;
    double timelinePosition;
    bool hasKey;
    bool hasChord;
};
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

Его задача — быстро получить музыкально осмысленный и редактируемый MIDI-скелет:

```text
DAW Chord / Key context
        ↓
Melody / Chord input
        ↓
Smart Voicing
        ↓
Separate MIDI voices
        ↓
ручная редактура партий
        ↓
артикуляции / expression / vibrato
        ↓
финальная аранжировка
```

Музыкант остаётся ответственным за окончательное музыкальное решение.

## Ориентиры

Divisimate и Scaler используются как референсы по отдельным функциям, но Smart Voicing не должен их копировать.

Ключевое отличие проекта — использование **родного гармонического контекста DAW**, если хост способен его предоставить.

Полная концепция и технические заметки находятся в [`docs/CONCEPT.md`](docs/CONCEPT.md).

## Язык ведения проекта

README, концепция, этапы разработки, GitHub Issues, задачи, подзадачи, roadmap и пояснения ведутся **на русском языке**.

Английский используется там, где он необходим технически: имена классов и методов, API, SDK, форматы, идентификаторы, код и общепринятые технические термины.

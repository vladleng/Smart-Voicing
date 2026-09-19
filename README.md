# Smart Voicing

Экспериментальный MIDI-плагин для гармонизации и распределения голосов от **Moon River Studio**.

Smart Voicing задуман как инструмент, который использует гармонический контекст самой DAW — аккорды, тональность и позицию на таймлайне — и объединяет его с входящим MIDI, чтобы генерировать музыкально осмысленные независимые голоса для духовой секции, ансамбля и других инструментов.

## Основная идея

Целевой workflow:

```text
Chord Track + Key Track DAW
        +
     MIDI input
        +
   Voicing preset
        ↓
   Smart Voicing
        ↓
Независимые MIDI-голоса
```

DAW остаётся источником гармонической истины. Если хост уже содержит Chord Track и Key Track, пользователь не должен вручную создавать вторую параллельную гармоническую карту внутри плагина.

## Текущий этап разработки

Сейчас проект находится на **Этапе 1 — ARA Context Proof of Concept**.

Рабочая версия: **Smart Voicing 0.0c**.

Главная задача этапа — подтвердить, что сторонний плагин может получать гармонический контекст проекта через ARA 2 и передавать его MIDI-инструменту в реальном времени.

## Что уже подтверждено в Studio Pro / Fender Studio

В процессе разработки выяснилось следующее:

- ARA/Event FX получает `Musical Context` проекта;
- доступны `Key Signatures`, `Sheet Chords`, `Tempo Entries` и `Bar Signatures`;
- Instrument-экземпляр может получить ARA binding, но Studio Pro не прикрепляет к нему `Musical Context`;
- поэтому один Instrument не может напрямую читать Chord Track / Key Track через ARA;
- изменения Chord Track приходят через ARA без перезагрузки плагина;
- изменение или добавление аккорда увеличивает `Bridge revision` в основном Smart Voicing;
- длина Audio Event, на котором находится ARA-плагин, не ограничивает область доступного гармонического контекста;
- playhead может находиться за пределами этого Event, а изменения Chord Track всё равно продолжают поступать в основной инструмент.

Последний пункт означает, что Audio Event используется только как **ARA-якорь**, а не как временное окно действия гармонии.

## Архитектура версии 0.0c

Из-за особенностей Studio Pro проект разделён на два лёгких VST3-компонента:

```text
Smart Voicing 0.0c/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной Instrument / MIDI engine:

- принимает MIDI;
- выдаёт MIDI;
- в дальнейшем будет выполнять harmonization, voicing и voice allocation;
- не запрашивает ARA напрямую;
- получает актуальный harmonic context от `Smart Voicing ARA`.

### Smart Voicing ARA.vst3

Служебный ARA/Event FX:

- устанавливается на любой Audio Event;
- пропускает аудио без изменений;
- получает `Musical Context` проекта через ARA;
- читает Chord / Key / Tempo / Bar data;
- публикует компактный snapshot для основного Smart Voicing.

На Windows связь между двумя VST3-модулями реализована через минимальный named shared-memory bridge без файлового I/O.

Текущая подтверждённая схема:

```text
Studio Pro Chord / Key Track
            ↓
           ARA
            ↓
    Smart Voicing ARA
            ↓
    shared context bridge
            ↓
      Smart Voicing
            ↓
       MIDI voices
```

## Следующий шаг

Сейчас bridge уже передаёт наличие ARA-контекста, количество событий и revision.

Следующая задача — передавать реальные значения:

```text
1.1  Am
3.1  F
5.1  C
7.1  E7

Key: A minor
```

После этого основной Smart Voicing должен определять текущий Chord и Key по позиции транспорта и использовать их для MIDI-гармонизации.

## Архитектурный принцип

Музыкальное ядро не должно зависеть от конкретной DAW или ARA.

Планируемая абстракция:

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

ARA — только один из способов получить гармонический контекст. Если конкретный хост не предоставляет Chord/Key через ARA, Smart Voicing в будущем должен уметь использовать MIDI- или Manual-context.

## Планируемые музыкальные режимы

После завершения ARA-этапа:

- Direct 4 Voice;
- Melody Harmonize;
- Chord redistribution / revoicing;
- Context-aware voicing.

Первые типы voicing:

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

## Первый практический состав

Начальный сценарий — духовая секция из четырёх голосов:

```text
Voice 1 → Trumpet
Voice 2 → Tenor Sax
Voice 3 → Trombone
Voice 4 → Baritone Sax
```

Но состав не должен быть зашит в движок. В дальнейшем каждый голос получит собственный инструментальный профиль и диапазон.

## Приоритет музыкальной логики

Долгосрочная схема приоритетов:

```text
1. Сыгранная нота / мелодия
2. Текущий аккорд
3. Текущая тональность
4. Гармоническая функция
5. Voice leading
6. Диапазоны инструментов
7. Выбранный voicing
```

Локальный аккорд должен иметь приоритет над механическим следованием основной гамме. Например, при `C major` и аккорде `A7` движок должен понимать необходимость `C#`.

## Производительность

Smart Voicing должен оставаться максимально лёгким:

- минимальная нагрузка CPU;
- минимальное потребление памяти;
- real-time-safe MIDI processing;
- отсутствие тяжёлых фоновых процессов;
- отсутствие постоянного polling хоста из audio thread;
- кэширование harmonic context вне real-time потока;
- простая UI-часть;
- музыкальная логика отдельно от JUCE/ARA wrapper-кода.

## Reference products

Полезные ориентиры:

- **Divisimate** — live routing и распределение голосов;
- **Scaler** — harmonic context, divisi, voicing и multi-channel MIDI.

Smart Voicing не должен копировать эти продукты. Его основное отличие — использование **гармонической карты самой DAW**, когда хост её предоставляет.

## Roadmap

- **Этап 0** — каркас проекта и базовая сборка — завершён.
- **Этап 1** — ARA Context Proof of Concept — текущий этап.
- **Этап 2** — MIDI Router.
- **Этап 3** — Chord-aware harmonizer.
- **Этап 4** — Key-aware engine.
- **Этап 5** — Jazz voicing.
- **Этап 6** — Voice leading.
- **Этап 7** — Instrument profiles и presets.

## Философия проекта

Smart Voicing не должен автоматически создавать «готовую аранжировку».

Его задача — быстро создать качественный редактируемый каркас партий:

```text
Chord / Key DAW
      ↓
Melody / chord input
      ↓
Smart Voicing
      ↓
Separate MIDI voices
      ↓
Ручная редактура
      ↓
Articulations / Expression / Vibrato
      ↓
Финальная аранжировка
```

Музыкант остаётся главным автором результата.

## Документация

Подробная концепция и технические заметки находятся в [`docs/CONCEPT.md`](docs/CONCEPT.md).

## Язык ведения проекта

Все этапы разработки, GitHub Issues, задачи, подзадачи, roadmap и пояснения ведутся **на русском языке**.

Английский используется только там, где это технически необходимо: API, имена классов и методов, форматы, SDK, идентификаторы и код.

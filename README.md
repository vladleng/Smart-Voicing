# Smart Voicing

**Smart Voicing** — экспериментальный MIDI-плагин Moon River Studio для гармонизации и распределения независимых голосов с использованием гармонического контекста самой DAW.

Главная идея:

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

DAW остаётся **источником гармонической истины**. Smart Voicing не должен требовать отдельную копию Chord Track / Key Track внутри плагина.

## Текущий статус

Проект находится на стадии **pre-alpha**.

Последняя завершённая версия: **Smart Voicing 0.1**.  
Текущая рабочая версия: **Smart Voicing 0.1e**.  
**Этап 1 — ARA Context Proof of Concept завершён.**  
**Текущий этап: Этап 2 — MIDI Router.**

## Подтверждено в Studio Pro

- `Smart Voicing ARA` как ARA/Event FX получает Musical Context проекта;
- доступны Key Signatures, Sheet Chords, Tempo Entries и Bar Signatures;
- изменения Chord / Key / Time Signature приходят live без Reload;
- Audio Event служит ARA-якорем и не ограничивает диапазон считывания контекста;
- основной Instrument получает карты и transport через shared-memory bridge;
- Context Monitor показывает текущие Chord / Key / Time Signature / Tempo;
- MIDI output Smart Voicing можно использовать как источник downstream Instrument Tracks;
- Studio Pro предоставляет MIDI Input 1–16;
- Voice 1–4 реально разведены по MIDI Channels/Input 1–4 на четыре отдельных SWAM-инструмента;
- CC / automation / Pitch Bend проходят downstream;
- Voice Stack, legato/portamento gesture и смена аккордов под Sustain подтверждены практическим тестом;
- ARA Context и MIDI Router работают параллельно.

## Пакет

```text
Smart Voicing <version>/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной Instrument / MIDI engine:

- принимает и выдаёт MIDI;
- получает harmonic-context snapshot от ARA-компонента;
- содержит Context Monitor и MIDI Router;
- далее получит harmonizer, voicing и voice leading.

### Smart Voicing ARA.vst3

Служебный ARA/Event FX reader:

- устанавливается на любой Audio Event;
- пропускает аудио без изменений;
- читает Musical Context через ARA;
- публикует harmonic context и transport для основного Instrument.

## Shared bridge

Текущий Windows-прототип использует named shared memory без файлового I/O.

Bridge ABI v3 разделяет harmonic snapshot и transport snapshot. Transport публикуется только при фактическом изменении и остаётся real-time safe: без mutex, allocation и файлового I/O в audio callback.

## Boundary semantics

Для Chord / Key / Time Signature:

```text
до границы    → предыдущий контекст
на границе    → новый контекст
после границы → новый контекст
```

Для компенсации floating-point расхождения host transport и ARA positions используется `boundary tolerance = 0.0001 PPQ`.

## Этап 2 — MIDI Router

Цель этапа — получить стабильные независимые Voice 1–4, пригодные для последующего harmonizer engine.

### 0.1a — MIDI Router Probe

- transparent MIDI pass-through;
- Note On / Note Off / CC / Pitch Bend diagnostics;
- подтверждение downstream routing в Studio Pro.

Тест: [`docs/TEST-0.1a.md`](docs/TEST-0.1a.md).

### 0.1b — Direct 4 Voice Router

- ноты распределяются сверху вниз;
- Voice 1 → Ch1, Voice 2 → Ch2, Voice 3 → Ch3, Voice 4 → Ch4;
- channel MIDI messages broadcast на Ch1–4;
- подтверждено реальное разделение четырёх SWAM-инструментов.

Тест: [`docs/TEST-0.1b.md`](docs/TEST-0.1b.md).

### 0.1c — Stable Voice Ownership + Sustain

- Voice slots сохраняют channel identity;
- движение одного голоса больше не пересортировывает соседние Voice;
- Router учитывает CC64 во внутреннем ownership state.

Тест: [`docs/TEST-0.1c.md`](docs/TEST-0.1c.md).

### 0.1d — Voice Stack / Legato + Sustain Chord Morph

- каждый Voice имеет фиксированный note stack;
- overlap `Note On` остаётся на том же Voice/channel и даёт mono-инструментам legato/portamento cue;
- Sustain не замораживает voicing: следующий аккорд можно сыграть до pedal-up;
- pedal-up удаляет только физически отпущенные старые ноты;
- четыре Voice сохраняют устойчивую идентичность.

Тест: [`docs/TEST-0.1d.md`](docs/TEST-0.1d.md).

### 0.1e — Distribution Modes + Gesture Classifier

Добавлены три базовых режима распределения:

```text
1. Сверху вниз
2. Снизу вверх
3. Заполнить 4 голоса
```

Правила первых двух режимов:

- `Сверху вниз`: при 1–4 нотах заполняются V1, V2, V3, V4 сверху вниз;
- `Снизу вверх`: при 1–4 нотах заполняются V4, V3, V2, V1 снизу вверх;
- при chord gesture из 5+ нот в output остаются максимум четыре независимых Voice.

`Заполнить 4 голоса`:

```text
1 нота → V1=V2=V3=V4
2 ноты → V1=V2=верхняя, V3=V4=нижняя
3 ноты → V1=верхняя, V2=средняя, V3=V4=нижняя
4 ноты → по одной ноте на V1–V4
```

Внутренняя ownership-модель использует `note → Voice mask`, поэтому одна физическая нота может корректно управлять несколькими Voice одновременно.

Gesture Classifier различает:

- **Chord Gesture** — короткая группа нот (окно около 45 ms): формируется максимум четырёхголосный frame; пятая и последующие chord-notes не превращаются в дополнительную ноту внутри одного Voice;
- **Voice Gesture** — более поздняя отдельная нота: трактуется как continuation ближайшего Voice и сохраняет same-channel legato/portamento behaviour;
- **Sustain Chord Morph** — новый chord gesture может заменить текущий frame при pedal-down, старые sustain-held ноты остаются только как внутренний stack до pedal-up.

Выбранный режим сохраняется в plugin state и восстанавливается при повторном открытии проекта.

Тест: [`docs/TEST-0.1e.md`](docs/TEST-0.1e.md).

Текущие задачи: [Issue #17 — Этап 2: MIDI Router](https://github.com/vladleng/Smart-Voicing/issues/17).

## Архитектурные принципы

- musical core не должен зависеть от конкретной DAW;
- host capabilities определяются по возможностям, а не имени приложения;
- ARA-контекст кэшируется вне real-time audio thread;
- MIDI processing должен оставаться real-time safe;
- CPU и память используются экономно;
- UI остаётся диагностическим и минимальным до стабилизации engine;
- итог Smart Voicing — редактируемый MIDI-скелет, а не автоматически законченная аранжировка.

Планируемый слой harmonic context:

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

## Roadmap

- **Этап 0** — каркас проекта и базовая сборка. ✅
- **Этап 1** — ARA Context Proof of Concept. ✅ → `0.1`
- **Этап 2** — MIDI Router. 🚧 → `0.1a ... 0.2`
- **Этап 3** — Chord-aware harmonizer.
- **Этап 4** — Key-aware engine.
- **Этап 5** — Jazz voicing.
- **Этап 6** — Voice leading.
- **Этап 7** — Instrument profiles / presets.

## Версионирование

Рабочие версии этапа используют буквенные суффиксы:

```text
0.1a → 0.1b → 0.1c → 0.1d → 0.1e → ... → 0.2
```

Внутренний CMake version остаётся числовым (`0.1e` → `0.1.5`). Подробно: [`docs/VERSIONING.md`](docs/VERSIONING.md).

## Язык проекта

README, концепция, задачи, Issues, roadmap и пояснения ведутся на русском языке. Английский используется для API, SDK, идентификаторов, кода и общепринятых технических терминов.

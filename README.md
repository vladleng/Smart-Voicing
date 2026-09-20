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
Текущая рабочая версия: **Smart Voicing 0.1d**.  
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
- Router учитывает CC64 во внутреннем ownership state;
- выявлено ограничение: Sustain слишком жёстко удерживал старый voicing и блокировал новый аккорд до pedal-up;
- выявлена необходимость настоящего overlap-legato внутри каждого Voice.

Тест: [`docs/TEST-0.1c.md`](docs/TEST-0.1c.md).

### 0.1d — Voice Stack / Legato + Sustain Chord Morph

У каждого Voice появился собственный фиксированный note stack.

```text
Voice 1 / Ch1
├── previous held/sustain note
└── current top note
```

Основные правила:

- overlap `Note On` отправляется на тот же Voice/channel **до** `Note Off` предыдущей ноты;
- для mono/physical-model destination instruments это является стандартным MIDI cue для legato / portamento;
- если верхняя overlap-нота отпущена, предыдущая физически удерживаемая нота остаётся в downstream mono stack;
- Sustain больше не замораживает Voice slots: следующий аккорд можно сыграть при pedal-down;
- старые sustain-held ноты остаются ниже нового аккорда до pedal-up;
- при pedal-up удаляются только ноты, клавиши которых уже отпущены;
- новый физически удерживаемый аккорд продолжает звучать;
- repeat sustain-held note остаётся на том же Voice/channel;
- single/few-note continuation выбирает ближайший Voice;
- chord-sized continuation сохраняет вертикальный порядок Voice 1→4;
- Router Monitor показывает текущую ноту и `[stack N]` каждого Voice;
- router state использует фиксированные массивы без heap allocation и mutex.

Тест: [`docs/TEST-0.1d.md`](docs/TEST-0.1d.md).

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
0.1a → 0.1b → 0.1c → 0.1d → ... → 0.2
```

Внутренний CMake version остаётся числовым (`0.1d` → `0.1.4`). Подробно: [`docs/VERSIONING.md`](docs/VERSIONING.md).

## Язык проекта

README, концепция, задачи, Issues, roadmap и пояснения ведутся на русском языке. Английский используется для API, SDK, идентификаторов, кода и общепринятых технических терминов.

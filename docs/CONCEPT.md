# Smart Voicing — концепция и заметки разработки

**Статус:** pre-alpha / архитектурный прототип  
**Рабочее название:** Smart Voicing  
**Основная цель:** создать лёгкий, host-agnostic MIDI-плагин для гармонизации и распределения голосов, который может использовать гармонический контекст самой DAW.

---

## 1. Идея продукта

Smart Voicing должен позволять музыканту работать напрямую с гармонической структурой, уже созданной в DAW.

Целевой рабочий процесс:

1. Создать последовательность аккордов в Chord Track DAW.
2. Задать Key / tonal context проекта.
3. Загрузить Smart Voicing.
4. Выбрать режим harmonization / voicing или preset.
5. Подготовить несколько целевых инструментальных дорожек.
6. Нажать Record.
7. Играть мелодию или аккордовый материал.
8. Smart Voicing в реальном времени создаёт и распределяет отдельные музыкальные голоса.
9. Получившиеся партии можно редактировать по отдельности.

Целевой пользовательский опыт:

```text
DAW Chord + Key context
        +
     MIDI input
        +
  voicing preset
        ↓
   Smart Voicing
        ↓
 independent voices
        ↓
separate instrument tracks
```

---

## 2. Главный принцип: DAW остаётся источником гармонической истины

Smart Voicing не должен заставлять пользователя создавать вторую независимую гармоническую карту внутри плагина, если хост уже содержит Chord Track и Key Track.

Предпочтительный контекст:

```text
KEY / TONALITY
+
CURRENT CHORD
+
TIMELINE POSITION
```

Далее harmonizer объединяет этот контекст с:

- входящей melody note;
- сыгранным аккордом;
- выбранным voicing mode;
- voice-leading rules;
- диапазонами инструментов;
- будущими arrangement rules.

---

## 3. Host-agnostic архитектура

Smart Voicing не должен быть привязан к Fender Studio / Studio Pro или любой другой конкретной DAW.

Fender Studio / Studio Pro используется как первый reference host для разработки и тестирования, но внутренняя архитектура должна оставаться нейтральной к хосту.

Концептуально:

```text
┌──────────────────────────────────────┐
│            Harmony Core              │
│                                      │
│ chord interpretation                 │
│ key context                          │
│ harmonic functions                   │
│ voicing                              │
│ guide tones                          │
│ voice leading                        │
│ instrument ranges                    │
└──────────────────┬───────────────────┘
                   │
          Host-neutral context
                   │
      ┌────────────┴────────────┐
      │                         │
┌─────▼─────────┐         ┌─────▼────────┐
│ Context       │         │ MIDI / Plug- │
│ Providers     │         │ in Adapters  │
└─────┬─────────┘         └──────────────┘
      │
 ┌────┼──────────────┐
 ▼    ▼              ▼
ARA  MIDI          Manual
```

Voicing engine должен получать нейтральную структуру данных и не знать, откуда именно пришёл гармонический контекст.

Пример концептуального интерфейса:

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

Планируемые providers:

```text
IHarmonicContextProvider
├── ARAContextProvider
├── MidiContextProvider
└── ManualContextProvider
```

DAW-specific provider следует вводить только если конкретный хост действительно требует отдельного workaround.

---

## 4. ARA 2 — первый приоритет разработки

Первый полноценный этап проекта — не четырёхголосная гармонизация, а **ARA Context Proof of Concept**.

Задача этого этапа — ответить на вопросы:

- доступна ли ARA-интеграция в хосте;
- отдаёт ли хост chord data;
- отдаёт ли Key Signature / tonal-context data;
- можно ли отслеживать эти данные по таймлайну;
- можно ли получать live updates после редактирования Chord Track / Key Track;
- можно ли безопасно связать ARA-контекст с real-time MIDI processing;
- какие возможности отличаются между хостами.

На этом этапе полноценный voicing engine не нужен.

---

## 5. Что уже подтверждено в Studio Pro / Fender Studio

Эксперименты 0.0b и 0.0c показали следующее.

### 5.1 ARA/Event FX получает Musical Context

При загрузке плагина как ARA/Event FX Studio Pro предоставляет:

- `Key Signatures`;
- `Sheet Chords`;
- `Tempo Entries`;
- `Bar Signatures`;
- `Musical Context` проекта.

Подтверждённые диагностические состояния:

```text
ARA instance bound: YES
ARA document controller: YES
Host content access: YES
Musical contexts: 1
```

### 5.2 Instrument role не получает Musical Context напрямую

Тот же код, объявленный как Instrument, может получить ARA binding, однако Studio Pro не прикрепляет к такому экземпляру Musical Context:

```text
ARA instance bound: YES
ARA document controller: YES
Host content access: YES
Musical contexts: 0
```

Следствие: основной MIDI-инструмент не может надёжно получать Chord / Key напрямую через ARA в этом хосте.

### 5.3 Один VST3 не удалось использовать одновременно как удобный Instrument и Event FX

Эксперимент с одним бинарником `Instrument + Fx + ARA` показал, что Studio Pro фактически классифицирует такой плагин как Instrument и не даёт использовать его как обычный Event FX в нужном workflow.

Поэтому для текущего MVP принята двухкомпонентная схема.

---

## 6. Архитектура версии 0.0c

Пакет состоит из двух VST3-компонентов:

```text
Smart Voicing 0.0c/
├── Smart Voicing.vst3
└── Smart Voicing ARA.vst3
```

### Smart Voicing.vst3

Основной Instrument / MIDI engine:

- MIDI input;
- MIDI output;
- будущие функции harmonizer / voicing / voice leading;
- чтение уже подготовленного harmonic-context snapshot;
- отсутствие прямой зависимости от ARA lifecycle.

### Smart Voicing ARA.vst3

Служебный ARA/Event FX reader:

- загружается на Audio Event;
- пропускает аудио без изменений;
- получает Musical Context от хоста;
- читает доступные ARA content types;
- реагирует на изменения гармонического контекста;
- публикует обновлённый snapshot для основного Instrument.

Подтверждённая схема:

```text
Studio Pro Chord / Key Track
        ↓ ARA
Smart Voicing ARA
        ↓ shared harmonic context
Smart Voicing Instrument
        ↓
MIDI processing / generated voices
```

---

## 7. Shared bridge между двумя компонентами

Два отдельных VST3 bundle являются разными DLL-модулями, поэтому обычные C++ static-объекты между ними не разделяются.

Для Windows Proof of Concept используется минимальный named shared-memory bridge.

Он нужен только как транспорт между:

```text
Smart Voicing ARA
        ↓
shared memory snapshot
        ↓
Smart Voicing Instrument
```

Требования к bridge:

- без файлового I/O;
- без сети;
- без тяжёлого background process;
- компактный snapshot;
- revision counter для определения изменений;
- безопасное чтение стабильного snapshot;
- отсутствие прямого обращения к ARA из audio thread основного Instrument.

В 0.0c уже подтверждено, что Instrument получает через bridge те же counts для Key / Chord / Tempo / Bar, которые видит ARA/Event FX.

---

## 8. Live update гармонии

Подтверждено в Studio Pro:

- добавление аккорда в Chord Track увеличивает `Bridge revision`;
- замена аккорда также увеличивает `Bridge revision`;
- Reload плагина не требуется;
- перезапуск проекта не требуется.

Следовательно, ARA model updates пригодны для live synchronization гармонической карты.

Рабочая логика должна быть такой:

```text
Chord Track изменён
        ↓
ARA сообщает об обновлении Musical Context
        ↓
Smart Voicing ARA перечитывает актуальный context
        ↓
создаётся новый snapshot
        ↓
revision++
        ↓
Smart Voicing Instrument видит новую версию
```

Проверка live update Key Track остаётся отдельной задачей.

---

## 9. Audio Event используется только как ARA-якорь

В ходе тестирования 0.0c подтверждено важное поведение Studio Pro:

- длина Audio Event, на котором установлен `Smart Voicing ARA`, не ограничивает диапазон доступного Musical Context;
- playhead может находиться за пределами этого Event;
- при этом изменения Chord Track продолжают поступать в Smart Voicing Instrument;
- гармоническая карта не ограничивается временными границами Event.

Следовательно, Audio Event нужен как **ARA-якорь**, а не как временное окно действия гармонии.

Практический workflow может быть очень простым:

```text
короткий служебный Audio Event
└── Smart Voicing ARA

остальной проект
──────────────────────────────▶
Chord / Key context продолжает обновляться
```

Это означает, что не нужно создавать или растягивать служебный WAV на всю длину аранжировки.

---

## 10. ARA content types, представляющие интерес

Основные данные:

- `Key Signatures` / tonal context;
- `Sheet Chords`;
- `Tempo Entries`;
- `Bar Signatures`;
- timeline / musical position.

Позже при необходимости могут быть рассмотрены другие типы ARA content.

Важный принцип:

> ARA определяет модель интеграции и типы контента, но конкретный хост может предоставлять только часть возможностей.

Поэтому Smart Voicing должен использовать **capability detection**, а не предполагать одинаковое поведение всех DAW.

---

## 11. Capability-based behavior

Плагин должен уметь сообщить, что именно предоставляет текущий host.

Пример:

```text
HOST INTEGRATION

ARA 2                 ✓
Chord context         ✓
Key signatures        ✓
Tempo / timeline      ✓

Context source:
● Host
○ MIDI
○ Manual
```

В другом хосте:

```text
HOST INTEGRATION

ARA 2                 ✓
Chord context         —
Key signatures        —

Context source:
○ Host
● MIDI
○ Manual
```

Smart Voicing должен оставаться полезным даже если host не предоставляет Chord / Key через ARA.

---

## 12. Планируемые музыкальные режимы

### 12.1 Direct mode

Пользователь играет четыре ноты.

Smart Voicing распределяет их между четырьмя голосами:

```text
Highest note → Voice 1
2nd          → Voice 2
3rd          → Voice 3
Lowest       → Voice 4
```

### 12.2 Melody Harmonize

Пользователь играет одну мелодическую линию.

Smart Voicing использует текущие Chord + Key для построения остальных голосов.

Пример:

```text
Key: C major
Chord: C7
Melody: D
```

Возможный результат:

```text
Trumpet    D   = 9
Tenor      Bb  = b7
Trombone   E   = 3
Baritone   C   = root
```

### 12.3 Chord mode

Пользователь играет аккорд.

Smart Voicing перераспределяет / revoice сыгранный материал с учётом выбранного preset и диапазонов инструментов.

### 12.4 Context-aware mode

Приоритеты долгосрочной логики:

```text
1. Melody note / played material
2. Current chord
3. Current key / tonal context
4. Harmonic function
5. Voice leading
6. Instrument ranges
7. Selected voicing rules
```

Локальный аккорд должен при необходимости иметь больший приоритет, чем строгая диатоника.

Пример:

```text
Key: C major
Chord: A7
```

Smart Voicing должен понимать необходимость C#, несмотря на то что нота не входит в C major.

---

## 13. Планируемые voicing types

- Close;
- Drop 2;
- Drop 3;
- Drop 2+4;
- Spread;
- Unison;
- Guide Tones;
- Custom.

Практический первый набор после завершения ARA-этапа:

```text
Close
Drop 2
Guide Tones
Custom
```

---

## 14. Guide-tone concept

Guide-tone mode особенно важен для джазовой аранжировки.

Возможные правила:

- 3rd и 7th получают высокий приоритет;
- melody / tensions могут оставаться в верхнем голосе;
- root может быть исключён, если его уже ясно задаёт bass;
- средние и нижние голоса сохраняют функцию аккорда с минимальным движением.

Пример:

```text
Key: C
Chord: G7
Melody: A
```

Возможное распределение:

```text
Trumpet    A   = 9
Tenor      F   = b7
Trombone   B   = 3
Baritone   D   = 5
```

---

## 15. Voice leading

Smart Voicing не должен строить каждый аккорд полностью независимо от предыдущего.

Нужно сохранять состояние каждого голоса и выбирать следующий вариант с музыкально разумным движением.

Базовая MVP cost function может начинаться так:

```text
cost =
|voice1_new - voice1_old| +
|voice2_new - voice2_old| +
|voice3_new - voice3_old| +
|voice4_new - voice4_old|
```

Позже можно добавлять penalties за:

- voice crossing;
- слишком большие скачки;
- выход из playable / comfortable range;
- нежелательные doubling;
- плохое расположение guide tones;
- разрушение верхней мелодической линии.

---

## 16. Instrument ranges

Первый практический use case — квартет духовых:

```text
Voice 1 → Trumpet
Voice 2 → Tenor Sax
Voice 3 → Trombone
Voice 4 → Baritone Sax
```

Однако этот состав нельзя жёстко зашивать в движок.

Каждый голос в дальнейшем должен поддерживать:

- minimum note;
- maximum note;
- comfortable range;
- octave displacement;
- instrument profile.

Потенциальные профили:

- Trumpet;
- Flugelhorn;
- Alto Sax;
- Tenor Sax;
- Baritone Sax;
- Trombone;
- Horn;
- custom user profile.

---

## 17. MIDI output и workflow записи

Целевая модель — один независимый голос на один destination instrument.

Начальная схема:

```text
Voice 1 → MIDI Channel 1
Voice 2 → MIDI Channel 2
Voice 3 → MIDI Channel 3
Voice 4 → MIDI Channel 4
```

Пример:

```text
Smart Voicing
    │
    ├── CH1 → Trumpet instrument
    ├── CH2 → Tenor instrument
    ├── CH3 → Trombone instrument
    └── CH4 → Baritone instrument
```

Желаемый workflow:

```text
set chords + key
      ↓
choose voicing preset
      ↓
arm destination tracks
      ↓
press Record
      ↓
play melody or chords
      ↓
Smart Voicing generates separate voices
```

Отдельный технический вопрос для каждого хоста:

> Может ли DAW напрямую записывать MIDI, сгенерированный Smart Voicing, на несколько вооружённых дорожек за один проход?

Если нет, позднее могут понадобиться fallback-варианты:

- internal capture;
- commit / render to MIDI;
- drag-and-drop MIDI parts;
- другой лёгкий routing mechanism.

Это не нужно решать раньше завершения ARA Proof of Concept.

---

## 18. Standalone host не является исходной архитектурой

Divisimate-style external routing полезен для live MIDI distribution, но основное отличие Smart Voicing — непосредственная осведомлённость о гармонической карте DAW.

Поэтому standalone application / host не входит в начальную реализацию.

В будущем он может быть рассмотрен как дополнительный routing layer, но не как источник гармонической истины.

---

## 19. Performance goals

Smart Voicing должен оставаться намеренно лёгким.

Цели:

- минимальный CPU usage;
- минимальный memory footprint;
- минимальный UI overhead;
- real-time-safe MIDI processing;
- отсутствие ненужных background threads;
- отсутствие тяжёлых внешних host processes;
- harmony core отделён от GUI и wrapper-кода;
- внешние зависимости вводятся только при реальной пользе;
- ARA content не читается напрямую из real-time audio thread;
- real-time часть получает уже готовый immutable / lightweight snapshot.

---

## 20. Reference products

### Divisimate

Полезен как reference для:

- live routing;
- voice distribution;
- multi-instrument workflows;
- performance-oriented MIDI splitting.

### Scaler

Полезен как reference для:

- Divisi;
- multi-channel MIDI output;
- voice grouping;
- chord-follow behavior;
- voicing concepts;
- внутреннего harmonic context.

Smart Voicing не должен пытаться повторить весь функционал Scaler.

Основное отличие:

> Smart Voicing должен использовать собственную гармоническую карту DAW, когда хост способен её предоставить, вместо создания второй параллельной chord timeline внутри плагина.

---

## 21. Предлагаемая структура репозитория

Долгосрочно структура может развиваться к виду:

```text
Smart-Voicing/
│
├── src/
│   ├── core/
│   │   ├── harmony/
│   │   ├── voicing/
│   │   ├── voice_leading/
│   │   └── instruments/
│   │
│   ├── context/
│   │   ├── IHarmonicContextProvider.h
│   │   ├── ARAContextProvider.cpp
│   │   ├── MidiContextProvider.cpp
│   │   └── ManualContextProvider.cpp
│   │
│   ├── plugin/
│   │   ├── instrument/
│   │   └── ara_bridge/
│   │
│   └── formats/
│       ├── vst3/
│       └── other/
│
├── tests/
├── docs/
└── third_party/
```

JUCE может использоваться как platform / plug-in abstraction layer, но музыкальная логика по возможности должна оставаться независимой от JUCE.

---

## 22. Roadmap

### Этап 0 — каркас проекта и базовая сборка

- базовый JUCE / VST3 project;
- Windows CI;
- минимальный UI;
- MIDI/audio pass-through;
- структура репозитория;
- базовая документация.

Результат: `0.0a`.

### Этап 1 — ARA Context Proof of Concept

Цель:

- ARA Document Controller;
- capability detection;
- Key / Chord / Tempo / Bar content;
- live updates;
- двухкомпонентная схема ARA reader + Instrument;
- bridge между компонентами;
- чтение реальных Chord / Key значений;
- определение текущего контекста по позиции.

Рабочие версии: `0.0b`, `0.0c`, далее при необходимости.  
Готовая версия этапа: `0.1`.

### Этап 2 — MIDI Router

- MIDI input;
- note tracking;
- четыре output voices / channels;
- корректные Note On / Note Off;
- sustain handling;
- real-time safety.

### Этап 3 — Chord-aware harmonizer

- single-note melody input;
- current-chord interpretation;
- chord-tone generation;
- Close voicing.

### Этап 4 — Key-aware engine

- current key;
- harmonic function;
- secondary / chromatic dominants;
- chord-vs-key priority.

### Этап 5 — Jazz voicing

- Drop 2;
- Guide Tones;
- root omission;
- tensions;
- basic context-aware rules.

### Этап 6 — Voice leading

- previous-voice state;
- minimal-movement search;
- range constraints;
- voice-crossing penalties.

### Этап 7 — Instrument profiles и presets

- editable ranges;
- instrument profiles;
- custom routing / voicing presets.

---

## 23. Что пока не нужно строить

Не расширять scope раньше времени следующими функциями:

- standalone application;
- сложный визуальный routing;
- огромные preset libraries;
- articulation management;
- expression automation;
- vibrato automation;
- humanization systems;
- orchestral templates;
- networking;
- multi-port infrastructure;
- heavy graphics.

Ближайшая задача остаётся узкой:

> Надёжно получить реальный Chord / Key context из DAW через ARA, синхронизировать его с основным Instrument и подготовить нейтральный Harmonic Context для MIDI engine.

---

## 24. Философия разработки

Smart Voicing не должен пытаться автоматически создать полностью законченную аранжировку.

Его задача — быстро создавать музыкально полезный и редактируемый skeleton.

Целевой процесс после генерации:

```text
1. DAW Chord / Key context
2. Melody or chord input
3. Smart Voicing generation
4. Separate MIDI voices
5. Manual note editing
6. Articulations
7. Expression / vibrato
8. Final orchestration
```

Финальное музыкальное решение остаётся за музыкантом.

---

## 25. Ближайшая задача

После подтверждения bridge и live update Chord Track следующий шаг:

> **Прочитать реальные значения `Sheet Chords` и `Key Signatures`, сохранить их вместе с позициями на таймлайне, передать через shared context и отображать текущие Chord / Key в основном Smart Voicing.**

Дополнительно нужно отдельно подтвердить live update Key Track.

Полноценный voicing engine начинается только после завершения этой основы.

# Smart Voicing — основной концепт и архитектура

**Project:** Smart Voicing  
**Vendor:** Moon River Studio  
**Status:** Active Development  
**Stable checkpoint:** 0.4  
**Current stage:** Stage 5 — Jazz Voicing Engine  
**Current working line:** 0.4a… → 0.5  
**Reference host:** Studio Pro  
**Repository:** `vladleng/Smart-Voicing`

Этот файл — главный концептуальный документ проекта. GitHub Issues, PR, тестовые документы и код являются источником фактического состояния реализации; здесь хранится целостная музыкальная и архитектурная модель Smart Voicing.

---

## 1. Product Vision

Smart Voicing — не просто MIDI splitter и не генератор «трёх нот под мелодией». Цель проекта — создать **контекстно-зависимый виртуальный аранжировщик**, который использует гармоническую структуру DAW и создаёт четыре независимые музыкальные партии, пригодные для дальнейшего редактирования.

Базовая идея:

```text
Played Material
    +
Current Chord
    +
Current Key
    +
Harmonic Function
    +
Melodic Context
    +
Tension Policy
    +
Voicing Strategy
    +
Previous Voice State
    +
Instrument / Ensemble Profile
        ↓
4 independent musical voices
```

Главный критерий качества:

> Не «насколько эффектно плагин сыграл сам», а насколько хорош получившийся MIDI-материал как основа для настоящей аранжировки.

---

## 2. Источник гармонической истины

DAW остаётся главным источником гармонического контекста.

Приоритет данных:

```text
1. Played / Melody
2. Explicit Current Chord
3. Chord identity / characteristic tones
4. Current Key
5. Harmonic Function / real Resolution Target
6. Functional Tension Profile / Tension Level
7. Melodic Role / Tension Policy
8. Voicing Strategy
9. Voice Leading
10. Instrument / Ensemble Profile
```

Ключевой принцип:

> Key и Function интерпретируют Chord, но не переписывают его. Сыгранная melody также не «исправляется» теоретическим движком.

Пример:

```text
Key: C major
Chord: A7
```

Smart Voicing должен использовать C#, потому что локальный Chord имеет приоритет над общей диатоникой Key.

---

## 3. Подтверждённая архитектура ARA + Instrument

ARA-доступ к Chord/Key уже экспериментально подтверждён в Studio Pro.

```text
Studio Pro
Chord / Key / Tempo / Time Signature
        ↓ ARA
Smart Voicing ARA
        ↓ shared harmonic context + transport
Smart Voicing Instrument
        ↓
Harmony Core
        ↓
VoiceOutput[4]
        ↓
Router / Voice Stack / Sustain
        ↓
MIDI Ch1 / Ch2 / Ch3 / Ch4
```

### Smart Voicing ARA

- служебный ARA/Event FX reader;
- получает Musical Context проекта;
- читает Chord, Key, Tempo, Time Signature;
- публикует harmonic context и transport;
- не содержит основной voicing logic.

### Smart Voicing Instrument

- принимает MIDI;
- читает harmonic context через shared bridge;
- содержит Harmony Core;
- выдаёт четыре независимых Voice;
- поддерживает Direct Router и Melody Harmonize.

Harmony Core должен оставаться host-neutral. ARA — provider, а не фундамент музыкальной логики.

---

## 4. Подтверждённая база к 0.4

К стабильной версии 0.3 подтверждены:

- Chord Track и Key Track через ARA;
- Tempo / Time Signature / transport;
- live context updates;
- shared bridge ARA → Instrument;
- MIDI output Ch1–Ch4;
- downstream routing на четыре инструмента;
- CC / Pitch Bend passthrough;
- Direct Router;
- Voice Stack и stable ownership;
- sustain-aware routing и Sustain Chord Morph;
- Top Down / Bottom Up / Fill 4;
- Gesture Classifier;
- Chord Model: root, bass, quality, extensions, alterations, slash bass;
- Melody Harmonize;
- melody как авторитетный V1;
- live reharmonization удержанной melody на сменах Chord Track;
- sample-accurate chord boundaries без plugin lookahead/latency;
- `(no chord)` → V1 only;
- сохранение Harmony Mode / Distribution Mode в project state.

К стабильной версии 0.4 дополнительно подтверждены:

- `KeyModel` и scale-degree analysis;
- `Tonic / Predominant / Dominant / Other`, `Diatonic / Chromatic`;
- applied/secondary dominant candidate vs confirmed resolution;
- real next Chord Track event как единственный target evidence для target-aware profile;
- modal-interchange candidate MVP;
- candidate-based `Closed Voicing` с guide-tone priority и soft Upper Voice Spacing;
- characteristic-tone protection (`m7b5 b5`, augmented `#5`, sus identity, explicit altered fifth);
- `TensionPolicy` и Harmonic Candidate Pool;
- UI/state `Clean / Color / Rich`;
- `FunctionalTensionProfile`: Neutral / Dominant Unresolved / Dominant→Major / Dominant→Minor;
- target-aware Color и functionally intensified Rich;
- no next chord → unresolved, без inferred future target;
- explicit Chord Track material выше inference;
- Studio Pro musical acceptance major/minor II–V–I и `A7` vs `A7→Dm`;
- Windows CI 0.3f #302 — success.

---

## 5. Основные режимы

### 5.1 Direct Router

Пользователь играет готовый материал, Smart Voicing распределяет его по Voice.

```text
Top Down
Bottom Up
Fill 4
```

Fill 4:

```text
1 нота → V1=V2=V3=V4
2 ноты → V1=V2=верхняя, V3=V4=нижняя
3 ноты → V1=верхняя, V2=средняя, V3=V4=нижняя
4 ноты → по одной ноте на Voice
```

### 5.2 Melody Harmonize

Пользователь играет одну melody note/line. Плагин получает Chord + Key + Function и достраивает V2–V4.

Начиная с 0.3b:

```text
Melody Harmonize = Closed Voicing
```

Отдельный `Voicing Type` в 0.3b ещё не нужен: Closed является текущим встроенным способом гармонизации.

### 5.3 Chord Voicing / Transform — future

Пользователь играет полный chord. Сыгранный chord остаётся авторитетным material, а Smart Voicing revoice-ит/распределяет его по выбранной strategy и profile.

### 5.4 Riff Follow — future

Для performance-oriented profiles:

```text
Riff Transpose
Riff Adapt
```

- Transpose сохраняет интервальную shape;
- Adapt преобразует degrees/intervals по quality/function текущего chord.

---

## 6. Harmonic Interpretation

Stage 4 вводит интерпретацию Chord внутри Key.

Базовые категории:

```text
Tonic
Predominant
Dominant
Other
```

Дополнительно:

```text
Diatonic
Chromatic
Applied Dominant Candidate
```

Важно: `candidate` не равен подтверждённой функции. Для chromatic dominant необходим контекст разрешения.

Пример:

```text
Key: C major
Chord: D7

root degree: II
root family: Predominant
effective function: Dominant
relation: Chromatic
candidate: V/V
```

К 0.4 resolution-aware interpretation реализована. Важный final contract Stage 4:

```text
No next chord
→ Dominant / unresolved
→ no assumed resolution target

Real next chord on expected target root
→ confirmed target root + target quality
→ target-aware Functional Tension Profile
```

Smart Voicing интерпретирует реальную progression из Chord Track и не додумывает будущий target.

---

## 7. Tension Policy

Tensions — не команда «автоматически добавить 9/11/13». К 0.4 Stage 4 использует два связанных слоя:

```text
Functional Tension Profile
→ смысл harmonic colour в данном реальном обороте

Tension Level
→ интенсивность: Clean / Color / Rich
```

Классификация tone policy:

```text
Explicit
Available
Preferred
Contextual
Avoid-as-harmony
Melody-imposed
```

### 7.1 Melodic tension ≠ Harmonic tension

Важный принцип из *Modern Jazz Voicings*:

```text
melodic tension != harmonic tension
```

Если melody = 9, это не означает, что эту pitch class нужно дублировать внутри V2–V4.

### 7.2 Candidate Pool / Chord Scale

Будущая база кандидатов:

```text
Chord tones
+ explicit tensions
+ tonal / modal context
+ harmonic function
        ↓
Harmonic Candidate Pool
```

Explicit tension из Chord Track имеет приоритет над inferred policy.

### 7.3 Avoid note

`Avoid` не означает абсолютный запрет. Такая нота может быть нормальной melodic passing/approach note, но не должна автоматически использоваться как устойчивая harmonic tension.

### 7.4 Minor ninth

Minor ninth внутри сфокусированного voicing получает сильный negative weight, но не hard ban.

Контекстные исключения:

- dominant b9;
- dominant b13;
- modal harmony;
- intentionally dissonant voicing.

### 7.5 Базовая heuristic

Как стартовая policy, а не закон:

```text
whole-step above chord tone → generally available
half-step above chord tone  → generally unavailable
```

Function/mode/chord-symbol exceptions имеют больший приоритет.

### 7.6 Tension Level — accepted 0.4 contract

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour для реального target

Rich
→ functionally intensified tension / altered colour
```

Для confirmed `V→minor` `b13` может быть естественной Color-краской, а `b9/#9/#11` — более напряжёнными Rich candidates. Natural 13 не считается автоматически правильной только потому, что chord dominant.

Explicit `E13`, `E7b9`, `E7#5`, `E7b5` остаются authoritative.

Если следующего chord event нет, target не угадывается.

---

## 8. Closed Voicing — первый базовый strategy

0.3b превращает ранний `CloseVoicingHarmonizer` в настоящий candidate-based Closed Engine.

```text
Melody V1
    ↓
Chord + Key + Function + Tension Context
    ↓
Harmonic Candidate Pool
    ↓
select V2 / V3 / V4
    ↓
Full Vertical Evaluation
```

Учитываются:

- melody authority;
- guide tones;
- harmonic identity;
- root/fifth omission;
- explicit / melody-imposed tensions;
- compactness;
- Upper Voice Spacing Policy;
- slash bass;
- later: Voice Leading;
- later: Instrument Profile.

Closed — не «ближайшие chord tones вниз», а оценка полной вертикали.

---

## 9. Guide Tones и omissions

Для seventh/extended harmony 3 и 7 имеют высокий структурный вес.

Пример:

```text
Chord: G7
Melody: A

V1 A = 9
V2 F = b7
V3 B = 3
V4 D = 5
```

Root может отсутствовать, если harmonic identity уже читается. Ordinary perfect fifth часто легче всего уступает место более важной ноте.

Но `fifth expendable` не является универсальным правилом. Characteristic tones защищаются:

```text
m7b5: b5
augmented: #5
sus2 / sus4 identity tone
explicit altered fifth
```

Для simple triads root получает дополнительный вес, чтобы не потерять идентичность harmony.

---

## 10. Upper Voice Spacing Policy

Терция под melody — предпочтение, не правило.

```text
3rd  → preferred
4th  → common / acceptable
2nd  → contextual
5th+ → contextual / open
```

Нельзя форсировать preferred spacing ценой:

- неверного chord/function;
- потери guide tone;
- конфликтной tension;
- плохого Voice Leading;
- неподходящего instrument range/register.

Эта policy прежде всего относится к melody-led Closed family. Другие strategies имеют собственную spacing objective.

---

## 11. Voicing Strategy architecture

После изучения *Modern Jazz Voicings* архитектура уточняется: **не все voicing types должны быть transformations Closed**.

### 11.1 Closed / Drop family

```text
Harmonic Candidate Pool
        ↓
Closed
        ├ Closed
        ├ Drop 2
        ├ Drop 3
        └ Drop 2+4
```

Drop-family действительно естественно строить как transformation уже выбранного close-family harmonic material.

### 11.2 Spread — отдельная strategy

Spread не является просто octave-expanded Closed.

Ключевая логика:

- строится снизу вверх;
- bottom/root — структурный ориентир;
- inner voices должны хорошо представлять 3/7;
- верх добавляет support/chord tone/tension;
- spacing и balance являются целью самой strategy.

### 11.3 Quartal strategy

- предпочитает adjacent perfect/augmented fourths;
- major third между верхними Voice допустима, если сохраняется узнаваемый quartal sound;
- incomplete voicing допустим при ясной harmonic identity;
- minor ninth получает сильный penalty;
- tonal/modal context влияет на candidate pool.

### 11.4 Cluster strategy

Цель — не Closed compactness, а **density**:

- adjacent seconds preferred;
- non-second intervals уменьшают density;
- third между top voices допустима ради melody clarity;
- incomplete harmony допустима, если chord sound остаётся читаемым.

### 11.5 Upper Structure Triad strategy

Four-part UST:

```text
upper-structure triad
+
one supporting voice
```

Support выбирается из chord tone/tension подходящей chord scale. Strategy должна учитывать color и наличие отдельного bass/root support.

### 11.6 Итоговая модель

```text
Chord + Key + Function + Real Target
                ↓
      Functional Tension Profile
                ↓
        Tension Policy + Level
                ↓
        Harmonic Candidate Pool
                ↓
          VoicingStrategy
      ┌─────────┼──────────────┐
      ↓         ↓              ↓
Closed Family  Spread      Modern Strategies
      ↓                       ├ Quartal
Closed                        ├ Cluster
Drop 2                        └ Upper Structure Triad
Drop 3
Drop 2+4
                ↓
          Voice Leading
                ↓
        Instrument Profile
```

---

## 12. Melodic Context Engine — future arranger layer

Настоящий arranger должен понимать не только текущую вертикаль, но и роль melody note во времени.

Нужно различать:

```text
Chord tone
Tension
Diatonic passing tone
Chromatic passing tone
Neighbor / auxiliary
Chromatic approach
Scale approach
Double chromatic approach
Indirect resolution
Suspension
Enclosure
Outside note
```

Будущий pipeline:

```text
Melody stream
    ↓
MelodicRoleClassifier
    ↓
Target note / target voicing
    ↓
ApproachNotePolicy
    ├ Chromatic reharmonization
    ├ Diatonic reharmonization
    ├ Parallel reharmonization
    ├ Dominant reharmonization
    └ Independent lead
    ↓
Voicing Strategy + Voice Leading
```

Особенно важный принцип: non-chord melody note не обязана каждый раз создавать новый самостоятельный chord.

### Realtime vs Clip analysis

Для live MIDI будущая target note неизвестна. Поэтому архитектурно разделить:

```text
Realtime Melodic Policy
Clip / Offline Melodic Analysis
```

Не вводить mandatory lookahead/latency в live path ради полной классификации approach notes.

---

## 13. Voice Leading

Voice Leading — отдельный слой после harmonic/voicing decision.

Он должен сохранять Voice identity и оценивать не только абсолютное движение.

Базовые terms:

```text
movement
common-tone reward
stepwise reward
voice-crossing penalty
large-leap penalty
range/register penalty
guide-tone/tendency-tone continuity
strategy-specific objective
```

Примеры strategy-specific weights:

```text
Closed  → compactness + melody support
Spread  → balance + guide-tone continuity
Quartal → characteristic fourth structure
Cluster → density
UST     → upper-triad integrity + support voice
```

`Upper Voice Spacing Policy` — soft term, а не генератор интервала.

---

## 14. Instrument / Ensemble Profiles

Harmony Core не должен hardcode-ить «Voice 1 = Trumpet».

Первый практический ensemble:

```text
V1 → Trumpet
V2 → Tenor Sax
V3 → Trombone
V4 → Baritone Sax
```

Но это должен быть **Brass Ensemble Profile**, а не логика Harmony Core.

### InstrumentProfile должен знать больше, чем min/max

```text
InstrumentProfile
├ absoluteRange
├ practicalRange
├ comfortableRange
├ preferredRegister
└ registerZones[]
    ├ min/max
    ├ preferenceWeight
    ├ timbralCharacter metadata
    ├ dynamicSuitability metadata
    └ balanceWeight
```

Следовательно:

```text
playable(note) != desirable(note)
```

Voicing/Voice Leading должны использовать weighted scoring, а не только binary range check.

Будущие профили:

```text
Brass
Strings
Bass
Woodwinds
Custom Ensemble
```

---

## 15. Performance Architecture

Верхние уровни управления должны использовать один unified state:

```text
Instrument Type / Profile
Harmony Mode
Distribution Mode
Voicing Type
```

Предварительная Brass keyswitch map:

```text
Modes
C0   Direct Router
C#0  Melody Harmonize
D0   Chord Voicing
D#0  Riff Follow

Direct Distribution
E0   Top Down
F0   Bottom Up
F#0  Fill 4
G0   Smart / Auto [future]

Voicing
C1   Closed
C#1  Drop 2
D1   Open
D#1  Spread
E1   Guide Tones
F1   Block / Soli
F#1  Auto [future]
```

Правила:

- keyswitch и UI меняют один internal state;
- parallel state system не создавать;
- keyswitch notes swallowed;
- map локален для Instrument Profile;
- точные note numbers/octave labels фиксируются на implementation stage;
- keyswitch layer добавляется только после реализации самих Modes/Distribution/Voicing.

---

## 16. MIDI Output и editable workflow

```text
Voice 1 → MIDI Channel 1
Voice 2 → MIDI Channel 2
Voice 3 → MIDI Channel 3
Voice 4 → MIDI Channel 4
```

Результат должен быть записываемым на отдельные downstream MIDI tracks.

Типовой workflow:

```text
1. Chord Track / Key Track
2. Melody / Chord / Riff input
3. Smart Voicing
4. Four independent MIDI voices
5. Record / split
6. Manual editing
7. Articulations
8. Expression / vibrato
9. Final orchestration
```

---

## 17. Musical knowledge base

Smart Voicing должен опираться на формализованную аранжировочную базу, а не только на локальные эвристики проекта.

Project Sources могут содержать:

- arranging books;
- voicing literature;
- scores;
- course materials;
- user arranging notes.

GitHub хранит уже переработанные правила:

```text
Literature / Scores
      ↓ analysis
Musical Policies / Strategies
      ↓
Issues / Tests / Algorithms
      ↓
Code
```

Текущий основной reference:

**Ted Pease / Ken Pullig — Modern Jazz Voicings: Arranging for Small and Medium Ensembles**

Из книги уже формализованы:

- melodic vs harmonic tensions;
- chord-scale/candidate-pool thinking;
- avoid-note semantics;
- minor-ninth penalty with exceptions;
- approach-note harmonization;
- Closed/Drop family;
- Spread as independent bottom-up strategy;
- Quartal strategy;
- Cluster strategy;
- Upper Structure Triads;
- register/timbre-aware instrument profiles.

Практический reference для Closed:

**Closed Voicing, pt. 1 - Big Band Arranging SECRETS REVEALED**

---

## 18. Текущий roadmap

### Stage 0 — Project Skeleton ✅

VST3 project, build infrastructure, vendor/package conventions.

### Stage 1 — ARA Context ✅ → 0.1

Chord / Key / Tempo / Time Signature / transport / shared bridge.

### Stage 2 — MIDI Router ✅ → 0.2

Four Voice, Ch1–Ch4, Voice Stack, Sustain, Distribution, Gesture Classifier.

### Stage 3 — Chord-aware Harmonizer ✅ → 0.3

HarmonicContext, ChordModel, Melody Harmonize, basic Close, live reharmonization, state integration.

### Stage 4 — Key-aware Engine ✅ → 0.4

Завершён 2026-09-22. Основные итерации:

- **0.3a** — KeyModel + Harmonic Function MVP;
- **0.3b** — candidate-based Closed Voicing;
- **0.3c** — resolution-aware function, applied dominant confirmation, modal-interchange candidate, exact realtime chord boundaries;
- **0.3d** — TensionPolicy + Harmonic Candidate Pool + Clean/Color/Rich;
- **0.3e** — Functional Tension Profiles + characteristic-tone protection;
- **0.3f** — target-aware Color, no inferred future target, confirmed `V→minor` colour semantics;
- **0.4** — stable Stage 4 checkpoint.

Final Stage 4 contract:

```text
Played Melody
> Explicit Chord Track
> Chord identity / characteristic tones
> Key
> Function + REAL next-chord target evidence
> Functional Tension Profile
> Tension Level / Tension Policy
> Voicing Strategy
```

Stage 4 отвечает на вопрос: **какие pitch classes функционально оправданы в реально записанном harmonic turn?**

### Stage 5 — Jazz Voicing Engine → 0.5

- `VoicingStrategy` contract;
- Closed as first/default type;
- Drop 2 / Drop 3 / Drop 2+4 as Closed-family transformations;
- Spread as independent bottom-up strategy;
- architecture/test contract for Quartal / Cluster / UST;
- Unison / Octave / simple doubling policies where useful.

### Stage 6 — Voice Leading → 0.6

- previous Voice state;
- common-tone retention;
- stepwise movement;
- Voice identity;
- crossing/leap penalties;
- strategy-specific objectives;
- register-aware scoring hooks.

### Stage 7 — Instrument Profiles → 0.7

- Brass Profile;
- absolute/practical/comfortable ranges;
- register zones;
- balance/timbre scoring;
- octave displacement.

### Stage 8 — Presets / Performance Configurations → 0.8

Unified Mode / Distribution / Voicing state and saved performance configs.

### Stage 9 — Host Compatibility / Fallback → 0.9

Capability-based providers and host-neutral fallback paths.

### Stage 10 — Recording / Capture → 0.10

Workflow for committing/generated voices into editable DAW MIDI.

### Stage 11 — Stability / Panic / Final UI → 1.0

Optimization, Panic, hardening, final UI, full regression.

### Future arranger layers

- Melodic Context Engine: approach/passing notes and Independent Lead;
- expanded Instrument/Performance Profiles;
- Riff Follow;
- deeper DAW workflow integration.

---

## 19. Главный принцип архитектуры музыкального движка

Не создавать один гигантский `buildVoicing()` с сотнями исключений.

Разделять принятие решений:

```text
Harmonic Context
      ↓
Harmonic Function
      ↓
Melodic Role / Tension Policy
      ↓
Harmonic Candidate Pool
      ↓
Voicing Strategy
      ↓
Voice Leading
      ↓
Instrument / Ensemble Profile
      ↓
Performance / Routing Layer
```

Каждый слой должен:

- быть тестируемым;
- иметь понятный контракт;
- не переписывать более авторитетные данные предыдущего слоя;
- быть host-neutral там, где это возможно;
- не требовать mutex/file I/O/allocation в realtime path;
- допускать новые musical strategies без дублирования Harmony Core.

---

## 20. Итоговая формулировка Smart Voicing

Smart Voicing — **rule-based arranger engine**, в котором Chord, Key, Function, Melody, Tensions, Voicing Strategy, Voice Leading и Instrument Profile являются отдельными слоями музыкального решения.

Итоговая цепочка:

```text
DAW Harmonic Context
        +
Played Material
        ↓
Musical Interpretation
        ↓
Harmonic Candidate Pool
        ↓
Voicing Strategy
        ↓
Voice Leading
        ↓
Instrument / Ensemble Adaptation
        ↓
4 editable musical parts
```

Это и есть долгосрочная цель проекта: не заменить аранжировщика, а резко ускорить создание музыкально осмысленного четырёхголосного материала, который аранжировщик затем развивает вручную.

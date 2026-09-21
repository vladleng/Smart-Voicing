# Smart Voicing — направление Voice Leading

**Status:** Accepted design direction  
**Related:** #9, #10, #23, #25  
**Observed reference progression:** `Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7`

Этот документ фиксирует вывод после сравнения раннего Closed Engine и tension-aware 0.3d на одной и той же гармонической последовательности.

## 1. Основной принцип

Для базового Smart Voicing правильная цель — не максимальное движение всей секции за melody и не максимальная неподвижность любой ценой.

Цель:

```text
minimum necessary motion
```

То есть каждый Voice должен сохранять собственную линию и двигаться только настолько, насколько это нужно для:

- сохранения chord/function;
- сохранения guide tones / harmonic identity;
- корректного Voicing Strategy;
- хорошего spacing;
- естественного разрешения tendency tones;
- отсутствия voice crossing и неоправданных leaps.

## 2. Что считать предпочтительным движением

При прочих равных Voice Leading должен предпочитать:

```text
common tone / same pitch
        ↓
stepwise motion (semitone / whole tone)
        ↓
small interval
        ↓
large leap only when musically justified
```

`Oblique motion` — когда melody движется, а один или несколько внутренних голосов сохраняют общие ноты — является нормальным и часто предпочтительным поведением.

Если на `A7` внутренние Voices уже содержат сильные structural tones, например 3 и b7, они не должны автоматически двигаться вслед за каждой нотой melody.

## 3. Parallel / Block / Soli — отдельная музыкальная цель

Раннее поведение, где вся гармония движется почти параллельно melody, музыкально корректно, но это не должно быть универсальным default-поведением.

Такой тип движения относится к отдельной стратегии/политике:

```text
Parallel / Block / Soli harmonization
```

и особенно уместен для:

- sax/brass soli;
- chromatic/diatonic approach-note reharmonization;
- специальных melodic-context rules.

Связь: #23 `Melodic Context Engine` уже содержит `Parallel reharmonization` и `Independent lead` как разные способы обработки melody.

## 4. Default Voice Leading vs Soli

### Default Smart Voice Leading

```text
Melody changes
    ↓
keep common tones where useful
keep guide tones where useful
move remaining voices by the smallest musically valid distance
```

### Parallel / Soli behavior

```text
Melody changes
    ↓
all/most lower voices follow the lead according to the chosen reharmonization rule
```

Эти модели нельзя смешивать в один универсальный scoring rule.

## 5. Важное ограничение текущей 0.3d

0.3d уже может давать более устойчивые/ostinato-like внутренние Voices, потому что расширенный Harmonic Candidate Pool и Closed scoring выбирают другую вертикаль.

Но это **ещё не настоящий Voice Leading**.

Текущий Closed Engine в первую очередь отвечает на вопрос:

```text
какой лучший vertical voicing для текущего Chord + Melody?
```

Stage 6 должен добавить вопрос:

```text
какой лучший следующий voicing,
учитывая предыдущие V1–V4 и identity каждого Voice?
```

Поэтому удачную статичность 0.3d нужно считать хорошим направлением, но не доказательством уже реализованного Voice Leading.

## 6. Cost function для Stage 6

Минимальный accepted contract:

- strong reward: common tone остаётся в том же Voice slot;
- reward: semitone/whole-tone movement;
- lower reward: небольшой интервал;
- penalty: неоправданный leap;
- strong penalty: voice crossing;
- preserve: melody V1 authority;
- preserve: Chord/Function и Voicing Strategy;
- preserve: guide tones / harmonic identity;
- Tension Level регулирует candidate pool, но не делает «больше tensions» самостоятельной целью.

Главная формула поведения:

```text
not maximum motion
not minimum motion at any cost

→ minimum musically necessary motion
```

## 7. Связь с Tension Level

Tension Level должен помогать Voice Leading, а не конкурировать с ним.

```text
Level 1 — Clean
предпочитает structural chord tones

Level 2 — Color
может выбрать tension, если она улучшает vertical + continuity

Level 3 — Rich
расширяет contextual/altered pool,
особенно когда color даёт естественное tendency-tone resolution
```

После Stage 6 решение `Color/Rich` должно учитывать `previous Voice state`: common tones, smooth steps и resolution path.

## 8. Regression progression

Последовательность

```text
Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7
```

зафиксировать как один из музыкальных regression-тестов Voice Leading.

Она полезна потому что содержит:

- обычные minor-7 chords;
- chromatic root motion;
- explicit `b13`;
- explicit `#11`;
- maj7;
- dominant;
- участки, где можно сравнивать parallel motion и oblique/common-tone motion.

Особенно внимательно анализировать `Bbmaj7 → A7` и движение внутри `A7`: default Voice Leading не должен пересобирать V2–V4 вслед за каждой melody note, если текущие structural/common tones могут быть музыкально сохранены.

## 9. Roadmap boundary

- **0.3d / Stage 4:** Tension Policy, Tension Level, Harmonic Candidate Pool, статический Closed scoring.
- **Stage 5:** разные `VoicingStrategy` и их vertical objectives.
- **Stage 6:** полноценный previous-state-aware Voice Leading по этому документу.
- **#23 / later:** melodic-role-dependent Parallel / Diatonic / Chromatic / Independent Lead behavior.

Таким образом, более статичный внутренний голос сам по себе не является целью. Цель — независимые музыкальные линии с минимально необходимым движением и возможностью отдельно включать Soli/Parallel-поведение там, где оно действительно требуется аранжировкой.

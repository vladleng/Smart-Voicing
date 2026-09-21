# Smart Voicing 0.3d — Tension Policy + Harmonic Candidate Pool

Статус: **IN DEVELOPMENT — live Closed integration started**.

Цель версии: добавить отдельный музыкальный слой между `Chord + Key + Function` и `VoicingStrategy`, который классифицирует возможные гармонические краски и не смешивает их с performer-owned melody.

---

## 1. Архитектура

```text
Played Melody
      >
Explicit Chord Track
      >
Key / Mode
      >
Harmonic Function / Resolution
      ↓
Tension Policy
      ↓
Harmonic Candidate Pool
      ↓
Closed Voicing Strategy
```

Tension Policy не имеет права переписывать explicit Chord Track или "исправлять" сыгранную melody.

---

## 2. Классы 0.3d

Для каждого pitch class относительно chord root хранится роль:

```text
Chord Tone
Explicit
Preferred
Available
Contextual
Avoid-as-harmony
Unavailable
```

Отдельный независимый флаг:

```text
Melody-imposed
```

`Melody-imposed` означает, что performer сыграл эту ноту как V1. Даже `Avoid-as-harmony` или `Unavailable` не запрещают такую melody; классификация ограничивает только generated lower harmony.

---

## 3. Приоритет explicit tensions

Если Chord Track явно содержит:

```text
9 / b9 / #9
11 / #11
13 / b13
```

эта нота получает роль `Explicit` и имеет приоритет над inferred Key/Function policy.

Контрольные случаи:

```text
Cmaj7#11 in C major
F# = Explicit
```

```text
C7b9
Db = Explicit
```

Общее правило avoid-note не должно понижать явно записанную tension.

---

## 4. Baseline availability rules

Основано на принятом reference `Modern Jazz Voicings` (#24):

```text
whole-step above chord tone
→ generally available

half-step above chord tone
→ generally avoid-as-harmony
```

Это стартовая policy, а не абсолютный запрет.

Пример:

```text
Cmaj7 in C major
D = Preferred 9
F = Avoid-as-harmony 11
A = Preferred 13
```

Если melody = F:

```text
F remains V1
F = Avoid-as-harmony + Melody-imposed
```

---

## 5. Dominant baseline

Для dominant-quality / dominant-function harmony 0.3d начинает с консервативной Mixolydian candidate collection, если explicit Chord Track не говорит иное.

Это нужно прежде всего для secondary/applied dominant:

```text
C major
D7 -> G
```

Нельзя слепо наследовать весь C-major pitch set и тем самым считать F-natural нормальной гармонической краской над explicit F# в D7.

Ожидание:

```text
D7:
E  = Preferred 9
G  = Avoid-as-harmony 11 above F#
B  = Preferred 13
F  = Unavailable
F# = Chord Tone
```

---

## 6. Modal Interchange

Если 0.3c выдал `modalInterchangeCandidate`, inferred candidate collection использует parallel source mode, а не насильно active major/minor collection.

Пример:

```text
C major + Fm7
source = parallel C minor
```

---

## 7. Реализованный live slice

`ClosedVoicingContext` теперь содержит `TensionPolicy`.

Если live caller передал только `Key + HarmonicAnalysis`, `buildClosedVoicing()` сам строит policy для текущей melody без дополнительной DAW-зависимости:

```text
Chord + Key + HarmonicAnalysis + Melody
                ↓
         buildTensionPolicy()
                ↓
     Closed Harmonic Candidate Pool
                ↓
       role-aware candidate scoring
```

Generated V2–V4 теперь могут использовать:

```text
Explicit
Preferred
Available
Contextual
```

`Avoid-as-harmony` и `Unavailable` в generated lower harmony не допускаются.

Scoring 0.3d:

- structural guide tones 3/7 остаются приоритетными;
- `Explicit` получает сильный положительный приоритет;
- `Preferred` получает небольшой reward;
- `Available` допускается с небольшим penalty;
- `Contextual` допускается более осторожно;
- plain triads остаются chord-tone-only;
- fifth остаётся первым кандидатом на omission;
- root omission сохраняется для seventh/extended harmony;
- minor ninth получает отрицательный вес, а не hard ban;
- explicit altered tension может bypass generic minor-ninth penalty.

---

## 8. Host-neutral regressions

`SmartVoicingTensionPolicyTests` проверяет policy-классификацию.

`SmartVoicingHarmonizerTests` дополнительно проверяет уже музыкальное применение policy:

```text
Cmaj7 / C major + melody G
→ G-E-D-B
```

То есть Preferred 9 может заменить менее важный structural tone, но 3 и 7 сохраняются.

Также проверяется:

- Cmaj7 + melody F: F остаётся V1 как Melody-imposed Avoid;
- generated lower voices не дублируют avoid F;
- D7 -> G: Preferred 9 входит в Closed pool, F-natural не наследуется из C major;
- D7 -> Am сохраняет ту же conservative dominant baseline до более глубокой resolution policy;
- plain C major triad не превращается автоматически в add9/13.

---

## 9. Regression foundation

Перед началом musical integration пользователь подтвердил 0.3d foundation regression в Studio Pro:

- ARA / Chord / Key context работает;
- 0.3c resolution diagnostics не деградировали;
- Closed / Sustain / Direct Router / no-chord behavior работают;
- boundary transient notes отсутствуют при точном Note On на chord boundary.

Это считается baseline перед новым tension-aware output.

---

## 10. Следующий test slice

После green CI текущего HEAD нужно проверить в Studio Pro уже изменившийся музыкальный результат:

```text
Cmaj7 / C major
melody G  -> ожидается G-E-D-B
melody F  -> F остаётся V1, lower harmony не генерирует F

D7 -> G / C major
melody A  -> ожидается A-F#-E-C

C major triad / C major
melody G  -> остаётся G-E-C-G
```

Отдельно проверить explicit `#11` и `b9` после того, как Studio Pro Chord Track корректно передаёт их degree metadata.

---

## 11. Acceptance 0.3d

0.3d можно принять только после:

1. green `SmartVoicingTensionPolicyTests`;
2. green `SmartVoicingHarmonizerTests` с live candidate-pool logic;
3. Tension Policy реально влияет на Closed output;
4. explicit tensions из Chord Track имеют приоритет;
5. melody-imposed avoid/outside note остаётся V1;
6. context-aware minor-ninth penalty протестирован;
7. plain triads не получают inferred tensions автоматически;
8. Windows Build зелёный для финального 0.3d HEAD;
9. Studio Pro regression подтверждает 0.3b/0.3c behavior и новые tension cases;
10. UI / diagnostics синхронизированы с 0.3d перед финальным acceptance.

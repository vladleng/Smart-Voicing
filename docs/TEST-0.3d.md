# Smart Voicing 0.3d — Tension Policy + Harmonic Candidate Pool

Статус: **IN DEVELOPMENT**.

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
Voicing Strategy
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

Explicit altered tensions позже могут заменить этот baseline без конфликта.

---

## 6. Modal Interchange

Если 0.3c выдал `modalInterchangeCandidate`, inferred candidate collection использует parallel source mode, а не насильно active major/minor collection.

Пример:

```text
C major + Fm7
source = parallel C minor
```

---

## 7. Host-neutral tests

`SmartVoicingTensionPolicyTests` должен проверять минимум:

- Cmaj7 / C major: `9 preferred`, `11 avoid`, `13 preferred`;
- avoid note может быть `Melody-imposed`;
- Dm7 / C major: 9/11/13 доступны как preferred baseline;
- G7 / C major: 9 и 13 preferred, natural 11 avoid;
- D7 -> G / C major: applied dominant не наследует F-natural из global key;
- explicit `#11` и `b9` имеют приоритет над inference;
- Fm7 / C major использует parallel-minor evidence;
- при отсутствии Key/Function Smart Voicing не изобретает inferred tensions.

---

## 8. Что пока НЕ подключено

Первый slice 0.3d создаёт host-neutral `TensionPolicy` и `Harmonic Candidate Pool` + tests.

До следующего slice **не меняется live Closed output**. То есть пользовательская сборка остаётся маркированной как принятая 0.3c, пока Tension Policy не будет реально подключена в `ClosedVoicingContext` и candidate scoring.

Это сделано специально, чтобы не выдавать промежуточную архитектурную заготовку как готовую пользовательскую 0.3d.

---

## 9. Следующий slice

```text
TensionPolicy
      ↓
ClosedVoicingContext
      ↓
V2-V4 candidate generation
      ↓
role-aware scoring
```

Правила интеграции:

- structural chord tones и guide tones остаются основой chord identity;
- inferred tensions не должны вытеснять 3/7 без музыкальной причины;
- `Avoid-as-harmony` не используется в generated lower voices;
- `Explicit` имеет высокий приоритет;
- `Preferred / Available / Contextual` получают разные веса;
- minor ninth становится context-aware negative weight, а не hard ban;
- plain triads должны оставаться консервативными и не превращаться автоматически в add9/13 без достаточного scoring evidence.

---

## 10. Acceptance 0.3d

0.3d можно принять только после:

1. green host-neutral `SmartVoicingTensionPolicyTests`;
2. Tension Policy подключена в live `ClosedVoicingContext`;
3. Closed Engine умеет использовать Harmonic Candidate Pool без потери chord identity;
4. explicit tensions из Chord Track имеют приоритет;
5. melody-imposed avoid/outside note остаётся V1;
6. context-aware minor-ninth penalty протестирован;
7. Windows Build зелёный для финального 0.3d HEAD;
8. Studio Pro regression подтверждает 0.3b/0.3c behavior и новые tension cases.

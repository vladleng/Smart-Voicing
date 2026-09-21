# Smart Voicing 0.3c — Resolution-aware Harmonic Function

Цель версии: перестать считать dominant-form chord окончательно интерпретированным только по `Chord + Key` и добавить **timeline evidence** из следующего Chord Track event.

0.3c также добавляет первый безопасный `Modal Interchange Candidate` для borrowing из parallel major/minor.

---

## 1. Основная модель

```text
Current Chord + Key
        ↓
Static Harmonic Analysis
        ↓
Applied Dominant Candidate
        +
Next Chord Track event
        ↓
Resolution Evidence
        ↓
Candidate / Confirmed
```

Важно:

- `candidate` и `confirmed` — разные признаки;
- несовпадающий следующий chord **не означает**, что candidate автоматически ложный;
- возможны delayed / deceptive resolutions;
- Smart Voicing не переписывает Chord Track.

---

## 2. Host-neutral tests

### C major: D7

Static:

```text
D7
root = II
relation = Chromatic
candidate = V/V
confirmed = NO
```

D7 -> G:

```text
candidate = V/V
next root = G
confirmed = YES
```

D7 -> Am:

```text
candidate = V/V
next root = A
confirmed = NO
```

Candidate при этом сохраняется.

### C major: C7

Static:

```text
C7
candidate = V/IV
confirmed = NO
```

C7 -> F:

```text
candidate = V/IV
confirmed = YES
```

Этот тест нужен для неоднозначности:

```text
C7 = tonic dominant / blues sound ?
или
C7 = V/IV ?
```

Только фактическое разрешение в F даёт direct confirmation V/IV.

### Primary dominant

```text
C major: G7 -> C
```

Ожидание:

- Function = Dominant;
- primary V не маркируется как Applied Dominant.

```text
G major: D7
```

Ожидание:

- D7 = обычный V;
- не Applied Dominant.

---

## 3. Modal Interchange Candidate MVP

Алгоритм 0.3c пока не пытается полностью объяснять modal interchange. Он выдаёт evidence, если chromatic chord целиком помещается в parallel major/minor pitch collection.

### C major

Проверить:

```text
Fm7    -> candidate from parallel minor
Ebmaj7 -> candidate from parallel minor
```

D7 в C major не должен ошибочно становиться parallel-minor borrowing: это отдельное secondary-dominant evidence.

### C minor

Проверить:

```text
F major -> candidate from parallel major
```

---

## 4. Studio Pro diagnostics

После интеграции UI проверить Chord Track в **C major**:

```text
D7 | G7 | Cmaj7
```

На D7 diagnostics должны показывать примерно:

```text
Root degree: II
Relation: Chromatic
Applied: V/V
Resolution: CONFIRMED
Next chord: G...
```

Затем заменить G7 на Am:

```text
D7 | Am | ...
```

Ожидание:

```text
Applied: V/V candidate
Resolution: not confirmed
```

Проверить:

```text
C7 | Fmaj7
```

Ожидание:

```text
Applied: V/IV
Resolution: CONFIRMED
```

Проверить modal interchange:

```text
Fm7 | Cmaj7
Ebmaj7 | Cmaj7
```

Ожидание: diagnostics показывает candidate из parallel minor.

---

## 5. Melody Harmonize regression

Resolution-aware analysis не должен сам по себе ломать уже принятую 0.3b Closed-логику.

Проверить:

- Cmaj7 + `C D E F G A B C` сохраняет корректные compact Closed voicings;
- V1 остаётся melody;
- chord changes reharmonize только lower voices;
- Sustain работает;
- `(no chord) -> chord` работает;
- нет stuck notes;
- Direct Router не изменился.

Особенно проверить, что новая Function/Resolution information является **context/scoring evidence**, а не командой заменить явные chord tones.

---

## 6. Что сознательно НЕ входит в 0.3c

- полный chord-progression grammar;
- автоматическое распознавание всех deceptive resolutions;
- окончательная классификация blues / tonic dominant / backdoor / tritone-substitute по одному правилу;
- полный borrowed-chord taxonomy;
- melodic approach-note analysis;
- Tension Policy / chord scales — это 0.3d;
- Voice Leading — Stage 6.

---

## 7. Критерий принятия 0.3c

0.3c можно считать принятой, когда:

1. host-neutral tests проходят;
2. Windows CI зелёный для финального HEAD;
3. diagnostics в Studio Pro различает `candidate` и `confirmed` по реальному следующему Chord Track event;
4. modal-interchange MVP корректно показывает parallel-mode evidence на тестовых случаях;
5. 0.3b Closed / Sustain / Direct Router regression остаётся чистой.

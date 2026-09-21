# Smart Voicing 0.3c — Resolution-aware Harmonic Function

Цель версии: перестать считать dominant-form chord окончательно интерпретированным только по `Chord + Key` и добавить **timeline evidence** из следующего Chord Track event.

0.3c также добавляет первый безопасный `Modal Interchange Candidate` для borrowing из parallel major/minor и передаёт этот контекст в live `ClosedVoicingContext`.

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
        ↓
ClosedVoicingContext
        ↓
Closed Engine
```

Важно:

- `candidate` и `confirmed` — разные признаки;
- несовпадающий следующий chord **не означает**, что candidate автоматически ложный;
- возможны delayed / deceptive resolutions;
- Smart Voicing не переписывает Chord Track;
- Melody и explicit Chord остаются выше Key / Function / Resolution evidence;
- в 0.3c resolution evidence ещё не должна искусственно создавать разные tensions — это задача 0.3d.

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

Только фактическое разрешение в F даёт direct confirmation V/IV.

### Primary dominant

```text
C major: G7 -> C
G major: D7
```

Ожидание: primary V = Dominant, но не Applied Dominant.

---

## 3. Modal Interchange Candidate MVP

Для chromatic chord выдаётся evidence, если его pitch set помещается в parallel major/minor collection.

Контроль:

```text
C major: Fm7 / Ebmaj7 -> parallel minor
C minor: F major -> parallel major
```

D7 в C major не должен ошибочно становиться parallel-minor borrowing: это отдельное secondary-dominant evidence.

---

## 4. Studio Pro diagnostics — подтверждено

Практический тест 2026-09-21:

```text
D7 | G7 | D7 | Am7 | C7 | Fmaj7 | Fm7 | Cmaj7
```

Подтверждено:

```text
D7 -> G7    = V/V candidate + CONFIRMED
D7 -> Am7   = V/V candidate, NOT CONFIRMED
C7 -> Fmaj7 = V/IV candidate + CONFIRMED
Fm7         = modal interchange candidate: parallel minor
```

Также подтверждены C major Key Track и корректные `degree / root function / effective function / relation`.

---

## 5. Live ClosedVoicingContext integration

`Melody Harmonize` больше не использует context-free compatibility path `buildCloseVoicing()`.

```text
Current HarmonicContext
├ Chord
├ Key
└ timeline position
      +
Next Chord Track event
      ↓
HarmonicAnalysis
      ↓
ClosedVoicingContext
      ↓
buildClosedVoicing(...)
```

Closed Engine получает Key, Function, Diatonic/Chromatic relation, Applied Dominant Candidate/Confirmed и Modal Interchange Candidate.

0.3c не обязана давать разные ноты для `D7 -> G` и `D7 -> Am`: реальные различия tension selection относятся к 0.3d.

---

## 6. Финальный Studio Pro regression

Использован тест:

```text
Key: C major

D7 | G7 | D7 | Am7 | C7 | Fmaj7 | Fm7 | Cmaj7
 A | G  | A  | A   | G  | A     | Ab  | G
```

Подтверждено:

- V1 совпадает с сыгранной melody;
- explicit Chord Track остаётся авторитетным;
- `D7 + A` даёт корректный compact Closed;
- live reharmonization работает на chord boundaries;
- diagnostics сохраняет resolution/modal evidence;
- Closed 0.3b regression не нарушена.

Дополнительная регрессия 0.3b:

```text
Cmaj7 + C D E F G A B C
C -> C-B-G-E
D -> D-B-G-E
```

### Boundary regression — короткие MIDI-ноты на стыках

В первом полном Studio Pro тесте были обнаружены микроскопические generated notes на стыках. Причина: `kBoundaryTolerancePpq = 0.0001`, предназначенный для stopped cursor/UI, попадал в realtime playback path и мог открыть следующий chord на несколько samples раньше.

Начиная с `e7009da`:

```text
Transport PLAYING
→ realtime boundary = exact ARA event position + numerical epsilon only

Transport STOPPED
→ UI/cursor diagnostics keeps small PPQ tolerance
```

Финальный пользовательский тест 2026-09-21 подтвердил исправление: melody была размещена на дорожке Smart Voicing точно на границах Chord Track, затем четыре generated voices записаны на инструментальные дорожки. При совпадении Note On с chord boundary короткие transient notes отсутствуют.

Если новая melody note реально начинается раньше следующего chord event, Smart Voicing корректно использует новую melody со старым chord до фактической boundary, а затем reharmonize. Такой короткий voicing отражает реальные тайминги и не считается багом.

```text
melody Note On раньше chord boundary
→ новая melody + текущий старый chord
→ на chord boundary reharmonization

melody Note On точно на chord boundary
→ одна чистая смена voicing без transient notes
```

---

## 7. Что сознательно НЕ входит в 0.3c

- полный chord-progression grammar;
- все deceptive resolutions;
- окончательная классификация blues / tonic dominant / backdoor / tritone-substitute;
- полный borrowed-chord taxonomy;
- melodic approach-note analysis;
- Tension Policy / chord scales — 0.3d;
- Voice Leading — Stage 6.

---

## 8. Итог принятия 0.3c — ПРИНЯТО

Подтверждено 2026-09-21:

1. host-neutral tests проходят;
2. Windows Build #259 — success;
3. diagnostics в Studio Pro различает candidate/confirmed;
4. modal-interchange MVP подтверждён;
5. live Melody Harmonize использует `ClosedVoicingContext`;
6. Closed 0.3b regression сохранена;
7. realtime boundary bug исправлен;
8. при точном совпадении melody и chord boundary transient notes отсутствуют;
9. ранняя melody note корректно остаётся под старым chord до его фактической смены.

**Статус 0.3c: ACCEPTED / Studio Pro confirmed.**

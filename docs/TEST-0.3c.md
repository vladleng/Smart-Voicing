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

```text
D7 static -> V/V candidate
D7 -> G   -> V/V CONFIRMED
D7 -> Am  -> V/V candidate, not confirmed
```

### C major: C7

```text
C7 static -> V/IV candidate
C7 -> F   -> V/IV CONFIRMED
```

### Primary dominant

```text
C major: G7 -> C
G major: D7
```

Primary V = Dominant, но не Applied Dominant.

---

## 3. Modal Interchange Candidate MVP

```text
C major: Fm7 / Ebmaj7 -> parallel minor
C minor: F major -> parallel major
```

D7 в C major остаётся secondary-dominant evidence, а не parallel-minor borrowing.

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

`Melody Harmonize` использует:

```text
Chord + Key + Next Chord
        ↓
HarmonicAnalysis
        ↓
ClosedVoicingContext
        ↓
buildClosedVoicing(...)
```

Closed Engine получает Key, Function, Diatonic/Chromatic relation, Applied Dominant Candidate/Confirmed и Modal Interchange Candidate.

Различия tension selection между confirmed/unconfirmed dominant относятся к 0.3d.

---

## 6. Boundary regression

В первом полном Studio Pro тесте были обнаружены микроскопические generated notes на стыках. Причина: UI tolerance попадал в realtime playback path и мог открыть следующий chord на несколько samples раньше.

Начиная с `e7009da`:

```text
Transport PLAYING
→ realtime boundary = exact ARA event position + numerical epsilon only

Transport STOPPED
→ UI/cursor diagnostics keeps small PPQ tolerance
```

Финальный пользовательский тест 2026-09-21 подтвердил исправление: melody была размещена на дорожке Smart Voicing точно на границах Chord Track, затем generated voices записаны на инструментальные дорожки. При совпадении Note On с chord boundary короткие transient notes отсутствуют.

Если melody начинается раньше следующего chord event, короткий промежуточный voicing является корректным результатом реального тайминга:

```text
melody Note On раньше chord boundary
→ новая melody + старый chord
→ на chord boundary reharmonization

melody Note On точно на chord boundary
→ одна чистая смена voicing
```

---

## 7. Что сознательно НЕ входит в 0.3c

- полный chord-progression grammar;
- все deceptive resolutions;
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

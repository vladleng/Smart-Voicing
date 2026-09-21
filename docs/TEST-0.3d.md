# Smart Voicing 0.3d — Tension Policy + Harmonic Candidate Pool

Статус: **READY FOR STUDIO PRO MUSICAL ACCEPTANCE**.

Цель версии: добавить отдельный музыкальный слой между `Chord + Key + Function` и `VoicingStrategy`, который классифицирует возможные гармонические краски и не смешивает их с performer-owned melody.

Актуальная Windows-сборка для acceptance: **Build #295 — success**.  
Package: `Smart-Voicing-0.3d-Windows`.

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
      >
Tension Level / Tension Policy
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

## 3. Tension Level

Приняты три уровня гармонической насыщенности:

```text
Level 1 — Clean
предпочитай chord tones

Level 2 — Color
tension можно выбрать, если она даёт лучший voicing / spacing

Level 3 — Rich
contextual/altered candidates получают больше свободы,
если это поддерживается harmonic context
```

Важно:

- Level не задаёт обязательный список extensions;
- `Preferred` не означает «обязательно вставить»;
- `Explicit` и `Melody-imposed` стоят выше выбранного Level;
- полноценный выбор color по `previous Voice state` относится к Stage 6 Voice Leading.

Подробный contract: `docs/TENSION-LEVELS.md`.

---

## 4. Приоритет explicit tensions

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

## 5. Baseline availability rules

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

## 6. Dominant baseline

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

## 7. Modal Interchange

Если 0.3c выдал `modalInterchangeCandidate`, inferred candidate collection использует parallel source mode, а не насильно active major/minor collection.

Пример:

```text
C major + Fm7
source = parallel C minor
```

---

## 8. Реализованный live slice

`ClosedVoicingContext` содержит `TensionPolicy` + `TensionLevel`.

Если live caller передал Key + HarmonicAnalysis, `buildClosedVoicing()` строит policy для текущей melody без дополнительной DAW-зависимости:

```text
Chord + Key + HarmonicAnalysis + Melody + Tension Level
                ↓
         buildTensionPolicy()
                ↓
     Closed Harmonic Candidate Pool
                ↓
       role-aware candidate scoring
```

Generated V2–V4 могут использовать:

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
- `Preferred` зависит от Tension Level;
- `Available` допускается осторожнее;
- `Contextual/altered` фактически раскрывается на Rich;
- plain triads остаются chord-tone-only;
- fifth остаётся первым кандидатом на omission;
- root omission сохраняется для seventh/extended harmony;
- minor ninth получает отрицательный вес, а не hard ban;
- explicit altered tension может bypass generic minor-ninth penalty;
- без валидного Key context inferred tensions не изобретаются.

---

## 9. UI / State / Diagnostics

Реализовано:

- selector `Tension Level: 1 Clean / 2 Color / 3 Rich`;
- выбранный Level проводится в live `ClosedVoicingContext`;
- project state version 4 сохраняет Level;
- diagnostics показывают выбранный Level;
- смена Level должна применяться к текущему Melody Harmonize context.

---

## 10. Host-neutral regressions

`SmartVoicingTensionPolicyTests` проверяет policy-классификацию.

`SmartVoicingHarmonizerTests` дополнительно проверяет музыкальное применение policy, включая Level 1/2/3.

Контрольные случаи:

```text
Cmaj7 / C major + melody G
Clean → G-E-C-B
Color → G-E-D-B
```

Также проверяется:

- Cmaj7 + melody F: F остаётся V1 как Melody-imposed Avoid;
- generated lower voices не дублируют avoid F;
- D7 -> G: Preferred 9 входит в Color pool, F-natural не наследуется из C major;
- D7 -> Am сохраняет conservative dominant baseline;
- plain C major triad не превращается автоматически в add9/13;
- Rich допускает altered dominant candidates при валидном context;
- legacy chord-only path не изобретает inferred tensions без Key.

Windows Build #295 прошёл:

- Configure — success;
- Build — success;
- Test Harmony Core — success;
- Prepare versioned package — success;
- Upload VST3 package — success.

---

## 11. Studio Pro musical acceptance — следующий тест

Проверять **один и тот же материал на Level 1 / 2 / 3** и записывать generated MIDI, чтобы сравнивать вертикаль визуально и на слух.

### A. Diatonic II–V–I

Например:

```text
Dm7 | G7 | Cmaj7
```

Ожидание:

- Clean — наиболее structural;
- Color — умеренно использует 9/13, если они улучшают vertical;
- Rich — богаче, но не теряет 3/7 и функцию.

### B. Secondary dominant

```text
C major
D7 | G7 | Cmaj7
```

Проверить, что D7 не получает F-natural как обычный inferred color.

### C. Explicit altered dominant

Проверить Chord Track symbols с `b9`, `#9`, `#11`, `b13`.

Ключевой критерий:

```text
Explicit Chord Track > Tension Level
```

То есть explicit alteration не должна исчезать на Clean.

### D. Maj7 / avoid-note case

```text
Cmaj7 in C major
```

Проверить melody `D / F / A`:

- D/A могут работать как color;
- F остаётся performer-owned V1, но не должен автоматически появляться в V2–V4 как stable harmony.

### E. Modal interchange

Например:

```text
C major | Fm7 | Cmaj7
```

Проверить, что candidate pool для Fm7 не насильно выводится только из C major.

### F. Live Level switching

При удержанной melody переключить:

```text
Clean → Color → Rich → Clean
```

Проверить:

- V1 не меняется;
- V2–V4 перестраиваются корректно;
- нет stuck notes;
- нет лишних transient notes.

### G. Save / reopen

- выбрать Rich;
- сохранить проект;
- закрыть / открыть;
- убедиться, что Rich восстановился.

### H. Key Track / modulation

Проверить смену Key при работающем Melody Harmonize и убедиться, что новый context применяется без stuck/transient notes.

---

## 12. Voice Leading — НЕ критерий 0.3d

Сравнение раннего Closed и tension-aware 0.3d показало полезный эффект: внутренние Voices иногда становятся более устойчивыми, пока melody движется.

Это хорошее направление, но 0.3d **ещё не использует previous Voice state как полноценную cost function**.

Accepted future direction:

```text
not maximum parallel motion
not maximum static motion

→ minimum musically necessary motion
```

`Parallel / Block / Soli` должен быть отдельной musical policy/strategy, а не default-поведением.

Документ: `docs/VOICE-LEADING-DIRECTION.md`.  
Stage 6 task: #10.  
Melodic-context parallel/independent behavior: #23.

Музыкальный regression reference для Stage 6:

```text
Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7
```

Особенно анализировать `Bbmaj7 → A7` и внутреннее движение на `A7`.

---

## 13. Acceptance 0.3d

0.3d можно перевести в `ACCEPTED / CLOSED`, когда выполнено:

1. [x] green `SmartVoicingTensionPolicyTests`;
2. [x] green `SmartVoicingHarmonizerTests` с candidate-pool logic;
3. [x] Tension Policy реально влияет на Closed output;
4. [x] `Tension Level` реализован в engine/UI/state/diagnostics;
5. [x] explicit tensions имеют приоритет на engine level;
6. [x] melody-imposed avoid/outside note остаётся V1;
7. [x] context-aware minor-ninth penalty реализован;
8. [x] plain triads не получают inferred tensions автоматически;
9. [x] Windows Build #295 зелёный для текущего 0.3d HEAD;
10. [ ] Studio Pro подтверждает различие Clean / Color / Rich на реальных harmonies;
11. [ ] Studio Pro подтверждает explicit `#11 / b9` metadata;
12. [ ] live Level switching без stuck/transient notes;
13. [ ] save/reopen Tension Level;
14. [ ] Key Track / modulation regression;
15. [ ] после пользовательского acceptance синхронизировать Issue #8 / #25 / PR #22 и перевести этот документ в `ACCEPTED / CLOSED`.

После этого Stage 4 проходит финальный stable checkpoint `0.4`, PR #22 закрывает Stage 4, и разработка переходит к Stage 5 / `0.4a` (#9).

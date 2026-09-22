# Smart Voicing 0.3e — Functional Tension Profiles

Статус: **IN DEVELOPMENT**.

Цель версии: после Studio Pro acceptance 0.3d сделать inferred harmonic colour зависимым не только от chord/key, но и от **реального resolution target**, а также защитить characteristic chord tones от неправильной omission policy.

---

## 1. Причина появления 0.3e

0.3d успешно подтвердил:

- Clean vs Color;
- Tension Level UI/state;
- explicit vs inferred tension semantics;
- tension-aware Closed candidate pool.

Но musical acceptance выявил два blocking issue:

```text
Bm7b5
→ b5 могла исчезнуть как будто это ordinary expendable fifth
```

и

```text
E7 -> Am
→ natural 13 C# могла появляться из generic Mixolydian,
  хотя target/minor context требует другой tension profile
```

Кроме того Rich технически допускал alterations, но не имел достаточного функционального основания предпочитать их.

---

## 2. Новая архитектура

```text
Chord + Key + Function + Next Chord
        ↓
HarmonicAnalysis
        ├ dominantResolutionConfirmed
        ├ dominantTargetPitchClass
        └ dominantTargetQuality
        ↓
Functional Tension Profile
        ├ Neutral
        ├ Dominant / unresolved
        ├ Dominant -> major target
        └ Dominant -> minor target
        ↓
Tension Level
        ↓
Closed candidate pool / scoring
```

---

## 3. Characteristic tone contract

Нельзя считать любую fifth одинаково expendable.

0.3e защищает:

```text
m7b5 b5
augmented #5
sus2 / sus4 identity tone
explicit altered fifth
```

Ordinary perfect fifth по-прежнему может уступать место более важной guide/tension note.

---

## 4. Dominant target contract

### Major target

```text
G7 -> Cmaj
```

Color может использовать natural 9/13 как inside colour.

Rich может использовать более сильный altered colour, если actual resolution подтверждён и vertical scoring это оправдывает.

### Minor target

```text
E7 -> Am
```

Generic Mixolydian natural 13 не должна автоматически становиться Preferred.

Rich получает направленные candidates:

```text
b9
b13
```

а `#9/#11` остаются более contextual.

### Unresolved

```text
D7 -> Am
```

Если Am не является ожидаемым target root D7, Smart Voicing не должен использовать quality Am как будто она подтверждает D7 dominant profile.

---

## 5. Host-neutral tests

### TensionPolicyTests

Проверить:

- neutral Cmaj7 policy не регрессирует;
- primary `G7 -> Cmaj7` получает `dominantMajorTarget`;
- `D7 -> G` получает confirmed major-target profile;
- `D7 -> Am` остаётся unresolved;
- `E7 -> Am` получает `dominantMinorTarget`;
- `E7 -> Am`: b9/b13 = functionally directed Rich candidates;
- `E7 -> Am`: natural 13 C# не inferred Color candidate в ordinary A minor;
- explicit #11/b9 остаются authoritative;
- без Key inferred profile не изобретается.

### ClosedVoicingHarmonizerTests

Проверить:

- `Bm7b5` сохраняет F=b5 на Clean/Color/Rich;
- Color `E7 -> Am` не вставляет C# natural 13;
- Rich `E7 -> Am` способен выбрать F=b9 в подходящем Closed voicing;
- melody остаётся V1;
- existing 0.3b–0.3d regressions остаются зелёными.

---

## 6. Studio Pro acceptance

Проверять generated MIDI визуально и на слух.

### A. Major II-V-I

```text
Dm7 | G7 | Cmaj7
```

Сравнить Clean / Color / Rich.

Ожидание:

- Clean = structural;
- Color = inside/natural colour;
- Rich может повысить dominant tension, но не обязан менять каждый chord.

### B. Minor II-V-I — главный regression

```text
Bm7b5 | E7 | Am
```

Проверить:

- [ ] `Bm7b5` сохраняет F=b5 в Clean;
- [ ] `Bm7b5` сохраняет F=b5 в Color;
- [ ] `Bm7b5` сохраняет F=b5 в Rich;
- [ ] Color не вставляет C# natural 13 в E7 автоматически;
- [ ] Rich реально отличается на E7 и может использовать minor-target tension;
- [ ] Am target остаётся читаемым разрешением.

### C. Same dominant, different target

Сравнить:

```text
G7 -> Cmaj7
E7 -> Am
```

Ожидание: inferred tension vocabulary не обязан быть одинаковым.

### D. Confirmed vs unconfirmed secondary dominant

```text
C major:
D7 -> G
D7 -> Am
```

Ожидание:

- D7->G = confirmed target evidence;
- D7->Am = unresolved/candidate only;
- Rich не должен одинаково трактовать оба случая.

### E. Explicit altered dominant

Проверить отдельно:

```text
E7b9
E7#5
E7b5
E13
```

Ожидание: explicit Chord Track остаётся authoritative при Clean/Color/Rich.

### F. Level switching

На удержанной melody:

```text
Clean -> Color -> Rich -> Clean
```

- V1 не меняется;
- V2–V4 перестраиваются чисто;
- нет transient/stuck notes.

### G. State

- выбрать Rich;
- save/reopen;
- Rich восстанавливается.

### H. Key / modulation regression

Смена Key Track не должна ломать target-aware profile и не должна создавать stuck/transient notes.

---

## 7. Что НЕ проверяем как критерий 0.3e

Полноценный Voice Leading между successive voicings не является gate этой версии.

Accepted future direction:

```text
minimum musically necessary motion
```

но previous Voice state будет добавлен на Stage 6 / #10.

---

## 8. Acceptance 0.3e

0.3e можно закрыть, когда:

1. [ ] Windows CI green на финальном HEAD;
2. [ ] host-neutral Functional Tension Profile tests green;
3. [ ] characteristic b5 regression green;
4. [ ] Studio Pro major II-V-I acceptable;
5. [ ] Studio Pro minor II-V-I показывает правильную chord identity;
6. [ ] Rich на minor dominant имеет функционально отличимое напряжение;
7. [ ] confirmed/unconfirmed dominant target regression acceptable;
8. [ ] explicit altered chord metadata сохраняется;
9. [ ] Level switch/state/Key regression проходит;
10. [ ] Issue #8/#25/#28, PR #22 и project docs синхронизированы.

После этого Stage 4 фиксируется как stable `0.4`, PR #22 готовится к merge, затем начинается Stage 5 / `0.4a` (#9).

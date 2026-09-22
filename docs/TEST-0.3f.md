# Smart Voicing 0.3f — Target-aware Color

Статус: **IN DEVELOPMENT**.

Цель: довести `Functional Tension Profile` после Studio Pro тестов 0.3e так, чтобы `Color` и `Rich` опирались на **реальный следующий Chord Track event**, а не на предполагаемое разрешение.

## 1. Главное правило 0.3f

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
→ appropriate Color / Rich vocabulary
```

Smart Voicing не должен додумывать будущий аккорд. Если аранжировщику нужен target-aware dominant colour, target должен присутствовать в Chord Track.

## 2. Семантика уровней

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour именно данного real target

Rich
→ functionally intensified tension / altered colour
```

Знак `b/#` не определяет уровень автоматически. Например `b13` на `V -> minor` может быть естественной Color-краской, а natural 13 при confirmed minor target может быть менее уместна.

## 3. Minor-target dominant contract

Для confirmed:

```text
A7 -> Dm
E7 -> Am
```

0.3f policy:

```text
b13
→ Preferred / Color candidate
→ functionallyDirected

b9
→ Contextual / Rich candidate
→ functionallyDirected

#9 / #11
→ Contextual / Rich candidates

natural 13
→ не inferred Color при confirmed minor target
```

Explicit `A13 / E13` остаётся authoritative и не переписывается inference.

Natural 9 может оставаться restrained Color candidate только когда его поддерживает текущий tonal context.

## 4. Unresolved dominant contract

Пример:

```text
A7
```

без следующего Dm:

```text
Functional Profile = Dominant / unresolved
```

Движок не должен выводить `Dm` из Key/Function как будто это реальный target.

Generic natural 9/13 могут оставаться conservative unresolved Color vocabulary; altered candidates не получают directed reward.

## 5. Host-neutral regressions

- [x] `G7` без next chord = `Dominant / unresolved`;
- [x] no target -> `resolutionConfirmed = false`;
- [x] `A7` без Dm не получает inferred minor target;
- [x] `A7 -> Dm` = confirmed `Dominant -> minor target`;
- [x] `A7 -> Dm`: b13 доступна Color;
- [x] `A7 -> Dm`: natural 13 не inferred Color;
- [x] `A7 -> Dm`: b9 Rich-only;
- [x] `E7 -> Am`: тот же target-aware contract;
- [ ] existing 0.3a–0.3e regressions green в Windows CI.

## 6. Studio Pro acceptance

### A. Major II–V–I

```text
Dm7 | G7 | Cmaj7
```

Ожидание:
- Clean structural;
- Color inside natural colour;
- Rich более напряжённая dominant окраска при confirmed target.

### B. Minor II–V–I

```text
Bm7b5 | E7 | Am
```

Ожидание:
- Bm7b5 сохраняет b5;
- Color не использует natural 13 как generic Mixolydian default;
- Color может использовать b13 как natural minor-target colour;
- Rich может предпочесть b9/#9/#11 или другой более напряжённый directed colour;
- explicit chord material остаётся authoritative.

### C. No target vs real target

Сравнить:

```text
A7
```

и

```text
A7 | Dm
```

Ожидание:
- первый случай unresolved, без target guess;
- второй случай target-aware minor dominant;
- различие является намеренным и зависит от реально записанной progression.

### D. Пользовательский regression

```text
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Особенно проверить A7 перед Dm на Color/Rich.

### E. Explicit dominant colours

```text
E7b9
E7#5
E7b5
E13
```

Explicit symbol выше Functional Profile/Tension Level.

### F. Regression UI/state/live

- [ ] Clean -> Color -> Rich -> Clean на held melody;
- [ ] нет transient/stuck notes;
- [ ] save/reopen сохраняет level;
- [ ] Key Track change не ломает context;
- [ ] Direct Router regression.

## 7. Не входит в 0.3f

- inferred future target;
- previous-state Voice Leading;
- Melodic Context Engine;
- новые VoicingStrategy.

## 8. Exit

0.3f принимается после:

1. [ ] final Windows CI green;
2. [ ] Studio Pro major II–V–I pass;
3. [ ] Studio Pro minor II–V–I pass;
4. [ ] A7 no-target vs A7->Dm соответствует contract;
5. [ ] explicit alterations pass;
6. [ ] state/live regressions pass;
7. [ ] Issues #8/#25/#28/#29 + PR #22 + docs synchronized.

После acceptance:

```text
0.3f ACCEPTED
→ Smart Voicing 0.4 stable Stage 4 checkpoint
→ PR #22 ready / merge
→ Stage 5 / 0.4a
```

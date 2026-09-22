# Smart Voicing — Functional Tension Profiles

**Status:** 0.3f design contract  
**Related Issues:** #8, #24, #25, #28, #29  
**Related docs:** `TENSION-LEVELS.md`, `VOICE-LEADING-DIRECTION.md`, `TEST-0.3f.md`

## 1. Зачем нужен отдельный Functional Tension Profile

Studio Pro musical tests 0.3d–0.3e показали, что одного правила

```text
Chord + Key + Function
→ список доступных tensions
```

недостаточно.

Две доминанты с одинаковым chord quality могут требовать разной окраски в зависимости от **реального следующего аккорда**:

```text
G7 -> Cmaj
E7 -> Am
```

Они обе имеют dominant function, но target quality и направление resolution различаются. Поэтому Smart Voicing сначала читает реальный harmonic turn из Chord Track, а уже затем решает, какой inferred colour допустим для `Color` и какой tension оправдан для `Rich`.

## 2. Архитектура

```text
Current Chord
 + Current Key
 + Harmonic Function
 + Real Next Chord / Resolution Target
        ↓
Functional Tension Profile
        ↓
Tension Level
        ↓
Harmonic Candidate Pool / weights
        ↓
Voicing Strategy
        ↓
Stage 6: previous-state Voice Leading
```

`Functional Tension Profile` не переписывает Chord Track. Он используется только для **inferred** colour простого chord symbol.

## 3. Главное правило target evidence

0.3f фиксирует строгую политику:

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

Smart Voicing не должен додумывать продолжение оборота из Key/Function. Даже если `A7` теоретически ожидает Dm, без реально записанного `Dm` это остаётся unresolved dominant.

Это намеренное ограничение текущего Stage 4: аранжировщик явно задаёт progression в Chord Track, а движок интерпретирует её, не сочиняя будущую гармонию.

## 4. Минимальные профили

```text
Neutral
Dominant / unresolved
Dominant -> major target
Dominant -> minor target
```

### Neutral

Обычная tonal/modal inference для non-dominant harmony.

### Dominant / unresolved

Chord quality = dominant, но реальный следующий Chord Track event отсутствует или не подтверждает ожидаемый target root.

В этом режиме:
- natural 9/13 могут оставаться conservative generic Color vocabulary;
- altered candidates могут существовать для Rich;
- altered candidates **не получают functionally-directed reward**;
- profile не объявляет major/minor target заранее.

### Dominant -> major target

Пример:

```text
G7 -> Cmaj7
D7 -> G7
```

Natural 9/13 — типичная inside Color. Rich может использовать более напряжённые altered colours, если actual resolution подтверждён и vertical scoring это оправдывает.

### Dominant -> minor target

Пример:

```text
E7 -> Am
A7 -> Dm
```

Generic Mixolydian не является default для confirmed minor target.

0.3f разделяет function-aware Color и Rich:

```text
b13
→ Preferred / Color candidate
→ functionallyDirected

b9
→ Contextual / Rich candidate
→ functionallyDirected

#9 / #11(b5)
→ Contextual / Rich candidates

natural 13
→ не inferred Color при confirmed minor target
```

Natural 9 может оставаться restrained Color option только когда его поддерживает текущий tonal context.

Важно: `b13` попадает в Color не потому, что она «не очень напряжённая», а потому что в `V -> minor` это естественная function-aware краска. Знак `b/#` сам по себе не определяет уровень.

## 5. Generic dominant resolution evidence

`HarmonicAnalysis` сохраняет root и quality следующего аккорда.

Для dominant-quality chord ожидаемый target root:

```text
current root + perfect fourth
= +5 semitones
```

Если **реальный** next Chord Track event имеет этот root:

```text
dominantResolutionConfirmed = true
```

и сохраняются:

```text
dominantTargetPitchClass
dominantTargetQuality
```

Это работает как для ordinary V->I, так и для secondary/applied dominant.

Примеры:

```text
D7 -> G
```

= confirmed target evidence.

```text
D7 -> Am
```

= next chord присутствует, но не является ожидаемым target D7; profile остаётся unresolved.

```text
A7
```

без следующего chord event = unresolved. Движок не выводит Dm сам.

```text
A7 -> Dm
```

= confirmed minor-target profile.

## 6. Tension Level поверх профиля

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour именно данного target

Rich
→ functionally intensified tension
```

### Почему Rich не равен «больше tensions»

Неправильно:

```text
Rich = разрешить b9 #9 #11 b13 и выбрать ближайшую
```

Правильно:

```text
Real Function + Real Target
→ определяют смысл tension

Color
→ выбирает естественные краски этого оборота

Rich
→ разрешает более сильное направленное напряжение
```

Если target evidence отсутствует, Rich не должен притворяться, что знает разрешение.

## 7. Explicit chord symbol выше профиля

```text
E7b9
E7#5
E7b5
E13
```

— разные explicit harmonic instructions.

Smart Voicing не превращает их в generic `E7` и не угадывает colour заново.

Приоритет:

```text
Explicit Chord Track
>
Functional Tension Profile
```

Поэтому explicit `E13` остаётся E13 даже перед minor target, хотя simple `E7 -> Am` не должен автоматически получать natural 13 как inferred Color.

## 8. Characteristic chord tones

Functional colour не должен разрушать chord identity.

Policy:

```text
ordinary perfect 5th
→ often expendable

half-diminished b5
augmented #5
sus2 / sus4 identity tone
explicit altered fifth
→ characteristic / protected
```

Контрольный пример:

```text
Bm7b5 = B D F A
```

`F = b5` сохраняется как identity tone. Замена F на E=11 только ради более компактного/цветного voicing не является default behavior.

## 9. Связь с Voice Leading

Stage 4 формирует **правильный функциональный vocabulary**.

Stage 6 позже добавляет previous-state cost:

```text
common tone
stepwise motion
voice identity
leap penalty
tendency-tone resolution
```

Например для:

```text
E7(b9,b13) -> Am
```

Stage 6 сможет отдельно награждать:

```text
F  -> E
G# -> A
D  -> C
```

Но Stage 4 уже должен правильно различать:
- b13 как natural Color candidate minor-target dominant;
- b9 как stronger Rich candidate;
- natural 13 как не-default inferred colour при confirmed minor target.

## 10. Regression cases

### Major II-V-I

```text
Dm7 | G7 | Cmaj7
```

### Minor II-V-I

```text
Bm7b5 | E7 | Am
```

### No target vs confirmed target

```text
A7
```

vs

```text
A7 | Dm
```

### User harmonic regression

```text
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Критерии 0.3f:

- `Bm7b5` сохраняет b5 на Clean / Color / Rich;
- `E7 -> Am` и `A7 -> Dm` не получают natural 13 механически;
- Color может использовать target-aware b13;
- Rich может использовать более напряжённый b9/#9/#11;
- no next chord не создаёт assumed target;
- major/minor targets дают разные profiles;
- explicit altered chords остаются authoritative.

## 11. Граница ответственности

0.3f **не** решает:

- inferred future target;
- полноценный previous-state Voice Leading;
- approach/passing-note classification;
- выбор Closed/Drop/Spread/Quartal/Cluster/UST.

0.3f отвечает на один фундаментальный вопрос:

> Какой harmonic colour имеет смысл в реально записанном обороте до того, как Voicing Strategy и Voice Leading выберут конкретное расположение голосов?

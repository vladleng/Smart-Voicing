# Smart Voicing — Functional Tension Profiles

**Status:** 0.3e design contract  
**Related Issues:** #8, #24, #25, #28  
**Related docs:** `TENSION-LEVELS.md`, `VOICE-LEADING-DIRECTION.md`

## 1. Зачем нужен отдельный Functional Tension Profile

Studio Pro musical test 0.3d показал, что одного правила

```text
Chord + Key + Function
→ список доступных tensions
```

недостаточно.

Две доминанты с одинаковым chord quality могут требовать разной окраски в зависимости от реального разрешения:

```text
G7 -> Cmaj
E7 -> Am
```

Они обе имеют dominant function, но target quality и направление resolution различаются. Поэтому Smart Voicing должен сначала понять **куда ведёт гармония**, а уже затем решать, какой inferred colour допустим для `Color` и какой tension оправдан для `Rich`.

## 2. Архитектура

```text
Current Chord
 + Current Key
 + Harmonic Function
 + Next Chord / Resolution Target
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

## 3. Минимальные профили 0.3e

```text
Neutral
Dominant / unresolved
Dominant -> major target
Dominant -> minor target
```

### Neutral

Обычная tonal/modal inference для non-dominant harmony.

### Dominant / unresolved

Chord quality = dominant, но actual next chord не подтверждает ожидаемый target или next chord неизвестен и нельзя надёжно вывести target quality.

В этом режиме Rich может видеть altered candidates, но не должен считать их направленными только из-за факта dominant chord.

### Dominant -> major target

Пример:

```text
G7 -> Cmaj7
D7 -> G7
```

Natural 9/13 могут быть inside Color. Rich получает доступ к более напряжённым altered colours, но должен предпочитать их только при достаточном resolution evidence / voicing advantage.

### Dominant -> minor target

Пример:

```text
E7 -> Am
```

Generic Mixolydian больше не является универсальным default. Natural 13 не должна автоматически считаться `Preferred`, если active minor context её не поддерживает.

Направленные Rich candidates:

```text
b9
b13
```

Более контекстные:

```text
#9
#11 / b5
```

Конкретная нота выбирается не потому, что она «altered», а потому что она поддерживает функцию, target и будущую логику разрешения.

## 4. Generic dominant resolution evidence

Начиная с 0.3e `HarmonicAnalysis` сохраняет не только root следующего аккорда, но и его quality.

Для dominant-quality chord ожидаемый target root:

```text
current root + perfect fourth
= +5 semitones
```

Если реальный next Chord Track event имеет этот root:

```text
dominantResolutionConfirmed = true
```

и сохраняются:

```text
dominantTargetPitchClass
dominantTargetQuality
```

Это работает как для ordinary V->I, так и для secondary/applied dominant.

Важно:

```text
D7 -> G
```

даёт confirmed target evidence.

```text
D7 -> Am
```

не должен заимствовать minor quality Am как будто это target D7. Такой случай остаётся unresolved/candidate context.

## 5. Tension Level поверх профиля

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour

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
Function + Target
→ определяют смысл tension

Rich
→ разрешает более сильное напряжение,
   если оно подтверждено этим контекстом
```

Если strong evidence отсутствует, Rich может совпасть с Color.

## 6. Explicit chord symbol выше профиля

```text
E7b9
E7#5
E7b5
E13
```

— это разные explicit harmonic instructions.

Smart Voicing не должен превращать их в один generic `E7` и повторно угадывать colour.

Приоритет:

```text
Explicit Chord Track
>
Functional Tension Profile
```

Профиль нужен прежде всего для простого `E7`, `G7`, `D7` и т. п., когда colour должен выводиться из harmonic turn.

## 7. Characteristic chord tones

Functional colour не должен разрушать исходную chord identity.

Новая policy 0.3e:

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

`F = b5` должен сохраняться как identity tone. Замена F на E=11 только ради более компактного/цветного voicing недопустима как default.

## 8. Связь с Voice Leading

0.3e формирует **правильный функциональный vocabulary**.

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

Но 0.3e уже должен понимать, что F=b9 и C=b13 являются осмысленными candidates для minor-target dominant.

## 9. Regression cases

### Major II-V-I

```text
Dm7 | G7 | Cmaj7
```

### Minor II-V-I

```text
Bm7b5 | E7 | Am
```

Критерии 0.3e:

- `Bm7b5` сохраняет b5 на Clean / Color / Rich;
- `E7 -> Am` не получает natural 13 C# механически из generic Mixolydian;
- Rich может выбрать functionally directed b9/b13;
- `G7 -> Cmaj` использует другой profile, чем `E7 -> Am`;
- `D7 -> G` и `D7 -> Am` различаются по resolution evidence;
- explicit altered chords остаются authoritative.

## 10. Граница ответственности

0.3e **не** решает:

- полноценный previous-state Voice Leading;
- approach/passing-note classification;
- выбор Closed/Drop/Spread/Quartal/Cluster/UST.

0.3e отвечает на один фундаментальный вопрос:

> Какой harmonic colour имеет смысл в данном обороте до того, как Voicing Strategy и Voice Leading выберут конкретное расположение голосов?

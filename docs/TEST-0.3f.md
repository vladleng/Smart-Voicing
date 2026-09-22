# Smart Voicing 0.3f — Target-aware Color

Статус: **ACCEPTED / CLOSED — 2026-09-22**.

0.3f завершает музыкальную корректировку Stage 4 перед стабильным checkpoint 0.4.

## 1. Финальный контракт

```text
No next chord
→ no assumed resolution target
→ Dominant / unresolved

Real next chord on expected target root
→ confirmed target root + target quality
→ target-aware Functional Tension Profile
```

Smart Voicing интерпретирует реальную progression из Chord Track и не додумывает будущий target.

## 2. Tension Level — принятая семантика

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside colour именно данного real target

Rich
→ functionally intensified tension / altered colour
```

Знак `b/#` сам по себе не определяет уровень. Для `V -> minor` `b13` может быть нормальной Color-краской, тогда как `b9/#9/#11` чаще относятся к более напряжённому Rich vocabulary.

## 3. Host-neutral regression — PASS

- [x] `G7` без next chord = `Dominant / unresolved`;
- [x] no target -> `resolutionConfirmed = false`;
- [x] `A7` без Dm не получает inferred minor target;
- [x] `A7 -> Dm` = confirmed `Dominant -> minor target`;
- [x] `A7 -> Dm`: b13 доступна Color;
- [x] `A7 -> Dm`: natural 13 не inferred Color;
- [x] `A7 -> Dm`: b9 доступна Rich, не Color;
- [x] `E7 -> Am` следует тому же target-aware contract;
- [x] `Bm7b5` сохраняет characteristic b5;
- [x] explicit chord material остаётся authoritative;
- [x] regressions 0.3a–0.3e прошли Windows CI.

## 4. CI — PASS

**Windows Build #302**  
Run: `35682905650`  
HEAD: `40922a936e725c297009cb38620a0cd098c0aad7`  
Result: **success**.

Подтверждено:
- обе VST3 собираются;
- Harmony Core tests проходят;
- package/artifact 0.3f создаётся;
- host-neutral Tension Policy / Closed / Key-aware regressions green.

## 5. Studio Pro musical acceptance — PASS

Пользователь подтвердил завершение этапа после сравнения Clean / Color / Rich на нескольких гармонических примерах.

### Major II–V–I

```text
Dm7 | G7 | Cmaj7 | Am7
```

Результат:
- Clean сохраняет structural harmony;
- Color использует natural inside colour;
- Rich усиливает dominant tension при реальном target, не альтерируя всё подряд.

### Minor II–V–I

```text
Bm7b5 | E7 | Am7
```

Результат:
- `Bm7b5` сохраняет `F = b5`;
- Color не использует generic natural 13 как Mixolydian default;
- Rich получает minor-target altered vocabulary.

### No target vs real target

Сравнивались:

```text
A7
```

и

```text
A7 | Dm7
```

Подтверждено:

```text
A7 without target
Clean → structural A7
Color → conservative unresolved dominant colour (например 13)
Rich  → не делает вид, что знает Dm target

A7 -> Dm7
Clean → structural A7
Color → target-aware b13 colour
Rich  → более напряжённые directed colours: b9 / b5(#11) и т. п. в зависимости от melody/vertical context
```

Это считается ключевым acceptance result 0.3f.

### User regression progression

```text
Dm7 | Db7b13 | Cm7 | B7#11 | Bbmaj7 | A7 | Dm7
```

Подтверждено:
- explicit `Db7b13` и `B7#11` сохраняют заданный harmonic material;
- A7 меняет inferred Color/Rich vocabulary только при наличии реального Dm target;
- melody остаётся authoritative V1.

## 6. Что намеренно НЕ входит

- inferred future target;
- полноценный previous-state Voice Leading;
- Melodic Context / approach-note classification;
- новые VoicingStrategy beyond Closed.

Эти функции не являются блокерами Stage 4.

## 7. Важная граница

Stage 4 отвечает:

> Какие pitch classes музыкально оправданы в реально записанном harmonic turn?

Stage 5 отвечает:

> Как организовать этот правильный harmonic material в разные voicing shapes?

Stage 6 отвечает:

> Как выбрать последовательность voicings с minimum musically necessary motion?

## 8. Exit

- [x] Windows CI green;
- [x] major II–V–I musical acceptance;
- [x] minor II–V–I musical acceptance;
- [x] A7 no-target vs A7->Dm соответствует contract;
- [x] explicit alterations regression;
- [x] пользователь объявил Stage 4 завершённым;
- [x] 0.3f accepted и переводится в stable 0.4 checkpoint.

Следующий checkpoint:

```text
Smart Voicing 0.4
= Stable Stage 4 — Key-aware Engine + Functional Tensions
```

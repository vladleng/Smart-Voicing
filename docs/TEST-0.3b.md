# Smart Voicing 0.3b — Melody Harmonize → Closed Voicing MVP

Цель версии: превратить существующий режим `Melody Harmonize` из простого nearest-chord-tone MVP в первый музыкально осмысленный **Closed Voicing Engine**, сохраняя V1 как performer-owned melody и строя V2–V4 как единую компактную гармоническую вертикаль.

## Музыкальная концепция

Практический референс:

**Closed Voicing, pt. 1 - Big Band Arranging SECRETS REVEALED**  
https://www.youtube.com/watch?v=hSfNLJuAYSg&t=121s

Используется не буквальная big-band оркестровка, а принцип мышления:

```text
Melody V1
   ↓
Current Chord
   +
Key / Function context
   +
explicit / melody-imposed tension context
   ↓
кандидаты V2 / V3 / V4
   ↓
оценка полной вертикали
   ↓
Guide tones / omissions / compactness / spacing
   ↓
Closed Voicing
```

## Что меняется относительно 0.3 / 0.3a

Раньше базовый `CloseVoicingHarmonizer` просто выбирал ближайшие chord tones ниже melody.

В 0.3b появляется candidate-based оценка всей вертикали:

- `V1` всегда остаётся сыгранной melody note;
- non-chord / tension melody не исправляется;
- V2–V4 должны идти строго ниже V1;
- compactness предпочтительна примерно в пределах одной октавы, но не является hard constraint;
- guide tones / characteristic tones получают повышенный приоритет;
- root может быть опущен в seventh / extended voicing, если гармоническая идентичность читается без него;
- fifth не является обязательной;
- для простых triads root получает дополнительный вес для сохранения идентичности;
- explicit slash bass остаётся авторитетным нижним голосом V4, когда его можно корректно разместить;
- explicit chord colours / extensions остаются допустимыми кандидатами;
- алгоритм остаётся realtime-safe: fixed-size search, без dynamic allocation / mutex / file I/O.

## Upper Voice Spacing Policy

Интервал `V1–V2` оценивается как soft preference:

- 3rd — preferred;
- 4th — common / acceptable;
- 2nd — contextual;
- 5th+ — contextual / open.

Терция **не является правилом**. Нельзя получать её ценой неправильного chord tone, guide tone, explicit tension, slash bass или плохой полной вертикали.

## Автоматические тесты

`SmartVoicingHarmonizerTests` должны подтвердить минимум:

1. **Cmaj7 + G melody** сохраняет эталонный closed result из Stage 3:
   - V1 G4;
   - V2 E4;
   - V3 C4;
   - V4 B3.

2. **Cmaj7 + D melody (melody-imposed 9)**:
   - melody D сохраняется как V1;
   - ожидаемый compact closed candidate: `D–B–G–E`.

3. **Dm7 + E melody (9)**:
   - melody E сохраняется;
   - ожидаемая вертикаль: `E–C–A–F`;
   - присутствуют 3rd и 7th.

4. **G7 + A melody (9), dominant context**:
   - разрешён root omission;
   - ожидаемая вертикаль: `A–F–D–B`;
   - 3rd и 7th сохраняют dominant identity.

5. **C major triad + G melody**:
   - root не должен случайно исчезнуть;
   - ожидается `G–E–C–G`.

6. **C/E slash chord**:
   - V4 остаётся explicit E bass.

7. **No Chord**:
   - V1 остаётся melody;
   - V2–V4 inactive.

8. Старый `buildCloseVoicing()` временно остаётся compatibility entry point и должен использовать тот же Closed Engine.

## Ручной Studio Pro test после зелёного CI

Проверить в `Melody Harmonize`:

- обычную chord-tone melody;
- melody как 9;
- melody как другой non-chord tone;
- seventh chords, где root omission звучит естественно;
- triads — гармоническая идентичность не должна теряться;
- slash chords;
- live Chord Track reharmonization;
- Sustain-held melody;
- переход `(no chord) → chord`;
- отсутствие stuck notes;
- Direct Router regression.

Отдельно послушать `V1–V2` spacing: терция должна часто появляться как естественный preferred result, но не должна форсироваться там, где quart / second / wider spacing музыкально лучше.

## Что сознательно не входит в 0.3b

- выбор `Voicing Type` в UI;
- Drop 2 / Drop 3 / Drop 2+4 / Open / Spread;
- полноценный Stage 6 Voice Leading между последовательными voicings;
- resolution-aware secondary dominant confirmation;
- полный `Tension Policy` Available / Preferred / Avoid — это 0.3d;
- Instrument Profiles / ranges.

`Closed` в этой версии становится базовой гармонизацией. На Stage 5 он будет первым `Voicing Type`, а Drop/Open-варианты должны преобразовывать уже выбранный Closed material, а не заново определять гармонические ноты.

## Версия

- пользовательская версия: `Smart Voicing 0.3b`;
- CMake mapping: `0.3.2`;
- Windows artifact: `Smart-Voicing-0.3b-Windows`.

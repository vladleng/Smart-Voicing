# Smart Voicing 0.3a — Key Context + Harmonic Function MVP

Цель версии: начать Этап 4 и добавить первый host-neutral аналитический слой, который интерпретирует текущий `NormalizedChord` внутри активного `KeyContext`, не меняя звучание и voicing относительно стабильной 0.3.

## Что добавляется

- `KeyModel` / `NormalizedKey`;
- нормализация root Key из ARA circle-of-fifths в pitch class;
- определение `major / minor / custom` по interval mask Key Track;
- определение ступени root текущего chord относительно Key;
- базовая функция `Tonic / Predominant / Dominant / Other`;
- проверка `Diatonic / Chromatic` по фактическим chord tones относительно key scale;
- `Applied Dominant Candidate` и предполагаемая целевая ступень;
- безопасный fallback: если Key отсутствует, существующий Chord-aware Harmonizer 0.3 продолжает работать без Key-aware анализа.

## Важная граница 0.3a

0.3a **не объявляет secondary dominant окончательно только по одному chord symbol**.

Например, `D7` в `C major` структурно является сильным кандидатом `V/V`, но реальная функция окончательно подтверждается контекстом/разрешением. Поэтому в 0.3a используется понятие `Applied Dominant Candidate` + target degree. Resolution-aware подтверждение можно добавить следующей итерацией Stage 4, используя соседние события Chord Track.

Это позволяет не путать, например, dominant-form tonic/blues chord и фактический `V/IV`, когда одного текущего аккорда недостаточно для однозначной интерпретации.

## Приоритет данных

```text
Played / Melody
      >
Current Chord
      >
Current Key
      >
Harmonic Function
```

Key и Function являются контекстом и не переписывают явно заданный Chord Track.

## Автоматические тесты

`SmartVoicingKeyAwareTests` должны подтвердить минимум:

- C major и A minor корректно нормализуются;
- C major: C=I, D=II, B=VII, C#=chromatic;
- `Cmaj7` в C major = I, Tonic, Diatonic;
- `Dm7` в C major = II, Predominant, Diatonic;
- `G7` в C major = V, Dominant, Diatonic и не applied dominant;
- `D7` в C major = chromatic chord, candidate `V/V`, target degree V;
- `A7` в C major = candidate `V/II`, target degree II;
- `D7` в G major = обычная V dominant, не applied candidate;
- отсутствие Key даёт safe fallback без ложной функции;
- все тесты 0.2b–0.3 продолжают проходить.

## Что сознательно не входит в 0.3a

- влияние Key/Function на V1–V4;
- automatic tensions;
- borrowed chords / modal interchange;
- resolution-aware secondary dominant confirmation;
- новые voicing types;
- Voice Leading;
- Instrument Profiles.

## Ручной тест в Studio Pro

После зелёного CI добавить/проверить diagnostics текущих `Key / Degree / Function / Relation / Applied target` и пройти несколько комбинаций одного chord в разных Key Track. Звучание Melody Harmonize и Direct Router должно остаться идентичным 0.3.

## Версия

- пользовательская версия: `Smart Voicing 0.3a`;
- CMake mapping: `0.3.1`;
- Windows artifact: `Smart-Voicing-0.3a-Windows`.

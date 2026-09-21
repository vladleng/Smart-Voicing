# Smart Voicing 0.2b — контрольный тест

Цель версии: превратить структурные данные текущего ARA Chord (`root`, `bass`, `intervals[12]`) в host-neutral внутреннюю chord model, пригодную для следующего шага — Melody Harmonize.

## Что добавлено

- `ChordModel.h/.cpp`;
- перевод ARA circle-of-fifths root / bass в canonical pitch class `0..11`;
- сохранение исходного fifths-index для корректного различения enharmonic spelling;
- `ChordQuality`: major / minor / dominant / diminished / half-diminished / augmented / sus2 / sus4 / power / no-third / unknown;
- `ChordExtension`: 6 / b7 / maj7 / 9 / 11 / 13;
- `ChordAlteration`: b5 / #5 / b9 / #9 / #11 / b13;
- отдельный признак slash bass;
- использование ARA diatonic degree annotations `1..13`, когда host их предоставляет;
- безопасная классификация generic interval mask без попытки угадывать неоднозначные extensions;
- `SmartVoicingCoreTests` без JUCE / ARA зависимости;
- запуск `ctest` в Windows CI;
- диагностический `normalizedChordSymbol()`, который формирует читаемый символ аккорда из внутренней модели, а не из неполного host display name.

## Что сознательно не входит в 0.2b

- генерация дополнительных MIDI-голосов;
- Close voicing;
- voice leading;
- реакция удержанной melody note на смену аккорда;
- instrument ranges;
- изменение поведения MIDI Router 0.2.

## Автоматические тесты

CI проверяет как минимум:

- circle-of-fifths: C / G / D / F / Bb / C# / Db;
- major / minor / dominant;
- maj7 / m7;
- diminished / diminished7 / half-diminished;
- sus2 / sus4 / augmented;
- 6 / 9 / 13;
- b5 / #5 / b9 / #9 / #11 / b13;
- slash chord `C/E`;
- generic ARA interval usage без diatonic degree;
- undefined chord;
- читаемые символы `Dsus4`, `D7sus4`, `D7#11`, `C7b9`, `C7#9`, `C7b13`, slash chord.

## Результаты пользовательских тестов в Studio Pro — 2026-09-20

Подтверждено:

- `A/B` корректно распознаётся как slash chord;
- diminished корректно определяется как `quality diminished`;
- `Dsus4` корректно определяется как `quality sus4`;
- `D7#11` корректно определяется как dominant с extension 11 и alteration `#11`;
- после перехода UI на `normalizedChordSymbol()` сложный altered dominant `D7#9#11b13` отображается полностью и совпадает с Chord Selector Studio Pro;
- ARA context продолжает корректно передавать Chord / Key / Time Signature / Tempo;
- MIDI Router 0.2 после повторной проверки работает штатно: V1–V4 / Ch1–4, Sustain / Voice Stack без выявленных регрессий и stuck notes.

## CI

- Windows Build #185 — `success`;
- `SmartVoicingCoreTests` / `ctest` — `success`;
- пакет `Smart-Voicing-0.2b-Windows` создаётся штатно.

## Итог

**Smart Voicing 0.2b подтверждена и закрыта 2026-09-20.**

Критерии прохождения выполнены: Chord Model подтверждена автоматическими тестами и реальным Studio Pro тестом, включая sus / extensions / alterations / slash bass; Router 0.2 регрессий не показал.

Следующая итерация: `0.2c — Melody Harmonize MVP + базовый Close voicing`.

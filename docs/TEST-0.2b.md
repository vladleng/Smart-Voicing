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

CI должен проверить как минимум:

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

## Ручной тест в Studio Pro

После успешного Windows CI:

1. Установить оба компонента из пакета `Smart Voicing 0.2b`.
2. Убедиться, что ARA Context продолжает переключать Chord / Key / Time Signature без регрессий.
3. Проверить несколько аккордов Chord Track, желательно:
   - `C`;
   - `Cm`;
   - `C7`;
   - `Cmaj7`;
   - `Cm7`;
   - `Cm7b5`;
   - `Cdim` / `Cdim7`;
   - `Csus2` / `Csus4`;
   - `C/E`;
   - один altered dominant, например `D7#11`.
4. Сверить, что строка `Аккорд:` теперь показывает **нормализованный символ Chord Model**, а не только host display name.
5. В технической диагностике проверить `Chord model: ... | symbol ... | quality ... | ext flags ... | alt flags ...`.
6. Коротко проверить старый Router 0.2: четыре канала, Sustain/Voice Stack и отсутствие stuck notes.

## Результат первого теста в Studio Pro — 2026-09-20

Пользователь проверил `A/B`, diminished, `Dsus4` и `D7#11`.

Подтверждено по диагностике:

- slash bass `A/B` распознан: root PC 9, bass PC 11, `slash YES`;
- diminished распознан как `quality diminished`;
- `Dsus4` уже корректно попадал во внутреннюю модель как `quality sus4`, raw interval mask `0 5 7`;
- `D7#11` уже корректно попадал во внутреннюю модель как `quality dominant`, `ext flags 18`, `alt flags 16`, то есть b7 + 11 и отдельная #11 alteration.

Обнаруженная проблема относилась не к Chord Model, а к отображению: большая строка `Аккорд:` использовала старый host/context text, который для этих примеров показывал только `D` и `D7`, скрывая `sus4` и `#11`. Поэтому 0.2b **не закрывается до повторного теста обновлённой сборки**.

Исправление: добавлен `normalizedChordSymbol()` и UI переведён на символ, построенный из внутренней Chord Model. Raw host text остаётся только как сравнительная диагностика.

## Критерий прохождения 0.2b

Версия подтверждается после двух условий:

1. `SmartVoicingCoreTests` успешно проходят в Windows CI.
2. Пользователь подтверждает в Studio Pro, что несколько разных Chord Track событий, включая sus / extensions / alterations / slash bass, отображаются и классифицируются ожидаемо, а Router 0.2 не получил регрессий.

После этого можно переходить к `0.2c — Melody Harmonize MVP + базовый Close voicing`.

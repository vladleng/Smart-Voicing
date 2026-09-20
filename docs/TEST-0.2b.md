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
- запуск `ctest` в Windows CI.

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
- undefined chord.

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
   - один altered dominant, если Studio Pro позволяет удобно задать его.
4. Сверить, что диагностическая chord model определяет семейство аккорда и slash bass ожидаемо.
5. Коротко проверить старый Router 0.2: четыре канала, Sustain/Voice Stack и отсутствие stuck notes.

## Критерий прохождения 0.2b

Версия подтверждается после двух условий:

1. `SmartVoicingCoreTests` успешно проходят в Windows CI.
2. Пользователь подтверждает в Studio Pro, что несколько разных Chord Track событий классифицируются ожидаемо и Router 0.2 не получил регрессий.

После этого можно переходить к `0.2c — Melody Harmonize MVP + базовый Close voicing`.

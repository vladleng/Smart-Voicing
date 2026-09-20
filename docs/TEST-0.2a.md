# Smart Voicing 0.2a — контрольный тест

Цель версии: проверить новый host-neutral слой Harmonic Context поверх уже подтверждённого ARA/shared-memory bridge, не меняя музыкальное поведение MIDI Router 0.2.

## Что добавлено

- `HarmonicContext.h` — нейтральные `ChordContext`, `KeyContext`, `TimeSignatureContext`, `HarmonicContext`;
- `IHarmonicContextProvider` — интерфейс источника гармонического контекста;
- `ARAContextProvider` — адаптер существующего `SharedHarmonicContextBridge`;
- `VoiceOutput[4]` — нейтральный контракт будущего Harmonizer → Router;
- диагностическое отображение статуса neutral provider в основном Instrument;
- CMake version `0.2.1`, пакет/артефакт `Smart Voicing 0.2a`.

## Что сознательно не менялось

- MIDI Router 0.2;
- Distribution Modes;
- Gesture Classifier;
- Stable Voice Ownership;
- Voice Stack / legato;
- Sustain Chord Morph;
- MIDI Channels 1–4;
- ARA reader и bridge ABI v3.

Гармонизация MIDI в 0.2a ещё не активна.

## Тест в Studio Pro

1. Установить оба компонента из одного пакета `Smart Voicing 0.2a`.
2. Открыть проект, где `Smart Voicing ARA` уже получает Chord / Key / Tempo / Time Signature.
3. Открыть основной `Smart Voicing`.
4. Убедиться, что заголовок показывает `Smart Voicing 0.2a - Harmonic Context Core`.
5. Убедиться, что строка bridge показывает одновременно:
   - `Smart Voicing ARA: CONNECTED`;
   - `Neutral provider: READY`.
6. Перемещать playhead и запускать playback по участкам с разными аккордами.
7. В технической диагностике проверить:
   - `position YES`;
   - `chord DEFINED` на участке с аккордом;
   - `key AVAILABLE` при наличии Key Track;
   - `time signature AVAILABLE`;
   - `Neutral chord: start PPQ ... | root ... | bass ... | interval mask ...` меняется согласно активному Chord Track.
8. Проверить границы нескольких аккордов — старый Context Monitor и neutral provider должны переключаться на одном и том же месте.
9. Сыграть прежний тест Router на 4 SWAM-инструмента:
   - Voice 1–4 остаются на Ch1–4;
   - Distribution Modes работают как в 0.2;
   - Sustain Chord Morph работает;
   - отдельные Voice Gesture сохраняют legato/portamento;
   - CC / Pitch Bend проходят downstream;
   - stuck notes отсутствуют.

## Критерий прохождения 0.2a

Версия считается подтверждённой пользователем, если новый neutral provider показывает тот же активный Chord / Key / Time Signature, что и существующий ARA Context Monitor, а поведение MIDI Router не изменилось относительно 0.2.

После подтверждения можно переходить к следующей итерации Этапа 3: нормализация chord model и первый тестируемый Chord-aware Harmonizer.

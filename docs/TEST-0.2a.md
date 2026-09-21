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

## Результат теста — 2026-09-20

**Статус: ПРОЙДЕН.**

Подтверждено в Studio Pro пользователем:

- заголовок `Smart Voicing 0.2a - Harmonic Context Core` отображается корректно;
- `Smart Voicing ARA: CONNECTED`;
- `Neutral provider: READY`;
- при позиции PPQ 7.0 активный Context Monitor показывает `Dm`;
- Key Context показывает `C major`;
- Time Signature показывает `4/4`;
- Tempo показывает `120.00 BPM`;
- neutral provider сообщает `position YES`, `chord DEFINED`, `key AVAILABLE`, `time signature AVAILABLE`;
- для активного `Dm` neutral chord содержит `start PPQ 4.000000`, `root 2`, `bass 2`, `interval mask 0 3 7`, что соответствует минорному трезвучию D–F–A;
- MIDI Router 0.2 остаётся активным; по пользовательскому тесту новых регрессий не выявлено.

Windows CI для commit `fb22a87d49f9ff329192d18338d4864645d63c52` завершён успешно. Шаги Configure / Build / Prepare versioned package / Upload VST3 package прошли успешно. Создан artifact `Smart-Voicing-0.2a-Windows`.

## Критерий прохождения 0.2a

Версия считается подтверждённой пользователем, если новый neutral provider показывает тот же активный Chord / Key / Time Signature, что и существующий ARA Context Monitor, а поведение MIDI Router не изменилось относительно 0.2.

**Критерий выполнен. Smart Voicing 0.2a зафиксирована как успешно протестированная рабочая итерация Этапа 3.**

Следующая итерация: нормализация chord model и подготовка первого тестируемого Chord-aware Harmonizer.

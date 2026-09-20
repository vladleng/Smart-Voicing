# Smart Voicing 0.2d — контрольный тест

Цель версии: при удержанной melody note автоматически перестраивать сгенерированные нижние голоса на границе нового Chord Track без нового Note On от пользователя.

## Что реализовано

- `LiveReharmonizer` — host-neutral план перехода между текущим и новым `VoiceOutput[4]`;
- V1 исключён из chord-driven transition и остаётся под управлением сыгранной melody note;
- V2–V4 пересчитываются из текущего `NormalizedChord` при активной melody note;
- если конкретный Voice сохраняет ту же ноту, Note Off / Note On не создаются;
- изменившиеся нижние голоса сначала получают Note Off старой ноты, затем Note On новой;
- `(no chord)` оставляет только V1 и выключает V2–V4;
- возвращение валидного аккорда снова создаёт V2–V4;
- добавлена диагностика `reharmonizations` и `last PPQ`;
- добавлены host-neutral `SmartVoicingLiveReharmonizerTests`;
- CMake version: `0.2.4`;
- Windows package: `Smart Voicing 0.2d`.

## Базовое правило 0.2d

```text
V1 = сыгранная и удерживаемая melody note
V2–V4 = заново рассчитанный базовый Close voicing из текущего Chord Track
```

При смене Chord Track V1 не меняется. V2–V4 перестраиваются только если реально изменился их целевой MIDI voicing.

## Исправление точности chord boundary

Первый Studio Pro тест показал небольшой сдвиг записанных V2–V4 вправо относительно границ Chord Track. Причина: исходная 0.2d проверяла новый chord только на границе очередного audio block и посылала transition с `samplePosition = 0` следующего блока.

Исправление оставляет reported plugin latency равной **0 samples** и не использует lookahead. Поскольку ARA заранее предоставляет карту Chord Track, Smart Voicing теперь:

1. получает PPQ начала текущего audio block;
2. заранее находит все chord boundaries, попадающие внутрь этого блока;
3. преобразует PPQ границы в точный sample offset через ARA tempo timeline; если timeline conversion недоступен — использует текущий BPM как fallback;
4. вставляет Note Off / Note On V2–V4 именно на этот `samplePosition` внутри `juce::MidiBuffer`;
5. обрабатывает входные MIDI-события и chord boundaries хронологически, чтобы Note On/Off внутри того же блока не конфликтовали с будущей reharmonization.

То есть исправление не задерживает сыгранную melody note и не добавляет PDC/processing latency. Меняется только точность timestamp сгенерированных MIDI-событий.

## Автоматические тесты

Windows Build #212 подтвердил базовую 0.2d до исправления:

- `SmartVoicingCoreTests` — passed;
- `SmartVoicingHarmonizerTests` — passed;
- `SmartVoicingLiveReharmonizerTests` — passed.

Для sample-accurate исправления добавлены тесты расчёта позиции внутри блока:

- 5 ms при 48 kHz → sample 240;
- 0.01 quarter при 120 BPM / 48 kHz → sample 240;
- event ровно на старте следующего блока не должен попадать в текущий блок;
- прошедшая PPQ boundary не должна планироваться повторно.

После нового коммита требуется новый Windows CI.

## Ручной тест в Studio Pro

Контрольный сценарий:

1. Установить актуальный `Smart Voicing 0.2d` и `Smart Voicing ARA` из одного пакета.
2. Выбрать `Melody Harmonize`.
3. Создать последовательность Chord Track.
4. Нажать и удерживать одну melody note.
5. Записать V1–V4 на отдельные MIDI-дорожки через несколько смен аккордов.
6. Проверить:
   - V1 остаётся непрерывной melody note;
   - V2–V4 перестраиваются точно на вертикальных границах Chord Track, без block-latency вправо;
   - не требуется повторно нажимать melody note;
   - нет зависших старых нот;
   - сам Smart Voicing не сообщает DAW дополнительную latency.
7. Дополнительно проверить `(no chord)`, Note Off/Stop и `Direct Router` в интеграционной регрессии 0.2e / 0.3.

## Результат первого пользовательского теста — 2026-09-20

Пользователь записал выход Smart Voicing в отдельные MIDI-дорожки Studio Pro.

Проверенная последовательность:

```text
Dm7 → Db7 → Cm7add11 → B7b5 → Bb6 → A7
```

Подтверждено:

- V1 / Trumpet остаётся одной непрерывной удержанной melody note;
- V2–V4 автоматически перестраиваются на сменах Chord Track;
- механизм Live Chord Reharmonization работает в Studio Pro.

Одновременно запись выявила небольшой block-latency сдвиг V2–V4 относительно точной границы аккорда. Поэтому 0.2d повторно открыта для точностного исправления и требует повторного Studio Pro теста после нового CI.

## Статус

Базовая Live Chord Reharmonization подтверждена. Реализуется sample-accurate chord-boundary fix без добавления plugin latency. Финальное подтверждение 0.2d — после зелёного CI и повторной MIDI-записи в Studio Pro.

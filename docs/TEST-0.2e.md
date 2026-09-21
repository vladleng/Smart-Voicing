# Smart Voicing 0.2e — интеграционный тест

Цель версии: объединить Melody Harmonize с уже существующими Voice Stack / Sustain / State-механиками Router 0.2 и провести регрессию обоих режимов перед стабильной 0.3.

## Что добавляется

- `MelodyGateState` — host-neutral ownership state для физической melody note и Sustain;
- Note Off melody при нажатой педали больше не уничтожает внутренний voicing сразу: V1–V4 остаются sustain-owned;
- пока педаль удерживается, sustain-owned melody продолжает реагировать на Chord Track и может reharmonize V2–V4;
- pedal-up освобождает sustain-owned voicing, если физическая melody key уже отпущена;
- если новая melody note нажата до pedal-up, она становится новым физическим owner и не снимается отпусканием педали;
- `CC64`, Expression/CC, Pitch Bend по-прежнему маршрутизируются на Voice 1–4;
- существующие `pushNoteToVoice / clearVoiceStack` остаются единым нижним Voice Stack/Router слоем для Direct Router и Melody Harmonize;
- сохранение `HarmonyMode` и `DistributionMode` остаётся через plugin state;
- CMake mapping: `0.2e → 0.2.5`;
- Windows package / artifact: `Smart Voicing 0.2e` / `Smart-Voicing-0.2e-Windows`.

## Автоматические тесты

`SmartVoicingLiveReharmonizerTests` дополнены проверками:

1. Note Off без Sustain сразу освобождает melody ownership;
2. Note Off под Sustain сохраняет voicing;
3. pedal-up освобождает sustain-owned melody;
4. новая melody note под Sustain становится новым физическим owner и переживает pedal-up;
5. чужой Note Off не снимает активную melody;
6. все тесты 0.2b–0.2d продолжают проходить.

### CI

Основная реализация 0.2e: `2db64b3efe0a30103a9e9f1eb90cdb11933a802a`; debug UI: `4f8aa208b5e6ba93eb50797bab9d531eb2442cc3`.

Windows CI запускается на каждом push ветки; финально учитывать нужно последний run текущего HEAD. До его успешного завершения CI считается **не подтверждённым**. Версия 0.2e остаётся **В РАБОТЕ** до зелёного последнего run и пользовательского теста в Studio Pro.

## Ручной тест в Studio Pro

После зелёного Windows CI проверить оба режима.

### Melody Harmonize + Sustain

1. Включить `Melody Harmonize` и создать несколько смен Chord Track.
2. Нажать melody note и Sustain.
3. Отпустить физическую клавишу, не отпуская педаль.
4. Подтвердить, что V1–V4 продолжают звучать и V2–V4 продолжают перестраиваться при следующих chord boundaries.
5. Отпустить Sustain — все sustain-owned Voices должны корректно завершиться без stuck notes.
6. Повторить, но до pedal-up нажать новую melody note: новая нота должна остаться после pedal-up, а старые sustain-held ноты должны освободиться.

### State

1. Выбрать `Melody Harmonize` и один из Distribution Mode.
2. Сохранить проект / Show.
3. Закрыть и снова открыть проект.
4. Проверить восстановление выбранных режимов.
5. Переключиться `Melody Harmonize → Direct Router → Melody Harmonize` во время работы; между режимами не должно оставаться старых Voice.

### Direct Router 0.2 regression

Проверить:

- V1–V4 → MIDI Ch1–4;
- Chord Gesture и стабильный Voice Ownership;
- Fill 4 / Top Down / Bottom Up;
- Voice Stack / legato continuation;
- Sustain Chord Morph;
- CC / Expression / Pitch Bend;
- Note Off, CC120/123, Stop и отсутствие stuck notes.

### Harmonic regression

Проверить:

- Melody Harmonize строит базовый Close voicing;
- Live Chord Reharmonization 0.2d работает;
- sample-accurate chord boundary не регрессировал;
- `(no chord)` оставляет только V1, следующий chord возвращает V2–V4;
- slash bass сохраняется.

## Критерий прохождения 0.2e

0.2e считается подтверждённой только после зелёного Windows CI и пользовательского теста в Studio Pro, где Sustain/State работают в Melody Harmonize, а Direct Router 0.2 проходит регрессию без stuck notes.

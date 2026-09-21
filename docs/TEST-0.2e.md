# Smart Voicing 0.2e — интеграционный тест

Цель версии: объединить Melody Harmonize с уже существующими Voice Stack / Sustain / State-механиками Router 0.2 и провести регрессию обоих режимов перед стабильной 0.3.

## Что добавлено

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

## CI

Основная реализация 0.2e: `2db64b3efe0a30103a9e9f1eb90cdb11933a802a`; debug UI: `4f8aa208b5e6ba93eb50797bab9d531eb2442cc3`.

Финальный Windows Build **#226** для HEAD `ce5595b2fe8287db68fd1582664a74020d745c08` завершён успешно 2026-09-21. Configure / Build / `ctest` / Prepare versioned package / Upload artifact прошли успешно.

## Ручной тест в Studio Pro — 2026-09-21

**Статус: ПРОЙДЕН.** Пользователь подтвердил полный контрольный прогон перед переходом на 0.3.

Подтверждено:

- Melody Harmonize + live Chord Reharmonization;
- Sustain-owned melody: release физической клавиши под pedal-down не обрывает voicing;
- reharmonization V2–V4 продолжается при удержанной педали;
- pedal-up корректно освобождает sustain-owned Voices без stuck notes;
- новая melody note, сыгранная до pedal-up, остаётся активной;
- `(no chord)` fallback и возврат гармонии;
- non-chord melody сохраняет authority V1;
- slash bass работает в рамках текущего Close voicing;
- `Melody Harmonize → Direct Router → Melody Harmonize` не оставляет старых Voice;
- State восстанавливает Harmony Mode / Distribution Mode после сохранения и повторного открытия проекта;
- Direct Router 0.2 regression: V1–V4/Ch1–4, Chord Gesture, Stable Voice Ownership, Top Down / Bottom Up / Fill 4, Voice Stack / legato, Sustain Chord Morph, CC / Expression / Pitch Bend;
- Stop / Play, CC120/123 и отсутствие stuck notes;
- sample-accurate chord boundary из 0.2d не регрессировал;
- практический stress test не выявил новых проблем.

## Критерий прохождения 0.2e

0.2e считается подтверждённой после зелёного Windows CI и пользовательского теста в Studio Pro, где Sustain/State работают в Melody Harmonize, а Direct Router 0.2 проходит регрессию без stuck notes.

**Критерий выполнен. Smart Voicing 0.2e подтверждена и является базой стабильной версии 0.3.**

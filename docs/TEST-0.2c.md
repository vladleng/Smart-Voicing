# Smart Voicing 0.2c — контрольный тест

Цель версии: впервые использовать текущий Chord Track Studio Pro для реальной MIDI-гармонизации одной сыгранной melody note.

## Что добавлено

- режимы `Direct Router` и `Melody Harmonize`;
- host-neutral `CloseVoicingHarmonizer`;
- `V1` всегда сохраняет сыгранную melody note без коррекции;
- `V2–V4` строятся из текущего `NormalizedChord` ниже melody note;
- сначала используются разные chord pitch classes, затем при необходимости ближайшее удвоение;
- slash-bass при возможности закрепляется за `V4`;
- при отсутствии валидного Chord context безопасный fallback: только `V1 = melody`, `V2–V4` выключены;
- результат идёт в те же четыре Voice / MIDI Channels 1–4;
- `Direct Router` 0.2 остаётся отдельным режимом без изменения его логики;
- состояние Harmony Mode сохраняется в plugin state;
- отдельные host-neutral тесты `SmartVoicingHarmonizerTests`.

## Что сознательно не входит в 0.2c

- live reharmonization удержанной melody note при смене Chord Track — это 0.2d;
- полноценный Sustain Chord Morph / Voice Stack для сгенерированной гармонии — это 0.2e;
- voice leading между соседними voicings;
- instrument ranges;
- Drop 2 / Drop 3 и другие jazz voicings;
- использование Key как fallback-гармонизатора.

## Автоматические тесты

CI подтверждает:

1. `Cmaj7 + G4` → `G4 / E4 / C4 / B3`;
2. non-chord melody `F#4` над `Cmaj7` остаётся `F#4`, нижние Voice берутся из Cmaj7;
3. triad умеет сделать близкое удвоение при нехватке четырёх разных chord tones;
4. slash chord `C/E` помещает `E` в V4 ниже остальных Voice;
5. при отсутствии Chord context возвращается только melody Voice;
6. ChordModel tests 0.2b продолжают проходить.

Windows Build #198 завершён успешно; `SmartVoicingCoreTests` и `SmartVoicingHarmonizerTests` прошли, пакет `Smart-Voicing-0.2c-Windows` сформирован.

## Ручной тест в Studio Pro

Контрольный сценарий:

1. Установить `Smart Voicing 0.2c` и `Smart Voicing ARA` из одного пакета.
2. В Smart Voicing выбрать **Режим → Melody Harmonize**.
3. Поставить курсор/transport на участок с известным аккордом Chord Track.
4. Сыграть **одну** ноту.
5. Проверить, что:
   - `V1 / Ch1` = сыгранная нота;
   - `V2 / Ch2`, `V3 / Ch3`, `V4 / Ch4` появляются автоматически;
   - нижние Voice соответствуют текущему аккорду;
   - все четыре downstream SWAM-инструмента получают свои MIDI-ноты.
6. Проверить пример `Cmaj7 + G4`: ожидаемый базовый Close voicing `G4 / E4 / C4 / B3`.
7. Сыграть ноту, не входящую в аккорд: она должна остаться неизменной в V1.
8. Проверить slash chord, например `C/E`: V4 должен использовать E в подходящей нижней октаве.
9. На участке `(no chord)` сыграть одну ноту: должен звучать только V1, без придуманных нижних Voice.
10. Переключиться обратно в **Direct Router** и коротко убедиться, что поведение 0.2 осталось прежним.

## Результат пользовательского теста — 2026-09-20

Пользователь подтвердил основной сценарий в Studio Pro:

- режим `Melody Harmonize` включается штатно;
- сыгранная melody note гармонизируется автоматически;
- четыре голоса формируются и звучат музыкально корректно по текущему Chord Track;
- явных проблем в основном рабочем сценарии не выявлено.

Основной музыкальный MVP 0.2c подтверждён в реальном host.

Отдельные edge-case проверки `non-chord melody / slash bass / no chord fallback / Direct Router regression` остаются частью контрольного чек-листа и не считаются пользовательски подтверждёнными, пока не проверены отдельно.

## Важное ограничение теста

В 0.2c **не проверяем удержание одной melody note через смену аккорда Chord Track**. Это отдельная задача 0.2d. Также Sustain в Melody Harmonize пока только пересылается downstream; полная sustain-aware логика сгенерированных Voice будет в 0.2e.

## Критерий прохождения 0.2c

Основной Melody Harmonize MVP подтверждён CI и реальным Studio Pro тестом. Полное закрытие 0.2c — после короткой проверки оставшихся edge cases из ручного чек-листа.

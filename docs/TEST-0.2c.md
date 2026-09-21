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

## Результат пользовательского теста — 2026-09-20

Пользователь подтвердил в Studio Pro:

- режим `Melody Harmonize` включается штатно;
- сыгранная melody note автоматически гармонизируется по текущему Chord Track;
- четыре голоса формируются и по слуху звучат корректно;
- основной рабочий сценарий Melody Harmonize пригоден для дальнейшей разработки.

Отдельные edge-case сценарии (`non-chord melody / slash bass / no chord fallback / Direct Router regression`) покрыты автоматическими тестами, но не были изолированно подтверждены пользователем. Они сохраняются в регрессионном чек-листе 0.2e / 0.3 и не выдаются за отдельно пройденные ручные тесты.

## Итог

**Smart Voicing 0.2c принята и закрыта 2026-09-20.**

Ключевой критерий версии выполнен: в реальном Studio Pro одна сыгранная melody note создаёт четырёхголосную гармонизацию по текущему Chord Track, а Windows CI и оба набора Harmony Core tests проходят успешно.

Следующая итерация: `0.2d — Live Chord Reharmonization`.

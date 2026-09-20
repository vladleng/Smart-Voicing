# Smart Voicing 0.2 — итог Этапа 2: MIDI Router

## Статус

**0.2 фиксирует рабочий MIDI Router как завершённую основу для Chord-aware Harmonizer.**

Версия не добавляет новых музыкальных функций относительно подтверждённой 0.1e. Это контрольная точка после практической проверки Router в Studio Pro.

## Подтверждено в ходе разработки 0.1a–0.1e

- MIDI output Smart Voicing используется как источник downstream Instrument Tracks.
- Voice 1–4 направляются на MIDI Channels 1–4.
- Четыре SWAM-инструмента получают независимые партии через MIDI Input 1–4.
- Routed MIDI записывается на отдельные downstream-дорожки.
- CC / automation / Pitch Bend проходят downstream.
- Stable Voice Ownership сохраняет идентичность Voice при движении отдельных партий.
- Voice Stack создаёт same-channel legato / portamento gesture.
- Sustain Chord Morph позволяет взять следующий аккорд до отпускания педали.
- При pedal-up сохраняются физически удерживаемые ноты нового аккорда.
- Работают три режима Distribution: `Сверху вниз`, `Снизу вверх`, `Заполнить 4 голоса`.
- В Fill 4 одна нота может управлять четырьмя Voice через `note → Voice mask`.
- Gesture Classifier различает Chord Gesture и более поздний Voice Gesture.
- Chord Gesture ограничен максимум четырьмя независимыми Voice.
- 5+ нот аккордового жеста не создают дополнительного пятого Voice.
- ARA Context Monitor работает параллельно Router.

## Зафиксированные особенности 0.2

- Gesture Classifier использует короткое окно порядка 45 ms. Это tuning-параметр, а не окончательно закреплённое музыкальное значение.
- Voice crossing внутри стабильного Voice допускается, чтобы сохранять идентичность партии.
- MIDI Channels 1–4 являются текущим transport mapping первого reference host workflow, а не музыкальным ограничением будущего Harmony Core.
- Стандартные CC120/123 обрабатываются текущим Router path.

## Что сознательно перенесено на позднюю стабилизацию

- отдельная UI-кнопка `Panic / All Notes Off`;
- расширенный набор stop / restart / seek / plugin reactivation regression tests;
- финальная эксплуатационная полировка и stress testing перед 1.0;
- окончательный UI.

Эти пункты не блокируют следующий этап, потому что основной четырёхголосный routing workflow уже работает и записывается в Studio Pro.

## Следующий этап

**0.2a — Этап 3: Chord-aware Harmonizer + Harmonic Context.**

Следующая архитектурная цепочка:

```text
ARA / provider
      ↓
IHarmonicContextProvider
      ↓
HarmonicContext / ChordContext
      ↓
Chord-aware Harmonizer
      ↓
Voice 1–4
      ↓
существующий Router / Voice Stack
```

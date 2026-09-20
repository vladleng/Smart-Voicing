# Smart Voicing 0.1a — MIDI Router Probe

0.1a использовалась как диагностическая итерация перед первым реальным voice router.

## Что проверено в Studio Pro

- Smart Voicing принимает MIDI от контроллера.
- MIDI pass-through работает.
- Smart Voicing можно выбрать как MIDI/Event source для другой Instrument Track.
- Один MIDI output Smart Voicing можно одновременно направить на несколько destination instruments.
- CC / automation и Pitch Bend доходят до downstream instruments.
- Studio Pro предоставляет MIDI Input 1–16 для раздельной маршрутизации downstream MIDI channels.
- ARA Context Monitor продолжает работать одновременно с MIDI processing.

## Вывод

Для Этапа 2 используется схема:

```text
один VST3 Event/MIDI Output
        ↓
MIDI Channels 1–4
        ↓
Voice 1–4 / отдельные destination tracks
```

Следующая рабочая версия — `0.1b`, первый Direct 4 Voice Router.

Исходный подробный тест-план 0.1a считается выполненным в части host routing, необходимой для перехода к 0.1b.

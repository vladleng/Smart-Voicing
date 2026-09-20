# Версионирование Smart Voicing

Для проекта используется простая схема рабочих и завершённых версий.

## Принцип

- Буквенные суффиксы (`a`, `b`, `c`...) используются для промежуточных рабочих сборок внутри этапа разработки.
- После успешного завершения этапа версия повышается до следующего числового значения.
- Имя продукта и идентификаторы плагинов остаются стабильными, а номер версии используется в имени внешней папки сборки и в документации.

## История версий

- `Smart Voicing 0.0a` — базовый VST3-каркас, первая рабочая сборка. Этап 0.
- `Smart Voicing 0.0b` — первая рабочая ARA-сборка и проверка ролей Event FX / Instrument.
- `Smart Voicing 0.0c` — двухкомпонентная архитектура: основной Instrument + отдельный ARA reader.
- `Smart Voicing 0.0d` — реальные карты Chord / Key / Tempo / Time Signature через bridge.
- `Smart Voicing 0.0e` — Context Monitor и shared transport.
- `Smart Voicing 0.0f` — change-driven transport и полный интеграционный тест Studio Pro.
- `Smart Voicing 0.0g` — boundary hardening Chord / Key / Time Signature.
- `Smart Voicing 0.1` — **завершённый Этап 1: ARA Context Proof of Concept**.
- `Smart Voicing 0.1a` — MIDI Router Probe: прозрачный pass-through, диагностика MIDI и подтверждение downstream routing в Studio Pro.
- `Smart Voicing 0.1b` — первый Direct 4 Voice Router: ranked Voice 1–4 через MIDI Channels 1–4.
- `Smart Voicing 0.1c` — Stable Voice Ownership + sustain-aware state: закрепление Voice slots при движении отдельных голосов и корректная логика CC64.

## Текущая структура пакета

Номер версии указывается **не в имени VST3 bundle**, а в имени внешней папки пакета.

```text
Smart Voicing 0.1c/
├── Smart Voicing.vst3/
│   └── Contents/
│       └── ...
└── Smart Voicing ARA.vst3/
    └── Contents/
        └── ...
```

Назначение компонентов:

- `Smart Voicing.vst3` — основной Instrument / MIDI engine.
- `Smart Voicing ARA.vst3` — служебный ARA/Event FX reader гармонического контекста DAW.

Оба компонента устанавливаются вместе и считаются одной версией Smart Voicing.

## Текущий этап

После `0.1` идёт Этап 2 — MIDI Router.

Промежуточные версии Этапа 2:

- `0.1a` — MIDI Router Probe;
- `0.1b` — Direct 4 Voice Router;
- `0.1c` — Stable Voice Ownership + Sustain;
- при необходимости `0.1d`, `0.1e` и т. д.;
- после завершения этапа — `0.2`.

Далее схема повторяется аналогично:

`0.2a` → `0.2b` → ... → `0.3`

## Важное правило

Не менять `PRODUCT_NAME`, VST3 UID/manufacturer code или bundle id ради номера версии. Это нужно, чтобы DAW воспринимала обновления как те же компоненты, а не как новые продукты.

Версия должна отражаться в:

- имени внешней папки пакета сборки;
- имени CI-артефакта/архива;
- debug UI;
- PR / Issue;
- документации проекта.

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
- `Smart Voicing 0.1a` — MIDI Router Probe: pass-through, диагностика MIDI и подтверждение downstream routing.
- `Smart Voicing 0.1b` — Direct 4 Voice Router через MIDI Channels 1–4.
- `Smart Voicing 0.1c` — Stable Voice Ownership + sustain-aware state.
- `Smart Voicing 0.1d` — Voice Stack + legato continuation + Sustain Chord Morph.
- `Smart Voicing 0.1e` — Distribution Modes + Gesture Classifier + `note → Voice mask` + строгий максимум 4 Voice для chord gesture.
- `Smart Voicing 0.1f` — **планируемый Router Hardening**: VoiceOutput abstraction, Panic/reset, transport/state regression, stress tests.
- `Smart Voicing 0.2` — **планируемая финальная версия Этапа 2: MIDI Router**.

## Текущая структура пакета

Номер версии указывается **не в имени VST3 bundle**, а в имени внешней папки пакета.

```text
Smart Voicing 0.1e/
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

## Версии по этапам

```text
Этап 2: 0.1a ... 0.1f → 0.2
Этап 3: 0.2a ...      → 0.3
Этап 4: 0.3a ...      → 0.4
Этап 5: 0.4a ...      → 0.5
Этап 6: 0.5a ...      → 0.6
Этап 7: 0.6a ...      → 0.7
Этап 8: 0.7a ...      → 0.8
Этап 9: 0.8a ...      → 0.9
Этап 10: 0.9a ...     → 0.10
Этап 11: 0.10a ...    → 1.0
```

Буква последней рабочей версии внутри этапа не фиксирована заранее, кроме текущего плана 0.1f перед 0.2. Дополнительные буквенные версии добавляются только при необходимости.

## Важное правило

Не менять `PRODUCT_NAME`, VST3 UID/manufacturer code или bundle id ради номера версии. Это нужно, чтобы DAW воспринимала обновления как те же компоненты, а не как новые продукты.

Версия должна отражаться в:

- имени внешней папки пакета сборки;
- имени CI-артефакта/архива;
- debug UI;
- PR / Issue;
- документации проекта.

# Версионирование Smart Voicing

Для проекта используется простая схема рабочих и завершённых версий.

## Принцип

- Буквенные суффиксы (`a`, `b`, `c`...) используются для промежуточных рабочих сборок внутри этапа.
- После завершения этапа версия повышается до следующего числового значения.
- `PRODUCT_NAME`, VST3 UID, manufacturer code и bundle id не меняются ради номера версии.
- Версия отображается в имени пакета, CI artifact, debug UI, Issue/PR и документации.

## История версий

- `Smart Voicing 0.0a` — базовый VST3-каркас. Этап 0.
- `Smart Voicing 0.0b` — первая ARA-сборка и проверка ролей Event FX / Instrument.
- `Smart Voicing 0.0c` — двухкомпонентная архитектура Instrument + ARA reader.
- `Smart Voicing 0.0d` — карты Chord / Key / Tempo / Time Signature через bridge.
- `Smart Voicing 0.0e` — Context Monitor и shared transport.
- `Smart Voicing 0.0f` — change-driven transport и интеграционный тест Studio Pro.
- `Smart Voicing 0.0g` — boundary hardening Chord / Key / Time Signature.
- `Smart Voicing 0.1` — **завершённый Этап 1: ARA Context Proof of Concept**.
- `Smart Voicing 0.1a` — MIDI Router Probe.
- `Smart Voicing 0.1b` — Direct 4 Voice Router через MIDI Channels 1–4.
- `Smart Voicing 0.1c` — Stable Voice Ownership + sustain-aware state.
- `Smart Voicing 0.1d` — Voice Stack + legato continuation + Sustain Chord Morph.
- `Smart Voicing 0.1e` — Distribution Modes + Gesture Classifier + `note → Voice mask` + максимум 4 Voice для Chord Gesture.
- `Smart Voicing 0.2` — **завершённый Этап 2: MIDI Router**.
- `Smart Voicing 0.2a` — старт Этапа 3: host-neutral `HarmonicContext`, `IHarmonicContextProvider`, `ARAContextProvider` и контракт `VoiceOutput[4]`; пользовательский тест пройден.
- `Smart Voicing 0.2b` — нормализованная `ChordModel`: pitch class из circle-of-fifths, chord quality, extensions, alterations и slash bass; добавлены host-neutral unit tests.
- `Smart Voicing 0.2c` — первый `Melody Harmonize` + базовый Close voicing; подтверждено в Studio Pro.
- `Smart Voicing 0.2d` — Live Chord Reharmonization + sample-accurate chord boundaries без plugin latency.
- `Smart Voicing 0.2e` — интеграция Melody Harmonize с Sustain / Voice ownership / State и полная регрессия Router 0.2; подтверждено CI и пользовательским тестом в Studio Pro 2026-09-21.
- `Smart Voicing 0.3` — **завершённый Этап 3: Chord-aware Harmonizer + Harmonic Context**. Стабильный checkpoint поверх подтверждённой 0.2e без новых музыкальных изменений.

Отдельная промежуточная `0.1f` не выпускается: после успешного практического теста 0.1e этап зафиксирован напрямую как 0.2. Дополнительные hardening-функции, не блокирующие harmonizer development, перенесены на более позднюю стабилизацию.

## Текущая структура пакета

```text
Smart Voicing 0.3/
├── Smart Voicing.vst3/
│   └── Contents/
│       └── ...
└── Smart Voicing ARA.vst3/
    └── Contents/
        └── ...
```

Назначение компонентов:

- `Smart Voicing.vst3` — основной Instrument / MIDI engine и Harmony Core.
- `Smart Voicing ARA.vst3` — служебный ARA/Event FX reader harmonic context.

Оба компонента устанавливаются вместе и считаются одной версией Smart Voicing.

## Версии по этапам

```text
Этап 1: 0.0a ... 0.0g → 0.1
Этап 2: 0.1a ... 0.1e → 0.2
Этап 3: 0.2a ... 0.2e → 0.3
Этап 4: 0.3a ...      → 0.4
Этап 5: 0.4a ...      → 0.5
Этап 6: 0.5a ...      → 0.6
Этап 7: 0.6a ...      → 0.7
Этап 8: 0.7a ...      → 0.8
Этап 9: 0.8a ...      → 0.9
Этап 10: 0.9a ...     → 0.10
Этап 11: 0.10a ...    → 1.0
```

Буква последней рабочей версии внутри этапа заранее не фиксируется. Дополнительные буквенные версии добавляются только когда это реально нужно.

## CMake mapping

Рабочие буквенные версии отображаются на числовой CMake version:

```text
0.1a → 0.1.1
0.1b → 0.1.2
0.1c → 0.1.3
0.1d → 0.1.4
0.1e → 0.1.5
0.2  → 0.2.0
0.2a → 0.2.1
0.2b → 0.2.2
0.2c → 0.2.3
0.2d → 0.2.4
0.2e → 0.2.5
0.3  → 0.3.0
```

## Отложенная стабилизация

Функции, которые не блокируют развитие музыкального ядра, не должны задерживать переход между этапами. Например, отдельная UI-кнопка `Panic / All Notes Off` и финальная эксплуатационная полировка относятся к позднему этапу стабилизации перед 1.0. Интеграционная регрессия Router / Sustain / State перед стабильной 0.3 выполнена в 0.2e.

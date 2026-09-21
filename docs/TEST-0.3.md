# Smart Voicing 0.3 — стабильный checkpoint Этапа 3

Smart Voicing 0.3 фиксирует завершение **Этапа 3 — Chord-aware Harmonizer + Harmonic Context**.

0.3 не добавляет новых музыкальных алгоритмов относительно подтверждённой 0.2e. Это стабильная версия уже протестированного набора функций.

## Что входит в 0.3

- host-neutral `HarmonicContext` / `IHarmonicContextProvider`;
- `ARAContextProvider` поверх работающего shared bridge;
- нормализованная `ChordModel` с quality / extensions / alterations / slash bass;
- `Melody Harmonize`;
- базовый Close voicing;
- приоритет сыгранной melody note как V1, включая non-chord melody;
- Live Chord Reharmonization V2–V4 без нового Note On;
- anti-retrigger общих тонов в том же Voice slot;
- `(no chord)` fallback: V1 остаётся, V2–V4 выключаются и возвращаются на следующем валидном chord;
- sample-accurate scheduling chord boundary внутри audio block без lookahead и reported plugin latency;
- интеграция Melody Harmonize с Sustain / Voice ownership / общим Voice Stack;
- сохранение `HarmonyMode` и `DistributionMode` через plugin state;
- Direct Router 0.2 остаётся доступным без изменения поведения;
- Voice 1–4 продолжают маршрутизироваться через MIDI Channels 1–4.

## Подтверждённая база

Smart Voicing 0.2e прошла:

- Windows Build #226 — success;
- host-neutral automated tests;
- полный пользовательский Studio Pro regression test 2026-09-21;
- Sustain / State / Direct Router / harmonic regression / transport / stuck-note / stress tests.

Контрольный документ: `docs/TEST-0.2e.md`.

## Версия и пакет

- пользовательская версия: `Smart Voicing 0.3`;
- CMake version: `0.3.0`;
- пакет:

```text
Smart Voicing 0.3/
├── Smart Voicing.vst3/
└── Smart Voicing ARA.vst3/
```

- Windows artifact: `Smart-Voicing-0.3-Windows`.

## Критерий выпуска

Так как 0.3 меняет только номер стабильной версии, package metadata, UI/debug version text и документацию поверх уже подтверждённой 0.2e, повторный полный Studio Pro regression не требуется. Для выпуска 0.3 необходим зелёный Windows CI текущего 0.3 HEAD.

После успешного CI Stage 3 закрывается, ветка `stage-3-chord-harmonizer` вливается в `main`, а следующая разработка начинается с **Этапа 4 / Smart Voicing 0.3a — Key-aware Engine и гармонические функции**.

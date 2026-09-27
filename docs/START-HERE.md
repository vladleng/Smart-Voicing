# Smart Voicing — START HERE

**Назначение:** актуальная точка входа для следующего чата или разработчика. Фактическое состояние проверять по коду, GitHub Issue/PR и последнему CI; host-specific приёмка подтверждается отдельно от CI.

## Checkpoint

```text
Stable:          0.5 — Stage 5 Jazz Voicing Engine complete
Next:            Stage 6 / #10 — Voice Leading + Color/Rich refinement → 0.6
Reference host:  Studio Pro
Stage 5 issue:   #9
Stage 5 PR chain: #38 → #37 → #30 → main
```

Stage 5 принят по единому 32-тактовому материалу в Clean: восемь гармонических типов, 64 вертикали на тип. Drop family передаёт pitch classes выбранного Closed; Spread, Quartal, UST и Cluster имеют отдельные вертикальные цели. Unison, Octaves, Doubling — мелодические текстуры. Пользователь подтвердил работу Cluster и UST, затем предоставил повторные host-скрины исправлений `Cmaj7/E`, `Cmaj7/G`, `C13/UST` и `G7♭9` в Spread/Quartal. Точный итог, музыкальные компромиссы и критерии — [`STAGE5-CLEAN-ACCEPTANCE.md`](STAGE5-CLEAN-ACCEPTANCE.md), [`STAGE5-CLEAN-EIGHT-VOICINGS-REVIEW-2026-09-27.md`](STAGE5-CLEAN-EIGHT-VOICINGS-REVIEW-2026-09-27.md), [`TEST-0.5.md`](TEST-0.5.md).

## Архитектурная граница

```text
Stage 4: Chord + Key + Function + Resolution Target + Tension vocabulary
Stage 5: VoicingStrategy / вертикальная форма (без предыдущего аккорда)
Stage 6: Voice Leading / minimum musically necessary motion
Stage 7: Instrument Profiles / регистры, удобство и баланс
Stage 7B: deterministic Humanization + Expression Distribution
```

Clean использует написанные extensions/alterations, но не добавляет цвет ради заполнения формы. Если четыре голоса не вмещают все звуки, приоритет — ясность аккорда, guide tones и характерные альтерации. Color/Rich как быстрая окраска простых символов с учётом разрешения и движения относится к Stage 6. Правила типов — [`STAGE5-CLEAN-HARMONIZATION-RULES.md`](STAGE5-CLEAN-HARMONIZATION-RULES.md). Основной концепт — [`CONCEPT.md`](CONCEPT.md), подробный rationale Stage 5 — [`CONCEPT-STAGE5-ADDENDUM.md`](CONCEPT-STAGE5-ADDENDUM.md).

## Keyswitch map

MIDI note numbers являются контрактом; названия октав указаны по Studio Pro.

| MIDI | Studio Pro | Действие |
| --- | --- | --- |
| 32–35 | G#0–B0 | UST, Cluster, Quartal, Spread |
| 36–39 | C1–D#1 | Closed, Drop 2, Drop 3, Drop 2+4 |
| 40–42 | E1–F#1 | Unison, Octaves, Doubling |
| 43–45 | G1–A1 | Clean, Color, Rich |
| 46–47 | A#1–B1 | Direct Router, Melody Harmonize |

UI, saved project state и keyswitch меняют один state. Контрольные MIDI-ноты не проходят в записываемые партии. Sound Variations — отображение host, а не отдельная логика. Подробнее: [`STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md`](STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md).

## Следующая работа

Stage 6 / #10: previous Voice state, common tones, stepwise motion, сохранение личности каждого Voice, crossing/leap penalties и strategy-specific continuity. Color/Rich выбирают из Stage 4 candidate pool и учитывают реальный target. Низкие регистры конкретных инструментов — Stage 7. Интервальные и тембровые нюансы Drop/Quartal/Cluster, перечисленные в Clean review, можно уточнять на соответствующем этапе без переоткрытия принятой вертикальной основы.

Guardrails: MIDI 0–127, неизменная V1, не более четырёх слышимых голосов, bounded realtime search, без mutex/file I/O/dynamic allocation в audio callback. Одинаковый полный вход и состояние дают одинаковый результат. Музыкальные изменения сопровождать конкретной регрессией, затем CI и Studio Pro host check. Детали — [`MUSICAL-ENGINE-GUARDRAILS.md`](MUSICAL-ENGINE-GUARDRAILS.md) и [`CHAT-HANDOFF-RULE.md`](CHAT-HANDOFF-RULE.md).

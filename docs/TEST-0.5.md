# Smart Voicing 0.5 — Stage 5 acceptance

**Дата:** 2026-09-27. **Статус:** музыкальная приёмка Clean в Studio Pro пройдена; stable packaging/CI проверяется отдельно.

## Что принято

- Восемь гармонических типов — Closed, Drop 2, Drop 3, Drop 2+4, Spread, Quartal, UST, Cluster — проверены на одинаковых 64 вертикалях одного проекта. V1 не меняется, нижние голоса используют разрешённый аккордовый набор.
- Drop family сохраняет выбранные pitch classes Closed при октавных преобразованиях с оговорённым slash fallback. Spread остаётся открытой формой, Quartal — квартовой, UST — верхним трезвучием или допустимой частичной формой, Cluster — плотной верхней группой.
- После сводного анализа исправлена потеря корня `Cmaj7/E` и `Cmaj7/G`, потеря явной A/13 в частичном UST на `C13`, а также потеря A♭/♭9 в Spread и Quartal на `G7♭9`. Повторные скрины Studio Pro подтвердили сохранение этих звуков. У `G7♭9` на второй ноте такта 8 получено Spread F–B–A♭–G и Quartal F–B–F–A♭. Энгармоническая надпись G♯ соответствует A♭.
- Тесты Harmony Core охватывают slash и 13 для всех затронутых типов, а ♭9 для Spread/Quartal при explicit degree и generic host marker. Рабочая сборка после последнего фикса: [Windows CI run 36302920149](https://github.com/vladleng/Smart-Voicing/actions/runs/36302920149), VST3 и все CTest успешно собраны/проверены.

Полный анализ партитур и исторические неверные вертикали — [`STAGE5-CLEAN-EIGHT-VOICINGS-REVIEW-2026-09-27.md`](STAGE5-CLEAN-EIGHT-VOICINGS-REVIEW-2026-09-27.md). Правила и критерии — [`STAGE5-CLEAN-HARMONIZATION-RULES.md`](STAGE5-CLEAN-HARMONIZATION-RULES.md), [`STAGE5-CLEAN-ACCEPTANCE.md`](STAGE5-CLEAN-ACCEPTANCE.md).

## Принятые границы

- `Cmaj7/G` с G одновременно в мелодии и в басу оставляет лишь два внутренних голоса; нельзя одновременно вместить C, E и B. Вертикаль с C и E без B — четырёхголосный компромисс, а не новый функциональный аккорд.
- Quartal на `G7♭9` оставляет rootless B/F/A♭ и может располагать низкую A♭. Качество аккорда и явно написанная ♭9 теперь сохранены; инструментальный регистр проверит Stage 7.
- Выбор Color/Rich для базовых аккордов, движение между вертикалями и направленные разрешения — Stage 6 / Issue #10. Разбор непреднамеренных малых нон в чистых Drop, различимость верхней малой секунды Cluster и расширенные UST остаются адресными последующими решениями; это не меняет принятую Clean-основу.

## Стабильный пакет

Version `0.5.0`, package `Smart Voicing 0.5`, два компонента `Smart Voicing.vst3` и `Smart Voicing ARA.vst3`. Последний CI-пакет должен пройти Configure / Build / Test Harmony Core / Upload artifact на ветке Stage 5; ссылку и SHA фиксировать после зелёного прогона. Затем передать Stage 6 / #10 с этой версией как baseline.

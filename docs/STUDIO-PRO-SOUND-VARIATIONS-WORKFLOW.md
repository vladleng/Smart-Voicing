# Studio Pro — рекомендуемый Sound Variations workflow для Smart Voicing

Status: **recommended host workflow**  
Applies to: **0.4c4 fix1+**  
Reference host: **Studio Pro**

## Цель

Использовать Smart Voicing keyswitches не как набор «служебных MIDI-нот», а как понятный performance-интерфейс через **Sound Variations**.

Такой workflow даёт пользователю именованные музыкальные команды (`Closed`, `Drop 2`, `Rich` и т. д.), а Smart Voicing внутри по-прежнему получает канонические MIDI note numbers и меняет единый plugin state.

## Эталонная раскладка Sound Variations

MIDI note numbers остаются главным контрактом. Названия нот ниже соответствуют текущему Studio Pro octave naming, использованному в host-тестах.

```text
ниже основного блока — будущие / менее частые voicings
MIDI 32 / G#0  → UST       RESERVED
MIDI 33 / A0   → Cluster   RESERVED
MIDI 34 / A#0  → Quartal   RESERVED
MIDI 35 / B0   → Spread    RESERVED

основной Voicing Type блок
MIDI 36 / C1   → Closed
MIDI 37 / C#1  → Drop 2
MIDI 38 / D1   → Drop 3       RESERVED
MIDI 39 / D#1  → Drop 2+4     RESERVED
MIDI 40 / E1   → Unison
MIDI 41 / F1   → Octaves
MIDI 42 / F#1  → Doubling

Tension block — без изменений
MIDI 43 / G1   → Clean
MIDI 44 / G#1  → Color
MIDI 45 / A1   → Rich
```

## Почему это считается рекомендуемым workflow

На 49-клавишном контроллере самые часто используемые Voicing Type начинаются с `C1` и идут хроматически вверх. Сразу следом расположены Tension controls.

```text
C1    Closed
C#1   Drop 2
D1    Drop 3
D#1   Drop 2+4
E1    Unison
F1    Octaves
F#1   Doubling
G1    Clean
G#1   Color
A1    Rich
```

Это даёт одну компактную performance-зону без необходимости помнить MIDI numbers во время работы.

## Sound Variations как presentation layer

Архитектурный принцип:

```text
Studio Pro Sound Variation
        ↓
canonical keyswitch MIDI note
        ↓
Smart Voicing keyswitch decoder
        ↓
shared Voicing Type / Tension state
        ↓
UI + project state + realtime output
```

Sound Variations **не создают отдельное состояние**. Они являются только удобным host-side presentation layer над уже существующими state-параметрами Smart Voicing.

Следствия:

- изменение Sound Variation должно сразу отражаться в UI Smart Voicing;
- сохранение проекта остаётся обычным plugin-state persistence;
- те же MIDI note numbers могут использоваться и без Sound Variations в другой DAW;
- Smart Voicing не должен зависеть от Studio Pro-specific API для работы keyswitch layer.

## Reserved variations

Рекомендуется сразу создать в Sound Variations и будущие пункты:

- Drop 3;
- Drop 2+4;
- Spread;
- Quartal;
- Cluster;
- UST.

Пока соответствующий Voicing Type не реализован, его reserved keyswitch в `Melody Harmonize`:

- swallowed;
- не меняет Voicing Type;
- не доходит до downstream instrument tracks.

После реализации нового strategy существующая карта Sound Variations уже готова и не требует перенастройки.

## Практический workflow в MIDI editor

Пользователь может мыслить режимами аранжировки, а не служебными нотами:

```text
Closed + Clean
→ Drop 2 + Color
→ Drop 2 + Rich
→ Unison
→ Octaves
→ Closed + Clean
```

Voicing Type и Tension могут находиться в одной позиции. Smart Voicing должен объединить same-sample control changes в один итоговый musical refresh, чтобы не создавать промежуточную вертикаль.

## Host-side naming recommendation

В Sound Variations использовать короткие музыкальные имена без технических префиксов:

```text
UST
Cluster
Quartal
Spread
Closed
Drop 2
Drop 3
Drop 2+4
Unison
Octaves
Doubling
Clean
Color
Rich
```

Такой порядок визуально совпадает с физическим расположением keyswitches и хорошо читается в variation lane / Musical Symbols.

## Acceptance note

Эта схема зафиксирована как **рекомендуемый Studio Pro workflow** по результатам практической настройки Sound Variations пользователем 2026-09-24.

Полная host-acceptance самой версии 0.4c4 fix1 по-прежнему определяется тестами из `docs/TEST-0.4c4.md`; данный документ фиксирует пользовательский workflow и ergonomic layout, а не заменяет regression/CI acceptance.

# Studio Pro — рекомендуемый Sound Variations workflow для Smart Voicing

Status: **recommended / host-accepted workflow**  
Applies to: **0.4c4 fix2+**  
Reference host: **Studio Pro**

## Цель

Использовать Smart Voicing keyswitches не как набор служебных MIDI-нот, а как понятный performance-интерфейс через **Sound Variations**.

Такой workflow даёт пользователю именованные музыкальные команды (`Closed`, `Drop 2`, `Rich`, `Direct Router`, `Melody Harmonize` и т. д.), а Smart Voicing внутри по-прежнему получает канонические MIDI note numbers и меняет единый plugin state.

## Эталонная раскладка Sound Variations

MIDI note numbers остаются главным контрактом. Названия нот ниже соответствуют текущему Studio Pro octave naming, использованному в host-тестах.

```text
ниже основного блока — будущие / менее частые voicings
MIDI 32 / G#0  → UST       ACTIVE in 0.4h candidate
MIDI 33 / A0   → Cluster   RESERVED
MIDI 34 / A#0  → Quartal   ACTIVE in 0.4g candidate
MIDI 35 / B0   → Spread    ACTIVE

основной Voicing Type блок
MIDI 36 / C1   → Closed
MIDI 37 / C#1  → Drop 2
MIDI 38 / D1   → Drop 3
MIDI 39 / D#1  → Drop 2+4
MIDI 40 / E1   → Unison
MIDI 41 / F1   → Octaves
MIDI 42 / F#1  → Doubling

Tension block
MIDI 43 / G1   → Clean
MIDI 44 / G#1  → Color
MIDI 45 / A1   → Rich

Harmony Mode block
MIDI 46 / A#1  → Direct Router
MIDI 47 / B1   → Melody Harmonize
```

## Почему это считается рекомендуемым workflow

На 49-клавишном контроллере самые часто используемые Voicing Type начинаются с `C1` и идут хроматически вверх. Сразу следом расположены Tension controls и два выбора Harmony Mode.

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
A#1   Direct Router
B1    Melody Harmonize
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
shared Voicing Type / Tension / Harmony Mode state
        ↓
UI + project state + realtime output
```

Sound Variations **не создают отдельное состояние**. Они являются только удобным host-side presentation layer над уже существующими state-параметрами Smart Voicing.

Следствия:

- изменение Sound Variation должно отражаться в UI Smart Voicing;
- сохранение проекта остаётся обычным plugin-state persistence;
- те же MIDI note numbers могут использоваться и без Sound Variations в другой DAW;
- Smart Voicing не зависит от Studio Pro-specific API для работы keyswitch layer;
- Harmony Mode можно переключать из того же variation lane, не открывая окно плагина.

## Reserved variations

Рекомендуется сразу создать в Sound Variations и будущие пункты:

- Cluster.

Пока соответствующий Voicing Type не реализован, его reserved keyswitch в `Melody Harmonize`:

- swallowed;
- не меняет Voicing Type;
- не доходит до downstream instrument tracks.

После реализации нового strategy существующая карта Sound Variations уже готова и не требует перенастройки. `D1 / MIDI 38` был активирован как Drop 3 в 0.4d, а `D#1 / MIDI 39` активируется как Drop 2+4 в 0.4e без изменения карты.

## Harmony Mode controls

`A#1 / MIDI 46` и `B1 / MIDI 47` — глобальные mode controls:

```text
A#1 → Direct Router
B1  → Melody Harmonize
```

Они распознаются в обоих режимах, чтобы пользователь всегда мог вернуться в другой режим через Sound Variations. Их Note On/Off не должны попадать на downstream instrument tracks.

## Практический workflow в MIDI editor

Пользователь может мыслить режимами аранжировки, а не служебными нотами:

```text
Melody Harmonize
→ Closed + Clean
→ Drop 2 + Color
→ Drop 3 + Rich
→ Drop 2+4 + Rich
→ Unison
→ Octaves
→ Direct Router
→ Melody Harmonize
→ Closed + Clean
```

Voicing Type и Tension могут находиться в одной позиции. Smart Voicing объединяет same-sample control changes в один итоговый musical refresh, чтобы не создавать промежуточную вертикаль.

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
Direct Router
Melody Harmonize
```

Такой порядок визуально совпадает с физическим расположением keyswitches и хорошо читается в variation lane / Musical Symbols.

## Acceptance note

Схема зафиксирована как **рекомендуемый и практически проверенный Studio Pro workflow** по результатам настройки и host-тестов пользователя 2026-09-24.

`docs/TEST-0.4c4.md` является техническим acceptance record для keyswitch layer; данный документ фиксирует пользовательский workflow и ergonomic layout. Drop 3 и Drop 2+4 используют уже заранее зарезервированные позиции и не требуют перенастройки существующего Sound Variations набора.

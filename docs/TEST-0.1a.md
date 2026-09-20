# Smart Voicing 0.1a — тест MIDI Router Probe

Версия 0.1a не выполняет гармонизацию и не распределяет голоса. Её задача — проверить реальный MIDI-output workflow в Studio Pro перед реализацией Direct 4 Voice Router.

## Что реализовано

- основной `Smart Voicing` работает как прозрачный MIDI pass-through;
- входящий MIDI остаётся без изменения нот, velocity, каналов и CC;
- в UI добавлен `MIDI Router Probe`;
- отображаются количество MIDI-событий, Note On, Note Off, CC, Pitch Bend, прочие сообщения;
- отображаются обнаруженные MIDI-каналы 1–16 и последнее событие;
- кнопка `Сбросить MIDI stats` очищает диагностические счётчики;
- диагностика в audio thread работает по raw MIDI metadata, без создания строк и без копирования `MidiMessage`;
- ARA Context Monitor продолжает работать параллельно.

## Архитектурный вывод перед тестом

Стандартный JUCE VST3 wrapper предоставляет Smart Voicing один Event/MIDI output bus. Для нескольких независимых VST3 Event buses пришлось бы модифицировать/заменять wrapper, что сейчас неоправданно.

Поэтому базовый кандидат Этапа 2 — один Event output с разделением `Voice 1–4` по MIDI Channels 1–4. Версия 0.1a должна подтвердить, что Studio Pro удобно принимает такой output и позволяет направлять его дальше на instrument tracks.

## Тест 1 — входящий MIDI

1. Установить `Smart Voicing 0.1a` и перезапустить Studio Pro.
2. Создать Instrument Track со `Smart Voicing`.
3. Назначить обычную MIDI-клавиатуру на вход этой дорожки и включить Monitor.
4. Открыть Smart Voicing.
5. Нажать и отпустить несколько нот.

Ожидается:

- `events in/out` растут одинаково;
- растут `Note On` и `Note Off`;
- `Last` показывает последнюю ноту и MIDI channel;
- в `channels seen` появляется канал клавиатуры.

## Тест 2 — CC и Pitch Bend

1. Нажать `Сбросить MIDI stats`.
2. Покрутить Mod Wheel или другой MIDI CC.
3. Нажать Sustain pedal (CC64), если он подключён.
4. Подвигать Pitch Bend.

Ожидается:

- счётчик `CC` реагирует на контроллеры;
- Sustain отображается как `CC 64 = ...` в строке `Last`;
- `Pitch Bend` увеличивается и последнее значение меняется;
- плагин не зависает и не создаёт заметной нагрузки.

## Тест 3 — доступен ли MIDI output Smart Voicing в Studio Pro

Это главный тест 0.1a.

1. Создать вторую Instrument Track.
2. На неё загрузить любой простой звучащий инструмент.
3. Открыть выбор MIDI/Event input этой дорожки.
4. Проверить, появляется ли там `Smart Voicing` как источник событий.
5. Если Studio Pro показывает варианты по каналам (`MIDI Ch. 1–16`, `Event 1–16` или похожие), записать точное название этих пунктов.
6. Выбрать Smart Voicing как input, включить Monitor и сыграть на клавиатуре через первую дорожку.

Ожидается: второй инструмент должен получать те же ноты через Smart Voicing и звучать без двойных/зависших нот.

Если `Smart Voicing` вообще не появляется среди источников MIDI/Event input, сделать скриншот выпадающего списка. Это важный результат теста, а не просто ошибка.

## Тест 4 — проверка каналов 1–4

Нужно выяснить, сохраняет ли Studio Pro MIDI channel через Smart Voicing и можно ли использовать его для будущих Voice 1–4.

1. На source Instrument Track, которая отправляет MIDI в Smart Voicing, по очереди установить Channel 1, 2, 3 и 4, если Studio Pro позволяет выбрать output channel дорожки.
2. После каждого переключения сыграть несколько нот.
3. Смотреть `channels seen` и `Last ... | Ch N` в Smart Voicing.
4. Если destination track позволяет выбирать вход Smart Voicing отдельно по каналам/Event 1–16, проверить соответствующие четыре варианта.

Ожидается: выбранный канал должен доходить до Smart Voicing и далее до destination instrument без изменения.

Если Studio Pro не позволяет поменять канал source track или не показывает канальный фильтр на destination input, зафиксировать, какие именно варианты он предлагает.

## Тест 5 — запись MIDI после Smart Voicing

1. Включить запись на destination track, которая получает output Smart Voicing.
2. Сыграть короткую фразу.
3. Остановить запись.
4. Проверить, появился ли MIDI/Event clip на destination track.
5. Открыть clip и проверить ноты, velocity и канал.

Нам важно понять, может ли Smart Voicing быть не только live-router, но и источником записываемого редактируемого MIDI.

## Тест 6 — длинные и быстрые ноты

1. Сыграть несколько длинных нот.
2. Быстро повторять одну ноту.
3. Сыграть простой аккорд.
4. Поиграть legato.

Ожидается: destination instrument повторяет вход без пропущенных Note Off и без зависших нот.

## Тест 7 — совместная работа с ARA

1. Оставить `Smart Voicing ARA` как Event FX на ARA-якоре.
2. Проверить `CONNECTED` в основном Smart Voicing.
3. Во время MIDI pass-through двигать playhead и менять аккорды Chord Track.

Ожидается: MIDI Router Probe и Context Monitor работают одновременно; Chord / Key / Tempo / Time Signature продолжают обновляться как в 0.1.

## Что прислать после теста

Удобный формат отчёта:

```text
1 — MIDI input: OK / проблема
2 — CC / Pitch Bend: OK / проблема
3 — Smart Voicing виден как MIDI/Event source: ДА/НЕТ + как именно называется
4 — Channels 1–4: результат
5 — Запись output MIDI: ДА/НЕТ
6 — Быстрые/длинные ноты: OK / проблема
7 — ARA + MIDI одновременно: OK / проблема
```

Если пункты 3–5 проходят, в 0.1b можно переходить к первому `Direct 4 Voice Router`, используя подтверждённую Studio Pro модель routing.

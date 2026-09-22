# Smart Voicing — Tension Level

**Status:** Accepted in Smart Voicing 0.4 / Stage 4  
**Related Issues:** #8, #10, #24, #25, #28, #29  
**Reference:** Ted Pease / Ken Pullig — *Modern Jazz Voicings*  
**Functional profile contract:** `docs/FUNCTIONAL-TENSION-PROFILES.md`  
**Voice Leading contract:** `docs/VOICE-LEADING-DIRECTION.md`

`Tension Level` — степень гармонической насыщенности. Конкретный смысл tension определяется harmonic context, а не самим номером уровня.

Это UI/engine abstraction Smart Voicing, а не буквальная терминология книги.

## Final Stage 4 contract

```text
No next chord
→ no assumed resolution target

Real next chord
→ target-aware Functional Tension Profile
```

```text
Clean
→ structural chord identity

Color
→ functionally natural / inside harmonic colour for real target

Rich
→ functionally intensified tension / altered colour
```

Приоритет:

```text
Played / Melody
>
Explicit Chord Track
>
Chord identity / characteristic tones
>
Key / Function / REAL Resolution Target
>
Functional Tension Profile
>
Tension Level / Tension Policy
>
Voicing Strategy
```

Characteristic tones (`m7b5 b5`, augmented `#5`, sus identity, explicit altered fifth) защищаются; обычная perfect fifth может быть expendable.

Для confirmed `V -> minor`, `b13` может быть естественной Color-краской, natural 13 не inferred default, а `b9/#9/#11` могут быть более напряжённым Rich vocabulary. Explicit `E13`, `E7b9`, `E7#5`, `E7b5` остаются authoritative.

Studio Pro acceptance 2026-09-22 подтвердил major/minor II–V–I, `A7` vs `A7|Dm`, characteristic-tone protection и explicit alterations.

**Итог:** уровень управляет интенсивностью, а реальная progression управляет смыслом tension.

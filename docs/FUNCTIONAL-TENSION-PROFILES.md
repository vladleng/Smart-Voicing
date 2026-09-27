# Smart Voicing — Functional Tension Profiles

**Status:** Accepted in Smart Voicing 0.4 / Stage 4, refined by `0.4a fix2`.

Functional Tension Profile interprets inferred colour of a simple chord from the real harmonic turn.

```text
Current Chord + Key + Function + REAL Next Chord
→ Functional Tension Profile
→ Tension Level
→ Harmonic Candidate Pool
→ Voicing Strategy
```

Profiles:

```text
Neutral
Dominant / unresolved
Dominant -> major target
Dominant -> minor target
```

Final target rule:

```text
No next chord
→ no assumed resolution target

Real next chord on expected target root
→ confirmed target root + quality
→ target-aware profile
```

## Dominant -> minor target — refined contract

Source-derived reference: Ted Pease / Ken Pullig, *Modern Jazz Voicings*, sections on secondary-dominant chord scales and substitute-dominant chord scales.

For a confirmed ordinary `V7 -> minor` relationship:

```text
b13  → Preferred / Color, functionally directed
b9   → Contextual / Rich, functionally directed
#9   → Contextual altered candidate, NOT target-directed merely because target is minor
#11  → Contextual altered candidate, NOT target-directed merely because target is minor
b5   → do not infer from minor-target evidence alone
13   → not an automatic inferred Color for confirmed minor target
```

Important semantic distinction:

```text
same pitch class != same musical role
```

Examples:

```text
ordinary G7 -> Cm
Eb relative to G
→ inferred b13 tension

explicit G7#5
D#/Eb relative to G
→ explicit #5 chord alteration

ordinary G7 -> Cm
Db relative to G
→ do NOT promote as b5/#11 merely because the target is minor

explicit G7b5
Db relative to G
→ authoritative altered chord tone from Chord Track
```

This distinction is deliberate. `TensionPolicy` must not collapse `b13/#5` or `#11/b5` into one functional meaning just because they share a pitch class.

### Substitute dominant is a different function

A dominant resolving down by semitone as a substitute dominant has different chord-scale semantics. In the Berklee reference this is associated with Lydian b7 vocabulary, where `#11` can be a characteristic/available tension while the natural fifth remains present.

Example:

```text
Ab7 -> G7
possible interpretation: subV7/V -> V
#11 belongs to substitute-dominant colour
```

This is **not implemented as a separate Functional Tension Profile in 0.4a fix2**. It is documented as the correct future direction: substitute-dominant recognition must be added in Harmonic Interpretation / Functional Tension Profile, not guessed inside a VoicingStrategy.

## Explicit material remains authoritative

Explicit `E13`, `E7b9`, `E7#5`, `E7b5`, `B7#11` and other Chord Track alterations continue to outrank inferred functional colour. Characteristic chord tones remain protected.

Accepted Studio Pro regressions include `Dm7|G7|Cmaj7`, `Bm7b5|E7|Am`, `A7` vs `A7|Dm`, and `Dm7|Db7b13|Cm7|B7#11|Bbmaj7|A7|Dm7`.

`0.4a fix2` adds a concrete regression from Studio Pro:

```text
G7 -> Cm7
Melody: F3 (b7)
Rich target-aware result:
F3  = b7 melody
Eb3 = b13
B2  = 3
Ab2 = b9

Db/b5 must not receive a target-directed bonus from the minor target alone.
```

See `docs/CONCEPT.md`, `docs/TENSION-LEVELS.md`, `docs/MUSICAL-ENGINE-GUARDRAILS.md`, `docs/TEST-0.4a.md`, `docs/TEST-0.3f.md`, and `docs/TEST-0.4.md`.

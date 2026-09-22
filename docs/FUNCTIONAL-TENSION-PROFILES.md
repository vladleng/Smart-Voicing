# Smart Voicing — Functional Tension Profiles

**Status:** Accepted in Smart Voicing 0.4 / Stage 4.

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

For `V→minor`, b13 can be a Preferred/Color candidate, b9/#9/#11 can be stronger Rich candidates, and natural 13 is not an automatic inferred Color. Explicit `E13`, `E7b9`, `E7#5`, `E7b5` remain authoritative. Characteristic chord tones are protected.

Accepted Studio Pro regressions: `Dm7|G7|Cmaj7`, `Bm7b5|E7|Am`, `A7` vs `A7|Dm`, and `Dm7|Db7b13|Cm7|B7#11|Bbmaj7|A7|Dm7`.

See `docs/CONCEPT.md`, `docs/TENSION-LEVELS.md`, `docs/TEST-0.3f.md`, and `docs/TEST-0.4.md`.

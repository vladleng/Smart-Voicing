# Smart Voicing 0.3c — Resolution-aware Harmonic Function

## Status

**ACCEPTED / Studio Pro confirmed — 2026-09-21**

0.3c adds resolution-aware harmonic function, modal-interchange evidence, live `ClosedVoicingContext`, and exact realtime Chord Track boundary handling.

## Core behavior

```text
Current Chord + Key
        ↓
Static Harmonic Analysis
        +
Next Chord Track event
        ↓
Resolution Evidence
        ↓
Candidate / Confirmed
        ↓
ClosedVoicingContext
        ↓
Closed Engine
```

Priority remains:

```text
Played Melody > Explicit Chord > Key > Function > musical policy
```

## Confirmed harmonic cases

```text
C major: D7 -> G7    = V/V candidate + CONFIRMED
C major: D7 -> Am7   = V/V candidate, NOT CONFIRMED
C major: C7 -> Fmaj7 = V/IV candidate + CONFIRMED
C major: Fm7         = modal interchange candidate: parallel minor
```

Primary dominant remains distinct from applied dominant.

## Live integration

`Melody Harmonize` uses:

```text
Chord + Key + Next Chord
        ↓
HarmonicAnalysis
        ↓
ClosedVoicingContext
        ↓
buildClosedVoicing(...)
```

The engine receives Function, relation, Applied Dominant Candidate/Confirmed, and Modal Interchange Candidate. Tension selection differences belong to 0.3d.

## Closed regression

The accepted 0.3b Closed behavior remains intact, including:

```text
Cmaj7 + C -> C-B-G-E
Cmaj7 + D -> D-B-G-E
```

V1 remains performer-owned melody.

## Realtime boundary fix

Initial full Studio Pro testing exposed micro transient generated notes around chord boundaries because the stopped-cursor/UI PPQ tolerance was also influencing realtime context lookup.

From fix `e7009da`:

```text
Transport PLAYING
→ exact ARA chord boundary + numerical epsilon only

Transport STOPPED
→ small UI/cursor tolerance retained
```

Final Studio Pro test confirmed:

```text
melody Note On exactly at chord boundary
→ one clean voicing transition
→ no micro transient generated notes
```

If melody genuinely starts earlier than the next chord:

```text
new melody + old current chord
→ chord boundary
→ reharmonization
```

This is correct timeline behavior, not a bug.

## CI / acceptance

- Windows Build #259 — success for boundary-fix line;
- host-neutral Harmony Core tests — pass;
- Studio Pro diagnostics — pass;
- exact-boundary generated MIDI test — pass;
- 0.3b Closed regression — pass.

Final documentation-only HEAD may trigger another CI run; the current HEAD must also be green before starting 0.3d under the project workflow.

## Out of scope for 0.3c

- Tension Policy / Harmonic Candidate Pool — 0.3d;
- melodic approach-note analysis — future Melodic Context Engine;
- Voice Leading — Stage 6;
- full borrowed-chord / deceptive-resolution taxonomy.

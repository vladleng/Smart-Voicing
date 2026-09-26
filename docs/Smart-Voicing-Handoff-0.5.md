# Smart Voicing — Handoff 0.5

## 1. Current state

```text
Stable version:             0.4 — Stage 4 complete
Current development:        0.4f — Spread host candidate, Build #436 green; Studio Pro pending
Current Stage:              Stage 5 — Jazz Voicing Engine → 0.5
Working branch:             stage-5-jazz-voicing-engine
Main Issue:                 #9
Current PR:                 #30 (draft/open, tracks Stage 5)
Reference host:             Studio Pro
Implementation baseline:    69406d2881525652e8c70b5a8bbaf510ca9cbdd3
Latest verified CI:         Windows Build #421 / run 35959470042 — success
Host artifact:              Smart-Voicing-0.4e-Windows
```

GitHub remains source of factual truth. Documentation-only commits may be newer than the implementation baseline above.

## 2. Confirmed

### Implemented

- ARA → shared harmonic context → Instrument architecture;
- Chord / Key / Tempo / Time Signature / transport;
- Direct Router, four independent voices Ch1–Ch4, Voice Stack, Sustain, ownership/state;
- ChordModel + slash bass;
- Melody Harmonize;
- KeyModel + harmonic function analysis;
- applied/secondary dominant candidate + real-resolution confirmation;
- modal-interchange candidate MVP;
- exact realtime chord boundaries;
- TensionPolicy + Harmonic Candidate Pool;
- Functional Tension Profiles;
- characteristic-tone protection;
- common `VoicingStrategy` architecture;
- Closed, Drop 2, Drop 3, Drop 2+4;
- Unison, Octaves, deterministic Doubling;
- persistent Voicing Type;
- Tension keyswitches;
- Voicing Type keyswitches;
- Harmony Mode keyswitches;
- Studio Pro Sound Variations workflow.

### CI confirmed

Current 0.4e implementation: Windows Build #421 — green.

CI includes VST3 + ARA VST3 build, strategy regressions, keyswitch regressions, Harmony Core tests and package upload.

### Studio Pro confirmed

Accepted Stage 5 slices:

```text
0.4a  Foundation
0.4b  Drop 2
0.4c1 Unison
0.4c2 Octaves
0.4c3 Doubling
0.4c4 Performance Keyswitch Layer
0.4d  Drop 3
```

Host-confirmed workflow includes Voicing/Tension/Harmony Mode controls through Studio Pro Sound Variations.

**0.4e Drop 2+4 was accepted by the user after the Studio Pro test on 2026-09-26.**

## 3. Accepted 0.4e and current work 0.4f

Musical transform:

```text
Closed [V1, V2, V3, V4]
V2 → V2 - 12
V4 → V4 - 12
V1 unchanged
V2..V4 sorted by actual sounding pitch
```

Rules:

- Drop 2+4 changes register/shape only;
- it does not choose new tensions or chord tones;
- exact Closed pitch-class multiset is preserved;
- V1 melody remains authoritative;
- incomplete four-voice material or MIDI underflow falls back safely;
- explicit slash bass remains authoritative because original Closed V4 is itself octave-displaced.

Implemented:

- `VoicingType::drop24` appended without renumbering previous values;
- pure strategy transform + dispatcher;
- automated regressions;
- processor/state integration;
- UI integration;
- `D#1 / MIDI 39` keyswitch activation;
- keyswitch regression;
- `Smart-Voicing-0.4e-Windows` package;
- Build #421 green.

Host result: user reported the 0.4e test passed in Studio Pro on 2026-09-26. See `docs/TEST-0.4e.md`.

0.4f Spread core is an independent bottom-up strategy. V1 remains the performer melody; V4 is a root/slash-bass anchor; inner voices come from the Stage 4 candidate pool. Core Build #435 and full host Build #436 are green. Host integration activates `B0 / MIDI 35` without remapping other controls; Studio Pro acceptance is pending. See `docs/TEST-0.4f.md` and `docs/0.4f-HOST-CHECKLIST.md`.

## 4. Stable performance-control map

Canonical MIDI note numbers:

```text
32 G#0  UST             RESERVED
33 A0   Cluster         RESERVED
34 A#0  Quartal         RESERVED
35 B0   Spread          ACTIVE in 0.4f host candidate
36 C1   Closed
37 C#1  Drop 2
38 D1   Drop 3
39 D#1  Drop 2+4
40 E1   Unison
41 F1   Octaves
42 F#1  Doubling
43 G1   Clean
44 G#1  Color
45 A1   Rich
46 A#1  Direct Router
47 B1   Melody Harmonize
```

Sound Variations is a host-side presentation layer only. UI/project/keyswitch must always mutate the same shared states.

## 5. Architecture decisions that must survive future work

### Layer ownership

```text
Stage 4 = harmonic meaning / target-aware pitch vocabulary
Stage 5 = vertical organization / Voicing Strategy
Stage 6 = continuity / Voice Leading / stable Voice identity
Stage 7 = Instrument / Ensemble ranges and desirable register
Stage 7B = Performance Humanization / Expression Distribution
Stage 8 = saved Performance Configurations
```

Never fix a Stage 4 problem inside Stage 5, a Stage 5 problem inside Stage 7, etc.

Core rule:

> Сначала определить, к какому слою относится музыкальная проблема, и исправлять её только в этом слое.

### Drop-family rule

```text
Stage 4 chooses valid pitch vocabulary
→ Closed chooses a concrete vertical
→ Drop 2 / Drop 3 / Drop 2+4 transform that same material
```

No independent Function/Tension reinterpretation inside Drop strategies.

### Melodic textures

Unison / Octaves / Doubling are orchestration strategies over performer-owned melody. They may intentionally contain no harmonic information and therefore Clean/Color/Rich may be pitch-identical in those modes.

### Determinism

```text
same full input + same state = same output
```

No hidden randomness or time-based tie-breaks. Future Humanize requires an explicit/saved Seed.

### Realtime

No mutex, file I/O or dynamic allocation in realtime audio path.

## 6. Stage 4 harmonic contract

Priority:

```text
Played / Melody
> Explicit Current Chord
> Chord identity / characteristic tones
> Current Key
> Harmonic Function + REAL Resolution Target
> Functional Tension Profile / Tension Level
> Melodic Role / Tension Policy
> Voicing Strategy
> Voice Leading
> Instrument Profile
```

Important accepted semantics:

- no real next chord → no assumed target;
- unresolved dominant stays neutral/natural unless explicit context says otherwise;
- confirmed `V7 → minor`: b13 is strongly target-aware; b9 contextual/strong; #9 and #11 are not automatically target-derived only because the target is minor;
- explicit Chord Track alterations always win;
- characteristic `m7b5 b5` is protected;
- ordinary P5 is often expendable;
- melodic tension and harmonic tension are different roles;
- avoid-as-harmony does not ban a melody note;
- minor ninth is a strong soft negative, not a universal hard ban.

## 7. Known limitations / intentional deferrals

- no full previous-state Voice Leading yet — Stage 6;
- no temporal Melodic Context Engine/approach-note classification yet;
- no instrument-specific range adaptation yet — Stage 7;
- no Performance Humanization yet — Stage 7B;
- Spread core and host integration implemented; Studio Pro acceptance pending. Quartal / Cluster / UST remain unimplemented;
- UST/Cluster/Quartal keyswitch notes are reserved and swallowed in Melody Harmonize;
- host octave labels are convenience labels only; MIDI note numbers are the contract.

## 8. Future Performance Engine decision

Tracking: Issue #31 and `docs/PERFORMANCE-ENGINE-ROADMAP.md`.

Placement:

```text
Stage 6 Voice Leading
→ Stage 7 Instrument Profiles
→ Stage 7B Performance Engine
→ Stage 8 Presets
```

Expression processing order is fixed conceptually as:

```text
Base Expression automation
→ Expression Gain/Trim V1..V4
→ gain-adjusted per-Voice baseline
→ Expression Humanize / response / smoothing
→ Instrument Profile mapping
→ Ch1..Ch4
```

Four Expression Gains define stable section balance; Humanize is variation around each already adjusted Voice baseline. A quieter Baritone stays quieter after Humanize. Initial timing Humanize is delay-only to avoid lookahead/latency.

## 9. Relevant docs / Issues

Start with:

- `docs/START-HERE.md`;
- `docs/CONCEPT.md`;
- `docs/MUSICAL-ENGINE-GUARDRAILS.md`;
- `docs/TEST-0.4e.md`;
- `docs/0.4e-HOST-CHECKLIST.md`;
- `docs/STUDIO-PRO-SOUND-VARIATIONS-WORKFLOW.md`;
- `docs/PERFORMANCE-ENGINE-ROADMAP.md`;
- `docs/VOICE-LEADING-DIRECTION.md`;
- Issue #9 — Stage 5;
- Issue #31 — future Performance Engine;
- PR #30 — Stage 5 working PR.

Primary literature reference: Ted Pease / Ken Pullig — *Modern Jazz Voicings: Arranging for Small and Medium Ensembles*. GitHub stores only the formalized musical conclusions relevant to the engine.

## 10. NEXT ACTION

```text
1. Test `Smart-Voicing-0.4f-Windows` from Build #436 in Studio Pro using `docs/0.4f-HOST-CHECKLIST.md`.
2. Record any host bugs and repair if needed; otherwise mark 0.4f accepted in TEST/Issue/PR.
3. Continue with the next Stage 5 strategy only after that acceptance.
```

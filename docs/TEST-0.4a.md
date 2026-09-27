# Smart Voicing 0.4a — Stage 5 foundation acceptance

Status: **ACCEPTED / COMPLETED**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4 / Stage 4**

Accepted build label: **Smart Voicing 0.4a**

Acceptance date: **2026-09-22**

Next development slice: **0.4b — Drop 2**

> **Development guardrail:** before changing Harmony Core, Voicing Strategy, Voice Leading or MIDI transition semantics, read `docs/MUSICAL-ENGINE-GUARDRAILS.md`. Small musical fixes must stay inside the owning layer and receive regression coverage.

## Acceptance summary

0.4a is accepted as the completed **Stage 5 foundation** checkpoint.

The plug-in is now architecturally inside Stage 5 while `Voicing Type = Closed` remains the only implemented voicing strategy. The accepted foundation includes the common `VoicingStrategy` contract, persistent `Voicing Type`, realtime `Clean / Color / Rich` keyswitches, collapsible Diagnostics, melody-transition hygiene and the `0.4a fix2` correction for dominant-to-minor tension semantics.

Studio Pro acceptance confirmed the musical behaviour of `Clean / Color / Rich` across ordinary major harmony, major and minor II–V–I, unresolved dominants, confirmed `V7 -> minor`, characteristic `m7b5 b5`, explicit `#11`, slash basses and non-chord melody. Tension keyswitches were confirmed to affect the shared UI/state correctly and remain only on the Smart Voicing control track: they do **not** appear on downstream instrument tracks.

The final package name is:

```text
Smart Voicing 0.4a
Smart-Voicing-0.4a-Windows
```

The Stage 5 PR and Issue remain open because Stage 5 continues with 0.4b and later slices through 0.5.

## Scope completed for 0.4a

- [x] start `stage-5-jazz-voicing-engine` from stable `main / 0.4`;
- [x] internal CMake version `0.4.1`;
- [x] add common `VoicingType` / `VoicingContext` / `buildVoicing()` contract;
- [x] keep `Closed` as the first/default Stage 5 strategy;
- [x] route Melody Harmonize through the common Stage 5 dispatcher;
- [x] save/load `Voicing Type` in plugin state, with 0.4 projects defaulting to `Closed`;
- [x] add `Voicing Type` UI control;
- [x] add realtime Tension Level keyswitches (`Clean / Color / Rich`);
- [x] swallow Tension keyswitch notes so they never reach downstream instruments;
- [x] live Tension switching re-harmonizes the held melody through the existing lower-voice transition path;
- [x] make `Diagnostics` collapsible and reduce editor height in compact mode;
- [x] preserve Stage 4 harmonic semantics in the Stage 5 input contract;
- [x] add `0.4a fix` seamless melody-transition path as preventative MIDI-transition hygiene;
- [x] add `0.4a fix2` minor-target dominant tension-role correction;
- [x] confirm Closed musical output in Studio Pro for the accepted 0.4a scope.

## 0.4a fix — Seamless Melody Transition / MIDI note hygiene

### Investigation result for the originally reported micro-notes

The screenshot that triggered this investigation was **not a Harmony Core bug**.

The source melody note started slightly before the next Chord Track boundary. Therefore the engine correctly saw the previous chord first, built a voicing for that still-current chord, and then reharmonized V2–V4 when the real next chord boundary arrived.

```text
Melody starts before chord boundary
→ previous chord is still authoritative
→ previous-chord voicing is valid

Real Chord Track boundary arrives
→ new chord becomes authoritative
→ V2–V4 reharmonize
```

Do **not** "fix" this by silently adding chord lookahead, anticipation tolerance or snapping an early melody note to a future chord. If such behaviour is wanted for live performance, it must be designed as a separate musical feature with its own contract.

### Why `0.4a fix` remains in the code

Although the original screenshot had a different cause, the investigation exposed an independent transition weakness worth fixing preventively:

- the old Melody Harmonize path stopped the whole current voicing when a new melody Note On arrived;
- adjacent quantized notes can place old Note Off and new Note On at exactly the same sample;
- unchanged generated tones could therefore receive an unnecessary `Note Off` + `Note On` pair at the same timestamp;
- a DAW recording the MIDI output can materialize such redundant pairs as tiny fragments or unnecessary rearticulations.

Rules implemented in `0.4a fix`:

- MIDI note events at one sample position are treated as one melody event group;
- Note On wins over Note Off inside the same sample group, so host ordering does not create a transient silence;
- a new melody articulation computes one new Stage 5 voicing and diffs it against the currently sounding voicing;
- V1 follows/rearticulates the performed melody;
- unchanged V2–V4 common tones remain continuously sounding and are not retriggered;
- only voices whose actual MIDI note changes receive Note Off / Note On;
- repeated same-pitch melody notes still rearticulate V1, while unchanged lower voices remain continuous;
- a Tension Level keyswitch at the same sample is folded into the same musical decision instead of creating an intermediate reharmonization;
- a melody event at sample 0 suppresses the redundant block-start reharmonization pass so a chord/melody boundary does not create an avoidable intermediate voicing.

Automated `0.4a fix` regression verifies:

- [x] melody change can move V1/V2 while common V3/V4 receive no MIDI transition;
- [x] repeated same-pitch melody can retrigger V1 without retriggering V2–V4;
- [x] existing Chord Track lower-voice reharmonization behaviour remains intact.

The detailed host transition cases below remain useful as continuing regression scenarios for future Stage 5/6 changes, but are no longer acceptance blockers for the already accepted 0.4a checkpoint:

- exact adjacent melody seams;
- repeated same-pitch melody articulation;
- melody boundary exactly coincident with a Chord Track boundary;
- intentionally early melody before a Chord Track boundary.

## 0.4a fix2 — Dominant -> minor tension semantics

### Reason for the fix

Studio Pro test case:

```text
G7 -> Cm7
Melody: F3 (b7)
Tension Level: Rich
```

The previous policy treated the relative semitone-6 pitch class (`Db` over G) as `#11/b5` and marked it `functionallyDirected` merely because a minor target was confirmed. That allowed Closed scoring to prefer:

```text
F3  = b7 melody
Db3 = b5/#11-class pitch
B2  = 3
Ab2 = b9
```

This does **not** match the accepted functional interpretation for an ordinary dominant resolving to minor.

Source-derived rule from *Modern Jazz Voicings*:

- ordinary `V7 -> minor` naturally supports **b13** as target-aware colour;
- **b9** is a stronger target-directed minor-dominant tension;
- `#9` and `#11/b5` remain contextual altered material but are **not** promoted merely because the target is minor;
- explicit altered chord symbols remain authoritative;
- substitute-dominant `#11` semantics belong to a different functional profile (future work), not to ordinary `V7 -> minor`.

### Implemented and accepted 0.4a fix2 contract

```text
Confirmed V7 -> minor

b13  → Preferred / Color, functionallyDirected = true
b9   → Contextual / Rich, functionallyDirected = true
#9   → Contextual / Rich candidate, functionallyDirected = false
#11  → Contextual / Rich candidate, functionallyDirected = false
b5   → never inferred solely from minor-target evidence
13   → not automatically inferred for confirmed minor target
```

Pitch-class semantics are kept separate from role semantics:

```text
relative 8:
- inferred V7 -> minor = b13 tension
- explicit V7#5 = #5 chord alteration

relative 6:
- ordinary V7 -> minor = no target-directed bonus
- explicit V7b5 = authoritative altered chord tone
- future substitute dominant may interpret the same class as #11/Lydian b7 colour
```

Automated regression verifies:

- [x] `E7 -> Am`: b9 remains target-directed Rich;
- [x] `E7 -> Am`: b13 remains Preferred/Color and target-directed;
- [x] `E7 -> Am`: #9 is contextual but not target-directed from minor target alone;
- [x] `E7 -> Am`: #11/b5-class pitch is contextual but not target-directed from minor target alone;
- [x] explicit `E7b5` remains authoritative even at Clean;
- [x] `A7 -> Dm` keeps the same b9/b13 semantics;
- [x] concrete Closed regression `G7 -> Cm7`, melody F3, Rich produces:

```text
V1 F3  = b7 melody
V2 Eb3 = b13
V3 B2  = 3
V4 Ab2 = b9
```

and does not select `Db/b5` as a target-directed substitute for `Eb/b13`.

Studio Pro acceptance confirmed the same musical result in the real plug-in and additionally verified that unresolved dominants can retain natural dominant colour rather than being forced into target-derived alteration.

### Deferred, not part of 0.4a

A separate **Substitute Dominant -> Target** functional profile is still needed in future Harmony Interpretation. Example:

```text
Ab7 -> G7
```

can be interpreted as substitute dominant and should eventually receive its own Lydian-b7 / #11 semantics. This must be implemented in Harmonic Interpretation / Functional Tension Profile, not guessed inside Closed/Drop/Spread strategies.

See `docs/FUNCTIONAL-TENSION-PROFILES.md`.

## Tension Level keyswitch contract

Stable MIDI note numbers for 0.4a:

```text
MIDI 43 → Clean  (Studio Pro label: G1)
MIDI 44 → Color  (Studio Pro label: G#1)
MIDI 45 → Rich   (Studio Pro label: A1)
```

The MIDI note numbers are authoritative. Other DAWs may display different octave labels for the same physical notes.

Rules implemented and accepted in 0.4a:

- [x] the keyswitch changes the same `TensionLevel` state used by UI and project state;
- [x] no separate keyswitch-only tension state exists;
- [x] note-on and note-off for MIDI 43/44/45 are swallowed in `Melody Harmonize`;
- [x] when a melody is already held, a keyswitch recomputes the Stage 5 voicing and applies only the required lower-voice transition;
- [x] V1 remains performer-owned and is not retriggered by a Tension Level switch;
- [x] repeated realtime switching produced no observed stuck-note problem in Studio Pro;
- [x] keyswitch events remain on the Smart Voicing control track and are not recorded to downstream instrument tracks.

## Architectural regression

The new dispatcher must not change Stage 4 interpretation. Strategy input is already interpreted harmonic material:

```text
Chord
+ Key
+ Harmonic Function
+ Resolution Target
+ Functional Tension Profile
+ Tension Level / Policy
+ explicit / characteristic evidence
        ↓
VoicingContext
        ↓
buildVoicing()
        ↓
VoicingStrategy
```

For 0.4a the only strategy is `Closed`.

Automated regression compares direct `buildClosedVoicing()` against `buildVoicing(..., VoicingType::closed, ...)` for:

- [x] ordinary Cmaj7 Closed voicing;
- [x] target-aware G7 → Cmaj7 with `Color`;
- [x] target-aware E7 → Am with `Rich`;
- [x] `voicingTypeName(Closed)`.

Automated keyswitch regression verifies:

- [x] MIDI 43 / 44 / 45 are the stable Clean / Color / Rich map;
- [x] all three notes decode to the existing `TensionLevel` enum;
- [x] unrelated MIDI notes are ignored without changing the current level.

## Studio Pro musical acceptance completed

Confirmed during 0.4a acceptance:

- [x] ordinary `Cmaj7` across Clean / Color / Rich;
- [x] `Dm7 | G7 | Cmaj7` major II–V–I remains musically correct;
- [x] `Bm7b5 | E7 | Am` preserves the characteristic `m7b5 b5` and target-aware minor-dominant colour;
- [x] `G7 -> Cm7`, melody F3, Rich gives `F3 / Eb3 / B2 / Ab2` (`b7 / b13 / 3 / b9`) rather than `Db/b5` from minor-target inference;
- [x] explicit `#11` chord material remains authoritative in real host output;
- [x] explicit tensions remain authoritative in tested material;
- [x] slash bass remains authoritative (`Cmaj7/E`, `G7/B`) across Clean / Color / Rich;
- [x] non-chord melody remains V1 and is not rewritten;
- [x] unresolved dominant behaviour remains conservative/natural when there is no confirmed target;
- [x] UI and keyswitch use one shared `TensionLevel` state;
- [x] MIDI 43 / 44 / 45 switch Clean / Color / Rich correctly;
- [x] keyswitch notes do not appear on downstream instrument tracks;
- [x] repeated tension switching produced no observed stuck-note problem.

Additional host regression scenarios retained for future passes rather than used as blockers for 0.4a acceptance:

- loading a dedicated legacy 0.4 project and explicitly observing the `Voicing Type = Closed` migration path;
- exhaustive exact-sample seam inspection on recorded downstream MIDI;
- explicit host examples for every altered-fifth spelling (`b5`, `#5`, `#11`) separately;
- repeated compact/expanded Diagnostics UI regression.

These remain useful safety checks as Stage 5 grows, but the user accepted the 0.4a foundation after the musical and routing checks above.

## CI status at acceptance

- Windows Build #327: **success** after the final `fix2` scoring correction.
- Windows Build #328: **success** on the subsequent deterministic-output documentation checkpoint.
- The final `Smart Voicing 0.4a` package-name commit must also pass the same Windows build/test/package workflow before the final artifact is treated as CI-verified.

## Acceptance boundary — CLOSED

0.4a is accepted as completed.

The accepted checkpoint is:

```text
Stage 4 harmonic interpretation
        ↓
shared VoicingContext
        ↓
VoicingStrategy dispatcher
        ↓
Closed
```

with one persistent `Voicing Type` state, shared `Clean / Color / Rich` control, realtime tension keyswitches, collapsible Diagnostics, deterministic musical output, transition hygiene and the accepted dominant-to-minor tension semantics from `fix2`.

No additional voicing algorithm belongs to 0.4a.

**Next:** `0.4b — Drop 2`, implemented as a transformation of the already selected Closed harmonic material without re-guessing function or tension vocabulary.

# Smart Voicing 0.4a — Stage 5 foundation test plan

Status: **IN DEVELOPMENT**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4 / Stage 4**

## Scope approved for 0.4a

- [x] start `stage-5-jazz-voicing-engine` from stable `main / 0.4`;
- [x] internal CMake version `0.4.1`, package name `Smart Voicing 0.4a`;
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
- [ ] confirm Closed musical output against stable 0.4 in Studio Pro.

## Tension Level keyswitch contract

Stable MIDI note numbers for 0.4a:

```text
MIDI 43 → Clean  (Studio Pro label: G1)
MIDI 44 → Color  (Studio Pro label: G#1)
MIDI 45 → Rich   (Studio Pro label: A1)
```

The MIDI note numbers are authoritative. Other DAWs may display different octave labels for the same physical notes.

Rules implemented in 0.4a:

- the keyswitch changes the same `TensionLevel` state used by UI and project state;
- no separate keyswitch-only tension state exists;
- note-on and note-off for MIDI 43/44/45 are swallowed in `Melody Harmonize`;
- when a melody is already held, a keyswitch immediately recomputes the Stage 5 voicing and applies only the lower-voice transition plan;
- V1 remains performer-owned and is not retriggered by a Tension Level switch.

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

## Host regression required before 0.4a acceptance

In Studio Pro verify:

- [ ] existing 0.4 project opens with `Voicing Type = Closed`;
- [ ] Dm7 | G7 | Cmaj7 sounds the same as 0.4 with the same Tension Level;
- [ ] Bm7b5 | E7 | Am preserves m7b5 characteristic b5 and target-aware dominant colour;
- [ ] explicit tensions remain authoritative;
- [ ] slash bass remains authoritative;
- [ ] non-chord melody remains V1 and is not rewritten;
- [ ] live Chord Track changes update V2–V4 without retriggering V1 unnecessarily;
- [ ] UI `Clean / Color / Rich` still uses one shared `TensionLevel` state;
- [ ] MIDI 43 / 44 / 45 switch `Clean / Color / Rich` and the UI follows the same state;
- [ ] keyswitch notes do not appear in downstream MIDI;
- [ ] repeated tension switching creates no stuck notes;
- [ ] collapsed Diagnostics substantially reduces plug-in window height;
- [ ] collapsed/expanded Diagnostics does not change engine behavior or diagnostic data collection.

## Acceptance boundary

0.4a is accepted only when the plug-in is architecturally Stage 5 while `Voicing Type = Closed` remains musically compatible with stable 0.4. New voicing algorithms such as Drop 2 are explicitly deferred to 0.4b.

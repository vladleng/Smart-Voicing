# Smart Voicing 0.4a — Stage 5 foundation test plan

Status: **IN DEVELOPMENT**

Stage: **5 — Jazz Voicing Engine**

Stable base: **0.4 / Stage 4**

## Scope approved for 0.4a

- [x] start `stage-5-jazz-voicing-engine` from stable `main / 0.4`;
- [x] internal CMake version `0.4.1`, package name `Smart Voicing 0.4a`;
- [x] add common `VoicingType` / `VoicingContext` / `buildVoicing()` contract;
- [x] keep `Closed` as the first/default Stage 5 strategy;
- [ ] route Melody Harmonize through the common Stage 5 dispatcher;
- [ ] save/load `Voicing Type` in plugin state, with 0.4 projects defaulting to `Closed`;
- [ ] add `Voicing Type` UI control;
- [ ] add realtime Tension Level keyswitches (`Clean / Color / Rich`);
- [ ] swallow Tension keyswitch notes so they never reach downstream instruments;
- [ ] live Tension switching re-harmonizes safely without stuck notes;
- [ ] make `Diagnostics` collapsible and reduce editor height in compact mode;
- [ ] preserve Stage 4 harmonic semantics and Closed musical output.

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
- [ ] keyswitch `Clean / Color / Rich` changes the same state and UI follows it;
- [ ] keyswitch notes do not appear in downstream MIDI;
- [ ] repeated tension switching creates no stuck notes;
- [ ] collapsed Diagnostics substantially reduces plug-in window height;
- [ ] collapsed/expanded Diagnostics does not change engine behavior or diagnostic data collection.

## Acceptance boundary

0.4a is accepted only when the plug-in is architecturally Stage 5 while `Voicing Type = Closed` remains musically compatible with stable 0.4. New voicing algorithms such as Drop 2 are explicitly deferred to 0.4b.

# Smart Voicing 0.4c4 — Performance Keyswitch Layer

Status: **ACCEPTED / COMPLETED**

Stage: **5 — Jazz Voicing Engine**

Accepted in Studio Pro: **2026-09-24**

Stable musical base: **0.4c3 — Unison / Octaves / Doubling accepted**

## Result

0.4c4 completes the first practical performance-control layer for Stage 5. The user can select Voicing Type, Tension Level and Harmony Mode from one compact Sound Variations / keyswitch zone while all controls continue to use the same plugin state as the UI and project persistence.

Canonical MIDI note numbers are authoritative. Studio Pro note names below follow the octave convention used during host acceptance.

```text
FUTURE / MODERN VOICINGS
32  G#0  UST         RESERVED
33  A0   Cluster     RESERVED
34  A#0  Quartal     RESERVED
35  B0   Spread      RESERVED

PRIMARY VOICING TYPE
36  C1   Closed
37  C#1  Drop 2
38  D1   Drop 3      RESERVED until 0.4d
39  D#1  Drop 2+4    RESERVED
40  E1   Unison
41  F1   Octaves
42  F#1  Doubling

TENSION LEVEL
43  G1   Clean
44  G#1  Color
45  A1   Rich

HARMONY MODE
46  A#1  Direct Router
47  B1   Melody Harmonize
```

## Accepted architecture

```text
Studio Pro Sound Variation / MIDI keyswitch
                    ↓
              canonical MIDI note
                    ↓
           Smart Voicing decoder
                    ↓
      shared plugin state (no duplicate state)
                    ↓
     UI + project persistence + realtime engine
```

### Voicing Type

- `36 Closed`, `37 Drop 2`, `40 Unison`, `41 Octaves`, `42 Doubling` select the shared `VoicingType` state.
- `32..35`, `38..39` are reserved future slots and remain swallowed/inert in Melody Harmonize until implemented.
- Voicing Type switching while melody is held uses the existing transition path rather than a second rebuild engine.
- V1 remains performer-owned during harmonic Voicing Type changes.

### Tension

- Stable map remains `43 Clean`, `44 Color`, `45 Rich`.
- Tension and Voicing Type controls at the same sample are coalesced into one musical refresh.
- Pure melodic textures (Unison / Octaves / Doubling) retain their pitch layout even if Tension state changes; the selected Tension remains stored for the next harmonic voicing.

### Harmony Mode — fix2

- `46 / A#1` selects **Direct Router**.
- `47 / B1` selects **Melody Harmonize**.
- Mode keyswitches are global controls: they are recognized in both Harmony Modes so the user can always switch back and forth from the same Sound Variations map.
- Note On and Note Off for 46/47 are swallowed and never reach downstream instruments.
- The mode keyswitch writes the same `HarmonyMode` state used by UI/project persistence.
- A mode keyswitch in the first note-message sample group is preflighted before the processing path is selected, allowing a same-sample Sound Variation + first musical note to start in the requested mode.
- A later-in-block mode switch is latched safely for the following processing block; host acceptance found this workflow correct for the current Sound Variations use case.

## Compatibility

- Existing enum/state meanings remain unchanged.
- Existing Tension keyswitches remain unchanged.
- 0.4c4 fix1 changed only the ergonomic MIDI map for Voicing Type.
- 0.4c4 fix2 added Harmony Mode keyswitches without changing the saved state schema (`stateVersion = 5`).
- Direct Router remains available and its existing distribution logic is unchanged.

## Automated acceptance

- [x] Voicing Type map decodes exact implemented notes.
- [x] Reserved future Voicing Type notes stay inert/swallowed in Melody Harmonize.
- [x] Tension map stays exactly 43/44/45.
- [x] Harmony Mode map is exactly 46/47.
- [x] Harmony Mode block is adjacent above Rich=45.
- [x] Invalid keyswitch decoder input does not mutate state.
- [x] UI/project/MIDI share the same state for Voicing Type, Tension and Harmony Mode.
- [x] Windows Build #387 completed successfully, including build, all tests, packaging and artifact upload.
- [x] Artifact: `Smart-Voicing-0.4c4-fix2-Windows`.

## Studio Pro acceptance

User-confirmed in Studio Pro:

- [x] ergonomic C1-centered Voicing Type map works correctly;
- [x] Closed / Drop 2 / Unison / Octaves / Doubling switch correctly;
- [x] Tension keyswitches work correctly;
- [x] Sound Variations workflow is practical and convenient;
- [x] `A#1 / MIDI 46` selects Direct Router;
- [x] `B1 / MIDI 47` selects Melody Harmonize;
- [x] switching back and forth between both Harmony Modes behaves correctly;
- [x] control notes do not create unwanted downstream musical notes in the accepted workflow.

## Acceptance boundary

**0.4c4 is closed.**

The performance keyswitch layer is now considered stable enough to remain fixed while new Stage 5 voicing strategies are added. Reserved keys receive their implementation later without remapping the accepted Sound Variations layout.

Next slice: **0.4d — Drop 3**. Drop 3 remains a Closed-family transformation: Stage 4/Closed chooses the harmonic material once, then Stage 5 changes only vertical octave placement/shape.

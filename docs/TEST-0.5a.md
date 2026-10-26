# Harma Waves / Smart Voicing 0.5a — Voice Leading foundation

Status: IN DEVELOPMENT. Baseline: accepted 0.5 in main and merged UI PR #39.

Scope: host-neutral, fixed-size previous Voice state; pure deterministic motion cost; common-tone/stepwise preference; crossing and large-leap penalties. Preserve played V1, explicit chord identity, Stage 4 harmonic vocabulary, and Stage 5 strategy shapes. No changes to VST3 ID or saved project format.

Acceptance requires CI, CTest, and separate Studio Pro host checks. The 0.5a branch now connects bounded continuity scoring (maximum ten vertical-score points) to the actual candidate loops for Closed, Drop 2/3/2+4, Spread, Quartal, UST, and Cluster. Unison / Octaves / Doubling remain exact deterministic melody textures. Each candidate remains subject to the existing Stage 4 legal pitch vocabulary and Stage 5 structural scoring; history does not rewrite explicit chord tones or played V1.

Regression cases: major/minor II-V-I, `Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7`, constant common tones under moving melody, sustained melody across chord boundaries, reset on transport/mode changes, V1-only fallback, no stuck notes.

Deferred from 0.5a: strategy-specific motion weights, directed Rich resolutions, Color/Rich selection, instrument-specific register penalties.
## Integration checklist

- [x] Existing host-neutral `VoiceLeadingState` follows the accepted sounding V1..V4 in Melody Harmonize.
- [x] Candidate selection uses previous Voice state only as a bounded tie-break, avoiding harmonic legality override.
- [x] Drop variants score the **transformed sounding voice slots** instead of mistakenly scoring their pre-drop Closed source.
- [x] Strategy and harmony-mode switching clear the previous voice state, preventing cross-texture comparisons.
- [x] Every host-neutral voicing CTest target links the Voice Leading core; legacy no-history output stays unchanged.
- [x] Add CTest cases for unchanged voicings, identity, movement cost cap and a demonstrably altered Closed candidate.
- [ ] Confirm fresh Windows CI build + all CTest green.
- [ ] Manual Studio Pro listening/MIDI capture, chord boundaries and sustain.
- [ ] Verify no stuck MIDI notes on stop, mode changes and keyswitches.

### Musical host test suggestion

Melody Harmonize on separate output MIDI channels Ch1..Ch4. Use Clean first and play a short sequence over `Dm7 | G7 | Cmaj7` and over `Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7`. Compare movement of V2..V4 with legacy 0.5, noting retained common tones and fewer unnecessary leaps. Verify V1 never shifts, characteristic chord tones survive, and Drop families preserve their geometry and slash bass. Repeat chord boundaries with a melody held under sustain. Test three melodic textures separately; do not expect Voice Leading to rewrite those predetermined unison/octave/doubling layouts.

**Status:** audible selection logic is implemented in the draft branch; Windows CI and Studio Pro host acceptance remain independent gates before 0.5a can merge.

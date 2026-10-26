# Harma Waves / Smart Voicing 0.5b — acceptance candidate

Status: IN DEVELOPMENT. Based on accepted 0.5a. Stage 6 / Issue #10.
Do not merge to main before automated and Studio Pro musical acceptance.

## Diagnostic basis

In the supplied Closed/Clean 32-bar screenshots, many verticals were visibly
identical between 0.5 and 0.5a. The global ten-point 0.5a motion cap means
multiple distinct large-leap candidates can receive the same rank. The
screenshots remain a visual comparison, not an independent MIDI/channel
measurement; MIDI-per-channel exports are preferred for quantitative proof.

## First 0.5b slice: Closed + Drop 2 / Drop 3 / Drop 2+4

- First bounded candidate scan finds the optimal Stage 5 vertical.
- With previous V1..V4, the second bounded scan may choose another vertical
  **only within 14 points** of the best static vertical cost.
- It must preserve previously represented guide tones (3/7), characteristic
  chord material (sus/b5/#5), explicitly written 9/11/13, and slash major7
  root identity. Melody V1 stays immutable.
- Motion rank no longer saturates at ten and distinguishes several large leaps.
- For Drop, compare the **transformed sounding** V2..V4 rather than raw Closed.
- Initial notes without previous harmonic history retain static behavior.
- The bounded two-pass scan is realtime fixed-size, without locks, heap or I/O;
  host CPU/performance check remains required.

## Important constraint

The G2 -> G3 leap under Cmaj7/G at a high E4 melody can be unavoidable in
Closed: G2 lies over 24 semitones below E4, outside the search depth. Do not
claim to fix physically excluded candidates by increasing movement weight.

## Acceptance

- [x] Unit tests distinguish small motion, a single leap and multiple leaps.
- [x] Host-neutral progression regression for Closed + three Drop variants.
- [ ] Windows VST3/ARA build and full CTest success.
- [ ] Compare 0.5/0.5a/0.5b with same 32 bars, MIDI per voice.
- [ ] Preserve structure for all eight harmonic modes, especially independent
      Spread/Quartal/UST/Cluster (still on 0.5a rank in this first slice).
- [ ] Performance and stop/seek/sustain/legato host checks.

After this slice, extend strategy-specific continuity to Spread, Quartal,
UST and Cluster, then handle Color/Rich directed resolutions in 0.5c.

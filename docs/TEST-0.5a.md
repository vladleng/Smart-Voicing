# Harma Waves / Smart Voicing 0.5a — Voice Leading foundation

Status: IN DEVELOPMENT. Baseline: accepted 0.5 in main and merged UI PR #39.

Scope: host-neutral, fixed-size previous Voice state; pure deterministic motion cost; common-tone/stepwise preference; crossing and large-leap penalties. Preserve played V1, explicit chord identity, Stage 4 harmonic vocabulary, and Stage 5 strategy shapes. No changes to VST3 ID or saved project format.

Acceptance requires CI, CTest, and separate Studio Pro host checks. Initial cost engine may be added before it affects generated MIDI; do not claim audible Voice Leading until candidate selection is integrated.

Regression cases: major/minor II-V-I, `Dm7 | Db7(b13) | Cm7 | B7#11 | Bbmaj7 | A7`, constant common tones under moving melody, sustained melody across chord boundaries, reset on transport/mode changes, V1-only fallback, no stuck notes.

Deferred from 0.5a: strategy-specific motion weights, directed Rich resolutions, Color/Rich selection, instrument-specific register penalties.
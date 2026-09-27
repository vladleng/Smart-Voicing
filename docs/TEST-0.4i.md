# Smart Voicing 0.4i — Cluster

Status: **independent development candidate; local and Windows CI tests pass; Studio Pro pending**.

Input: Quartal 0.4g accepted in Studio Pro; UST 0.4h initial operation confirmed by the user. These earlier strategies and their package remain available.

## Musical contract

Cluster pursues adjacent seconds and a dense upper group, rather than the fourth stack of Quartal or a rearranged Closed voicing. A third at the top is allowed to preserve a clear performer melody. V1 remains the exact played pitch; V4 is a chord-tone support or the explicit lowest slash bass. An incomplete chord representation may be preferable to a false extension, but the chord's 3/7 and characteristic or explicit altered tones are protected in scoring.

All harmony pitches come from the existing Stage 4 chord/tension candidate pool. Plain triads do not gain invented extensions at Rich. Small seconds are part of the intended sound; compound minor ninths receive a strong soft negative with explicit/directed exceptions. The search is bounded, deterministic and allocation-free, with V1-only fallback where the MIDI domain is too low. Previous-voice motion and instrument ranges belong to later stages.

## Implementation and verification

- `VoicingType::cluster = 10` appended after UST; saved values 0..9 unchanged.
- Independent `buildClusterVoicing()` and host-neutral tests for density, V1, slash bass, characteristic notes, candidate gate, plain triads, low MIDI and determinism.
- `A0 / MIDI 33` activates the shared Voicing Type selection; UI/project-state bounds include 10. Existing controls 32 and 34..47 are unchanged.
- Local Cluster, keyswitch, dispatcher, Quartal and UST tests: passed. Windows Build #460 (commit `dfbac31`): compilation, all Harmony Core tests and package upload passed. [Run and package](https://github.com/vladleng/Smart-Voicing/actions/runs/36248477619), artifact `Smart-Voicing-0.4i-Windows`, SHA256 `3d6c750236b494a553468d8a631d9cd154d20e1f08914b14ceb459c22c4499cf`; ZIP integrity and paired VST3 bundles verified. Studio Pro: pending `docs/0.4i-HOST-CHECKLIST.md`.

After host acceptance of Cluster, close Stage 5 with `docs/STAGE5-CLEAN-ACCEPTANCE.md`: compare the vertical shape and harmonic fidelity of all voicings in Clean using fully specified chords. Clean must retain explicit extensions and alterations; it does not invent colour. Color/Rich refinement for quick harmonization of basic chords belongs to Stage 6 (#10), where continuity and directed resolution can inform candidate selection.

## Stage 5 Clean Closed score follow-up (2026-09-27)

The 32-bar Studio Pro score preserves all 64 melody notes and has no voice crossings. A `Cmaj7/E` event with E4 melody and E3 slash bass initially selected E–B–G–E, losing the C root and sounding like Em. Closed now gives an omitted root in a slash chord a soft penalty, so C can remain alongside the third and seventh when feasible. The chord model and explicit V4 slash bass remain authoritative. The direct regression covers this exact case, and the VoicingStrategy integration suite passes locally. Host recheck of bar 6 is pending.

At bar 5, the printed `C13` with Bb4 melody omitted A/13 in its second vertical. A direct Closed test supplied with explicit C, E, Bb, D and A (degree 13) already retains A below Bb in Clean. The score's Bb–E–D–C output is reproducible when the incoming interval mask has no A. Before changing Closed scoring, inspect Diagnostics at bar 5, second note: the neutral chord interval mask must contain relative pitch class `9`, and the normalized chord symbol / extension flags must contain the thirteenth. If the host presents `C13` but ARA supplies no A, investigate the host chord representation at the ARA boundary. Do not infer a 13 from the project key or add it in Closed without verified chord data.

The bass octave movement at bars 31–32 belongs to Stage 6 Voice Leading, which will score successive verticals using previous voice state.

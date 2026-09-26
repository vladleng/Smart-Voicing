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

After host acceptance of Cluster, run one consolidated comparison of intervals and Clean/Color/Rich behavior across all Voicing Types before starting Stage 6. Treat weak colour differentiation as a tracked tuning task, not as a new tension engine per strategy.

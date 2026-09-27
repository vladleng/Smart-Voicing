# Smart Voicing 0.4h — Upper Structure Triad

Status: **INDEPENDENT DEVELOPMENT BRANCH — Windows CI passed; user reports initial Studio Pro operation**.

Base: Stage 5 working branch with Quartal 0.4g host candidate. This UST branch does not change the 0.4g package under user test.

## Musical contract

UST organizes a major/minor upper-structure triad over a structural support note. It takes the already interpreted Stage 4 candidate vocabulary and never independently infers a dominant target or reinterprets Clean/Color/Rich.

1. V1 is always the exact performed melody. If it belongs to a legal upper triad, V1–V3 can form the complete triad.
2. When V1 is outside the triad or the current Stage 4 palette lacks one of its notes, UST may represent two triad members in V2/V3 rather than changing the melody or inventing the missing tension.
3. V4 is a supporting chord tone (root/guide preferred); an explicit slash bass takes precedence and must remain lowest.
4. Full upper triads are preferred over incomplete shapes. Whole-vertical scoring also protects 3/7 identity, characteristic m7b5 b5 and explicit alterations. It never sacrifices chord authority just to spell a named triad.
5. Plain triads remain chord-tone-only. Clean/Color/Rich can change which upper triads are legal, but a pitch change is not mandatory for every vertical.
6. Minor ninth is a strong soft negative with the accepted explicit/directed exceptions. Register/previous-voice adaptation belongs to later stages.
7. If no safe triad/support layout is possible, output V1 alone. Search is deterministic, bounded and allocation-free.

This is a Stage 5 UST layout for four abstract voices, not an instrument-specific orchestration preset.

## Implementation and verification

- `VoicingType::ust = 9` appended after Quartal without renumbering earlier project states.
- Independent `buildUpperStructureTriadVoicing()`, host-neutral tests for full/partial triad, chord authority, slash bass, explicit #11, m7b5, Stage 4 candidate gate, low MIDI safety and determinism.
- `MIDI 32 / G#0` activates the shared Voicing Type state; 33 remains reserved. UI and project-state bounds extended to 9.
- Local UST, keyswitch, Quartal, Spread, Drop 3, Drop 2+4 and dispatcher tests: passed.
- Windows Build #458 (commit `880d401`): build, Harmony Core tests and artifact upload passed. [Run and package](https://github.com/vladleng/Smart-Voicing/actions/runs/36246791839), artifact `Smart-Voicing-0.4h-Windows`, SHA256 `3e81496fe812fe7e35eee6dab71e7740308f907019fd2ef70001c1189931999e`. ZIP integrity and both VST3 bundles verified.
- Studio Pro: user reports UST mode works (2026-09-26). The complete interval/tension comparison across all Voicing Types is deferred until after Cluster, before Stage 6; individual checklist items are not claimed as exhaustively verified.

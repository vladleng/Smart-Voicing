# Harma Waves — UI refinement before Stage 6

The instrument editor displays **Harma Waves** and the understated **Moon River Studio** label. The VST3 product name, IDs and package names remain Smart Voicing 0.5 so existing Studio Pro projects continue to load the same plug-in.

The compact 1200 × 760 editor follows the approved dark mockup. All eleven Voicing modes and all three Tensions levels remain visible in two columns. Each selected row has a small illuminated indicator. Diagnostics expands below the main controls.

## Studio Pro host check

- [ ] Open a saved 0.5 project: chord, key, meter and tempo appear in the context strip.
- [ ] Click each Voicing row and each Tensions row: the lit indicator moves immediately and the chosen sound follows.
- [ ] Send Voicing MIDI 32–42 and Tensions MIDI 43–45: the matching indicator follows the keyswitch without changing the unrelated column.
- [ ] Switch to Direct Router: Voicing and Tensions appear disabled; Distribution is editable. Switch back to Melody Harmonize and verify the reverse.
- [ ] Save and reopen with Quartal / Color selected: both indicators restore from the project state.
- [ ] Expand and collapse Diagnostics; check the MIDI monitor and reset button remain usable.

The UI uses the existing processor state and keyswitch path. No harmony, routing or saved-state format changes are part of this work.

## UI acceptance (2026-10-26)

The user explicitly accepted the Harma Waves interface. PR #39 was merged into `main` as commit `f00449b8238737fb9c9f7234fe2c1fda148ac3a0` after the successful Windows Build #476 (run 36307567041).

Acceptance applies to the displayed UI design and authorizes Stage 6 development. The per-item Studio Pro technical checklist above was not independently reported item-by-item; do not mark those checks as individually verified on this basis alone.

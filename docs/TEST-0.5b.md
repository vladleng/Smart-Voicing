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

Correction to the previous wording: E4=MIDI64 and G2=MIDI43, so the G2
alternative lies **21 semitones** below the melody, *inside* the existing
24-semitone search depth. It can nevertheless lose because a G2 bass greatly
widens the Closed vertical (strong total-span penalty) and the history search
permits only 14 points of static-score loss. Retaining G2 is not automatically
the more musical Closed voicing; do not call this a fixed defect without
testing viable competing verticals.

## Acceptance

- [x] Unit tests distinguish small motion, a single leap and multiple leaps.
- [x] Host-neutral progression regression for Closed + three Drop variants.
- [x] Guard against exposed V1–V2 seventh in Closed if a compact reference
      exists; direct Cmaj7/E regression.
- [x] Guard against moving a chromatically altered extension (including
      generic-annotated dominant b9) into sounding V4 when the reference
      carries a structural bass; retain unaltered written thirteenths.
- [x] Reconcile correct 21-semitone E4–G2 interval in Cmaj7/G analysis.
- [x] Initial #503 Windows VST3/ARA build and full CTest success.
- [ ] CI verification of the revised harmonic guardrails (next build).
- [ ] Confirm Instrument top caption, MIDI Engine heading, diagnostics and
      ARA editor all display **0.5b** from generated build version.
- [ ] Confirm VST3 host metadata reports numerical version **0.5.2**
      while VST3 identity/product names and old project compatibility remain stable.
- [ ] Compare 0.5/0.5a/0.5b with same 32 bars, MIDI per voice.
- [ ] Preserve structure for all eight harmonic modes, especially independent
      Spread/Quartal/UST/Cluster (still on 0.5a rank in this first slice).
- [ ] Performance and stop/seek/sustain/legato host checks.

After this slice, extend strategy-specific continuity to Spread, Quartal,
UST and Cluster, then handle Color/Rich directed resolutions in 0.5c.

## Musical audit from 0.5b screenshots (pre-guardrail)

- Bar 4 C7#11: improved inner movement, with characteristic #11 and 3/7 retained.
- Bar 6 Cmaj7/E: B3-C3 (11-semitone exposed upper gap), and worse
  neighboring exit. New guard forbids upper gap > max(7, reference gap).
- Bar 8 G7b9: rootless V4 Ab2 duplicated the Ab3 melody, replacing the
  structural bass. New guard keeps the lower alteration out unless the
  vertical baseline itself already calls for it. Explicit written A/13 remains
  legal in V4; chromatically altered b9 is a different role.
- Bar 28 Db7b13: changes are largely neutral across surrounding motion.
- Cmaj7/G (last bar): independent task; see corrected range note above.

The present architecture knows previous sounding Voice slots but cannot see
the *future played melody note*. Hence the upper-gap protection addresses
a known undesirable musical state without falsely claiming a future-aware
two-sided optimization. A true next-melody lookahead, if available from
the host, belongs to a separately specified enhancement, not this patch.


## 0.5b follow-up — low register Closed / G7b9

The second vertical at G7b9 has F3 as the performer melody; retaining G2
and Ab2 immediately above each other produces a minor second in the low bass.
This is legal chord material but often muddy in a small ensemble, and cannot
be fixed merely by increasing previous-voice motion cost. With just three
generated notes, the static root+3+b9+b7 arrangement is inherently cramped
under F3.

Closed now applies a **soft** lower-register second penalty when both V3/V4
are below C3 (MIDI48), plus a low altered b9 bass penalty for non-slash
dominants. A rootless alternative with a fifth/third supporting bass may keep
the explicitly authored b9, the third and the seventh without the deep
semitone. No hard veto: explicit slash bass and unavoidable written
alterations take precedence. C3-B2 upper shell and mid-register seconds are
not generically outlawed. Spread/Quartal/UST/Cluster are not changed by the
Closed-specific rule.

- [x] Unit regression for G7b9 with F3 melody: verify b9, 3/b7, four voices
      and preference away from low G2–Ab2 adjacency with/without history.
- [x] Regression verifies slash bass Cmaj7/G remains authoritative.
- [ ] Windows CI and musical Studio Pro score review of this follow-up.

The explicit dominant b9 has a pre-existing Stage 5 omission penalty
(`flatNinthOmissionPenalty`) shared in `VoicingIdentity.h`. Closed now
applies it as well, specifically so the lower-register penalty cannot
"solve" the low G–Ab semitone by dropping Ab from a written G7b9.

## Studio Pro chord source verification — 2026-10-27

User provided the **actual Chord Track four-note spelling of G7b9** in Studio
Pro: G3, G#3 (enharmonic Ab3), B3, F4. **D is absent**.
The previous G7b9 unit fixture contained an assumed perfect fifth; it did
not match the user's source. A revised test covers both the exact four-note
source and the optional five-note source, with/without previous voice state.
It asserts that the four-note input must not invent D, must retain b9/B/F,
and should prefer avoiding the low Ab2-G2 semitone.

With melody F3 and an exact four-tone palette, Closed cannot simultaneously
use four different chord pitch classes, fit in a compact octave, and avoid
placing G and Ab together. The engine must weigh a rootless guide-tone
doubling against expanding the voicing; the test protects explicit material
without demanding an artificial fifth.

**Build provenance:** Both Instrument and ARA editor now display
`0.5b #<GitHub Actions run number>` on CI artifacts, because just `0.5b`
cannot distinguish builds #504 and #506 when replacing VST3 in Studio Pro.
VST3 semantic version remains 0.5.2; IDs remain unchanged. Local CMake
builds without GitHub Actions still display `0.5b`.

## 0.5b safety correction — four-note G7b9 and family-specific motion

The user's corrected Studio Pro Chord Track layout is **G3, B3, F4,
G#4 (Ab4)**, with **no D**. ChordContext/NormalizedChord uses pitch
classes plus interval-degree roles, so the octave of the written b9 does
not change the chord candidate pool. With F3 melody and only G, Ab, B, F,
a compact four-distinct-pitch-class Closed vertical may require the low
G2-Ab2 semitone. This is a legitimate compromise, not a blanket bug.
The regression NO LONGER mandates removing this second; it verifies
all named pitch classes, V1 ownership, descending order, and no invented D.
The *soft* low-register dissonance penalty remains useful when actual
equivalent choices exist, but explicit harmony and Closed identity win.

Spread, Quartal, UST and Cluster now use the **same bounded eight-candidate
history shortlist** with style-specific weight and vertical-cost margin.
This is a single vertical search, not a second exhaustive pass:
- Spread: preserve open bass anchor and 8-score-point window;
- Quartal: preserve fourth-oriented vertical and 6-point window;
- UST: preserve upper-structure eligibility and 5-point window;
- Cluster: preserve close upper density and 6-point window.

The shortlist never overrides chord legality, V1, slash bass or previously
represented guide/explicit altered tones; the Stage 5 static vertical remains
a safe fallback. Other melodic-section textures are untouched. All runs are
fixed-size, host-neutral and allocation-free.

- [x] Correct four-tone G7b9 synthetic regression and remove universal veto
      on an unavoidable low semitone.
- [x] Add style-dependent motion ranking for four independent strategies.
- [x] Add progression tests for these four styles including Cmaj9, G7b9,
      slash Cmaj7/G, V1 and preserved written 3/7/9.
- [ ] Windows CI for the above latest implementation.
- [ ] User host MIDI score acceptance on all strategies and realtime load.

## 0.5b fix1 — confirmed brief transition note in Spread / Cm

Source: user's five 32-bar 0.5b #508 Clean PDFs. The user explicitly
confirmed the extra note in bar 2 (Cm), Spread, V3 is a **micro-note
during transition** rather than a valid fifth voice. Tracking issue #43.

Static diagnosis found a plausible engine-level cause: while melody is held,
`processMelodyHarmonizeMidi` calls `refreshMelodyHarmonyAtPpq` at every
audio block start, even without a new chord, voicing type or Tension Level.
Because candidate search depends on its own previous sounding voicing,
repeated identical-context recalculations may alternate candidates,
emitting brief unnecessary off/on pairs. A coincident melody-note and Chord
Track boundary can additionally use a slightly earlier floating-point PPQ,
creating an old-chord candidate followed immediately by a new-chord update.

**Fix1 safeguards:**
- Host-neutral `MelodyHarmonyRefreshGuard` suppresses recalculation while
  chord/key context, ARA revision, transport revision and performance choices
  are unchanged. It excludes the continuously moving playback PPQ.
- First melody Note On commits its accepted context; applied chord/tension/
  voicing changes commit even when the new vertical has identical notes.
- Equal-sample note and chord boundary use the **exact scheduled chord PPQ**
  when generating the new melody voicing, not the potentially under-rounded
  `ppqForSamplePosition`; later boundary refresh sees the same context and
  emits no intermediate old-chord notes.
- Sustain/chord boundaries, key changes, ARA edits, mode/keyswitch changes,
  transport revisions and stop/reset remain refresh triggers.
- Tests exercise 256 consecutive steady Cm audio-block positions, event
  changes, reset, coincident boundaries and floating-point underflow.

**Status:** automated CI and real Studio Pro MIDI capture are still required.
The analysis supplies a defensible cause and preventative fix, **not a
confirmed event-level trace** of the original micro-note.
- [ ] Windows Build `0.5b fix1` passes CTest and packages VST3.
- [ ] Repeat Spread/Clean bar 2 in Studio Pro #fix1; V3 emits no transient.
- [ ] Repeat with chord boundary coincident with new melody note,
      legato/sustain, keyswitch and transport seek; no stuck notes.

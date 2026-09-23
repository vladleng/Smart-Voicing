#pragma once

#include "HarmonicContext.h"

#include <array>

namespace smartvoicing::harmony
{
struct VoiceTransition
{
    bool noteOff = false;
    int oldNote = -1;
    bool noteOn = false;
    int newNote = -1;
};

struct ReharmonizationPlan
{
    std::array<VoiceTransition, kVoiceCount> voices {};
    bool lowerVoicesChanged = false;
};

// Small host-neutral ownership state for Melody Harmonize. It mirrors the
// musical meaning of a physical key plus Sustain without knowing anything
// about JUCE, MIDI channels or the Router implementation.
struct MelodyGateDecision
{
    bool releaseVoicing = false;
};

class MelodyGateState
{
public:
    void reset() noexcept;
    void beginNote(int midiNote) noexcept;
    MelodyGateDecision endNote(int midiNote) noexcept;
    MelodyGateDecision setSustain(bool down) noexcept;
    void releaseMelody() noexcept;

    int activeNote() const noexcept { return currentNote; }
    bool keyDown() const noexcept { return physicalKeyDown; }
    bool sustainDown() const noexcept { return pedalDown; }
    bool ownsVoicing() const noexcept { return currentNote >= 0; }

private:
    int currentNote = -1;
    bool physicalKeyDown = false;
    bool pedalDown = false;
};

// Build a transition from the currently sounding Melody Harmonize voicing to
// the desired one. V1 is deliberately immutable here: the played melody owns
// V1, while only generated V2..V4 may be replaced by a Chord Track change.
ReharmonizationPlan planLowerVoiceReharmonization(const VoiceOutput& current,
                                                  const VoiceOutput& desired) noexcept;

// Build an atomic transition for a new played melody articulation.
//
// For harmonic strategies, retriggerMelody=true rearticulates repeated V1 while
// common generated lower voices remain sounding. Melodic-section strategies such
// as Unison may pass retriggerMatchingLowerVoices=true so repeated same-pitch
// melody notes rearticulate the whole section rather than only V1.
ReharmonizationPlan planVoicingTransition(const VoiceOutput& current,
                                          const VoiceOutput& desired,
                                          bool retriggerMelody,
                                          bool retriggerMatchingLowerVoices = false) noexcept;

// Timeline helpers for scheduling a known future Chord Track boundary inside
// the current audio block. Values outside the block return -1 instead of being
// clamped, so the next block owns that event. They do not add lookahead/latency.
int sampleOffsetFromTimelineSeconds(double blockStartSeconds,
                                    double eventSeconds,
                                    double sampleRate,
                                    int blockSamples) noexcept;

int sampleOffsetFromPpq(double blockStartPpq,
                        double eventPpq,
                        double bpm,
                        double sampleRate,
                        int blockSamples) noexcept;
}

#pragma once

#include <JuceHeader.h>
#include "InstrumentPluginProcessor.h"

class SmartVoicingInstrumentEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor&);
    ~SmartVoicingInstrumentEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshContextMonitor();

    SmartVoicingInstrumentProcessor& processor;

    juce::Label titleLabel;
    juce::Label bridgeLabel;
    juce::Label chordLabel;
    juce::Label keyLabel;
    juce::Label timeSignatureLabel;
    juce::Label tempoLabel;
    juce::Label positionLabel;
    juce::Label midiProbeTitleLabel;
    juce::Label midiProbeLabel;
    juce::TextButton resetMidiStatsButton;
    juce::Label debugLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingInstrumentEditor)
};

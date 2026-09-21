#pragma once

#include <JuceHeader.h>
#include "ARAContextProvider.h"
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
    smartvoicing::harmony::ARAContextProvider harmonicContextProvider;

    juce::Label titleLabel;
    juce::Label bridgeLabel;
    juce::Label chordLabel;
    juce::Label keyLabel;
    juce::Label timeSignatureLabel;
    juce::Label tempoLabel;
    juce::Label positionLabel;
    juce::Label harmonyModeLabel;
    juce::ComboBox harmonyModeBox;
    juce::Label tensionLevelLabel;
    juce::ComboBox tensionLevelBox;
    juce::Label distributionModeLabel;
    juce::ComboBox distributionModeBox;
    juce::Label midiProbeTitleLabel;
    juce::Label midiProbeLabel;
    juce::TextButton resetMidiStatsButton;
    juce::Label debugLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingInstrumentEditor)
};
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
    static constexpr int editorWidth = 900;
    static constexpr int compactEditorHeight = 590;
    static constexpr int expandedEditorHeight = 1190;

    void timerCallback() override;
    void refreshContextMonitor();
    void updateDiagnosticsVisibility();

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
    juce::Label voicingTypeLabel;
    juce::ComboBox voicingTypeBox;
    juce::Label tensionLevelLabel;
    juce::ComboBox tensionLevelBox;
    juce::Label distributionModeLabel;
    juce::ComboBox distributionModeBox;
    juce::TextButton diagnosticsButton;
    juce::Label midiProbeTitleLabel;
    juce::Label midiProbeLabel;
    juce::TextButton resetMidiStatsButton;
    juce::Label debugLabel;

    bool diagnosticsExpanded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingInstrumentEditor)
};
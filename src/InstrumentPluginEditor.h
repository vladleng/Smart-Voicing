#pragma once

#include <JuceHeader.h>
#include "ARAContextProvider.h"
#include "InstrumentPluginProcessor.h"
#include <array>
#include <memory>

class SmartVoicingChoiceButton final : public juce::TextButton
{
public:
    SmartVoicingChoiceButton(const juce::String& name, const juce::String& midi,
                             const juce::String& description = {})
        : juce::TextButton(name), midiLabel(midi), detail(description) {}

    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    void setActivePalette(juce::Colour indicator, juce::Colour row)
    {
        activeIndicator = indicator;
        activeRow = row;
    }

private:
    juce::String midiLabel, detail;
    juce::Colour activeIndicator { 0xff94c8d5 };
    juce::Colour activeRow { 0xff35454b };
};

class SmartVoicingInstrumentEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor&);
    ~SmartVoicingInstrumentEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int editorWidth = 1200;
    static constexpr int compactEditorHeight = 760;
    static constexpr int expandedEditorHeight = 1660;

    void timerCallback() override;
    void refreshContextMonitor();
    void updateDiagnosticsVisibility();

    SmartVoicingInstrumentProcessor& processor;
    smartvoicing::harmony::ARAContextProvider harmonicContextProvider;

    juce::Label titleLabel;
    juce::Label vendorLabel;
    juce::Label bridgeLabel;
    juce::Label chordLabel;
    juce::Label keyLabel;
    juce::Label timeSignatureLabel;
    juce::Label tempoLabel;
    juce::Label positionLabel;
    juce::Label harmonyModeLabel;
    juce::ComboBox harmonyModeBox;
    std::array<std::unique_ptr<SmartVoicingChoiceButton>, 11> voicingButtons;
    std::array<std::unique_ptr<SmartVoicingChoiceButton>, 3> tensionButtons;
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

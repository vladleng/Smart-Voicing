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
    void refreshDebugText();

    SmartVoicingInstrumentProcessor& processor;
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingInstrumentEditor)
};

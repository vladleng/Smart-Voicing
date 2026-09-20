#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmartVoicingAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit SmartVoicingAudioProcessorEditor(SmartVoicingAudioProcessor&);
    ~SmartVoicingAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshDebugText();

    SmartVoicingAudioProcessor& processor;
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingAudioProcessorEditor)
};

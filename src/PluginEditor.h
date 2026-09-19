#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SmartVoicingAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SmartVoicingAudioProcessorEditor(SmartVoicingAudioProcessor&);
    ~SmartVoicingAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SmartVoicingAudioProcessor& processor;
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartVoicingAudioProcessorEditor)
};

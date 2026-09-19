#include "PluginEditor.h"
#include "ARAContextDebugState.h"

namespace
{
juce::String availabilityText(bool available, int eventCount)
{
    if (! available)
        return "unavailable";

    return "AVAILABLE (events: " + juce::String(eventCount) + ")";
}
}

SmartVoicingAudioProcessorEditor::SmartVoicingAudioProcessorEditor(SmartVoicingAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("Smart Voicing 0.0b - Shared ARA Bridge Test", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    statusLabel.setJustificationType(juce::Justification::topLeft);
    statusLabel.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(statusLabel);

    setSize(600, 470);
    refreshDebugText();
    startTimerHz(4);
}

SmartVoicingAudioProcessorEditor::~SmartVoicingAudioProcessorEditor()
{
    stopTimer();
}

void SmartVoicingAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SmartVoicingAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(50));
    area.removeFromTop(10);
    statusLabel.setBounds(area);
}

void SmartVoicingAudioProcessorEditor::timerCallback()
{
    refreshDebugText();
}

void SmartVoicingAudioProcessorEditor::refreshDebugText()
{
    const auto snapshot = ARAContextDebugState::instance().getSnapshot();

    juce::String text;
    text << "VST3 role: Instrument + Fx\n";
    text << "MIDI input: YES\n";
    text << "MIDI output: YES\n\n";

    text << "Local ARA instance bound: " << (processor.isARABoundForDebug() ? "YES" : "NO") << "\n";
    text << "Registered ARA controllers: " << snapshot.registeredControllerCount << "\n";
    text << "Shared ARA context: " << (snapshot.sharedContextAvailable ? "YES" : "NO") << "\n";
    text << "Best musical contexts: " << snapshot.musicalContextCount << "\n";
    text << "Host content access: " << (snapshot.hostContentAccessAvailable ? "YES" : "NO") << "\n\n";

    text << "Key Signatures: "
         << availabilityText(snapshot.keySignaturesAvailable, snapshot.keySignatureEventCount) << "\n";
    text << "Sheet Chords: "
         << availabilityText(snapshot.sheetChordsAvailable, snapshot.sheetChordEventCount) << "\n";
    text << "Tempo Entries: "
         << availabilityText(snapshot.tempoEntriesAvailable, snapshot.tempoEntryEventCount) << "\n";
    text << "Bar Signatures: "
         << availabilityText(snapshot.barSignaturesAvailable, snapshot.barSignatureEventCount) << "\n\n";

    const auto seconds = processor.getLastPositionSecondsForDebug();
    const auto ppq = processor.getLastPpqPositionForDebug();
    text << "Transport seconds: " << (seconds >= 0.0 ? juce::String(seconds, 3) : "n/a") << "\n";
    text << "Transport PPQ: " << (ppq >= 0.0 ? juce::String(ppq, 3) : "n/a") << "\n\n";

    text << juce::String(L"\u041A\u043E\u0434\u0438\u0440\u043E\u0432\u043A\u0430: \u041E\u041A");

    statusLabel.setText(text, juce::dontSendNotification);
}

#include "InstrumentPluginEditor.h"
#include "SharedHarmonicContext.h"

namespace
{
juce::String availabilityText(bool available, int eventCount)
{
    if (! available)
        return "unavailable";

    return "AVAILABLE (events: " + juce::String(eventCount) + ")";
}
}

SmartVoicingInstrumentEditor::SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("Smart Voicing 0.0c - Instrument", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    statusLabel.setJustificationType(juce::Justification::topLeft);
    statusLabel.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(statusLabel);

    setSize(590, 410);
    refreshDebugText();
    startTimerHz(4);
}

SmartVoicingInstrumentEditor::~SmartVoicingInstrumentEditor()
{
    stopTimer();
}

void SmartVoicingInstrumentEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void SmartVoicingInstrumentEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(50));
    area.removeFromTop(10);
    statusLabel.setBounds(area);
}

void SmartVoicingInstrumentEditor::timerCallback()
{
    refreshDebugText();
}

void SmartVoicingInstrumentEditor::refreshDebugText()
{
    const auto context = SharedHarmonicContextBridge::instance().read();

    juce::String text;
    text << "Role: Instrument / MIDI engine\n";
    text << "MIDI input: YES\n";
    text << "MIDI output: YES\n\n";

    text << "Smart Voicing ARA bridge: " << (context.connected ? "CONNECTED" : "WAITING") << "\n";
    text << "Bridge revision: " << juce::String(static_cast<juce::int64>(context.revision)) << "\n";
    text << "Host content access: " << (context.hostContentAccessAvailable ? "YES" : "NO") << "\n";
    text << "Musical contexts: " << context.musicalContextCount << "\n\n";

    text << "Key Signatures: "
         << availabilityText(context.keySignaturesAvailable, context.keySignatureEventCount) << "\n";
    text << "Sheet Chords: "
         << availabilityText(context.sheetChordsAvailable, context.sheetChordEventCount) << "\n";
    text << "Tempo Entries: "
         << availabilityText(context.tempoEntriesAvailable, context.tempoEntryEventCount) << "\n";
    text << "Bar Signatures: "
         << availabilityText(context.barSignaturesAvailable, context.barSignatureEventCount) << "\n\n";

    const auto seconds = processor.getLastPositionSecondsForDebug();
    const auto ppq = processor.getLastPpqPositionForDebug();
    text << "Transport seconds: " << (seconds >= 0.0 ? juce::String(seconds, 3) : "n/a") << "\n";
    text << "Transport PPQ: " << (ppq >= 0.0 ? juce::String(ppq, 3) : "n/a") << "\n";

    statusLabel.setText(text, juce::dontSendNotification);
}

#include "PluginEditor.h"
#include "ARAContextDebugState.h"
#include "HarmonicContextDebugText.h"
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

SmartVoicingAudioProcessorEditor::SmartVoicingAudioProcessorEditor(SmartVoicingAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("Smart Voicing ARA 0.1b", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    statusLabel.setJustificationType(juce::Justification::topLeft);
    statusLabel.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(statusLabel);

    setSize(820, 680);
    refreshDebugText();
    startTimerHz(8);
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
    const auto local = ARAContextDebugState::instance().getSnapshot();
    const auto context = SharedHarmonicContextBridge::instance().read();

    juce::String text;
    text << "Role: ARA / Event FX context reader\n";
    text << "Audio: pass-through\n\n";

    text << "ARA instance bound: " << (processor.isARABoundForDebug() ? "YES" : "NO") << "\n";
    text << "ARA controllers: " << local.registeredControllerCount << "\n";
    text << "Host content access: " << (local.hostContentAccessAvailable ? "YES" : "NO") << "\n";
    text << "Musical contexts: " << local.musicalContextCount << "\n";
    text << "Harmony revision: " << juce::String(static_cast<juce::int64>(context.revision)) << "\n";
    text << "Transport revision: " << juce::String(static_cast<juce::int64>(context.transportRevision)) << "\n\n";

    text << "Key Signatures: "
         << availabilityText(local.keySignaturesAvailable, local.keySignatureEventCount) << "\n";
    text << "Sheet Chords: "
         << availabilityText(local.sheetChordsAvailable, local.sheetChordEventCount) << "\n";
    text << "Tempo Entries: "
         << availabilityText(local.tempoEntriesAvailable, local.tempoEntryEventCount) << "\n";
    text << "Bar Signatures: "
         << availabilityText(local.barSignaturesAvailable, local.barSignatureEventCount) << "\n\n";

    text << "Transport local: ";
    if (processor.getLastPpqPositionForDebug() >= 0.0)
        text << "PPQ " << juce::String(processor.getLastPpqPositionForDebug(), 6);
    else
        text << "n/a";

    if (processor.getLastPositionSecondsForDebug() >= 0.0)
        text << " | " << juce::String(processor.getLastPositionSecondsForDebug(), 6) << " sec";

    text << "\nShared transport: ";
    if (context.transportAvailable)
    {
        text << (context.transportPlaying ? "PLAY" : "STOP")
             << " | PPQ " << juce::String(context.transportPpq, 6)
             << " | " << juce::String(context.transportSeconds, 6) << " sec";
    }
    else
    {
        text << "n/a";
    }

    text << "\n\n";
    text << smartvoicing::debug::boundaryDiagnostics(context,
                                                      context.transportAvailable ? context.transportPpq
                                                                                 : processor.getLastPpqPositionForDebug());
    text << "\n\n";
    text << smartvoicing::debug::timelinePreview(context);

    statusLabel.setText(text, juce::dontSendNotification);
}

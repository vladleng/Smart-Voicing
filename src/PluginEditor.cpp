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
    titleLabel.setText("Smart Voicing ARA 0.1", juce::dontSendNotification);
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

    const auto seconds = processor.getLastPositionSecondsForDebug();
    const auto ppq = processor.getLastPpqPositionForDebug();
    text << "Transport seconds: " << (seconds >= 0.0 ? juce::String(seconds, 6) : "n/a") << "\n";
    text << "Transport PPQ: " << (ppq >= 0.0 ? juce::String(ppq, 9) : "n/a") << "\n";
    text << "Shared transport: "
         << (context.transportAvailable ? "AVAILABLE" : "n/a")
         << " | " << (context.transportPlaying ? "PLAY" : "STOP") << "\n";
    text << "Transport publish: changes only\n";
    text << smartvoicing::debug::boundaryDiagnostics(context, ppq) << "\n\n";

    text << smartvoicing::debug::activeContextText(context, ppq) << "\n";
    text << smartvoicing::debug::timelinePreview(context) << "\n";

    text << juce::String(L"\u041A\u043E\u0434\u0438\u0440\u043E\u0432\u043A\u0430: \u041E\u041A");

    statusLabel.setText(text, juce::dontSendNotification);
}

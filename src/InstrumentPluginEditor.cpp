#include "InstrumentPluginEditor.h"
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

void configureContextLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    label.setMinimumHorizontalScale(0.75f);
}
}

SmartVoicingInstrumentEditor::SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("Smart Voicing 0.1a - Context Monitor", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    bridgeLabel.setJustificationType(juce::Justification::centredLeft);
    bridgeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(bridgeLabel);

    configureContextLabel(chordLabel);
    configureContextLabel(keyLabel);
    configureContextLabel(timeSignatureLabel);
    configureContextLabel(tempoLabel);
    addAndMakeVisible(chordLabel);
    addAndMakeVisible(keyLabel);
    addAndMakeVisible(timeSignatureLabel);
    addAndMakeVisible(tempoLabel);

    positionLabel.setJustificationType(juce::Justification::centredLeft);
    positionLabel.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(positionLabel);

    debugLabel.setJustificationType(juce::Justification::topLeft);
    debugLabel.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(debugLabel);

    setSize(860, 720);
    refreshContextMonitor();
    startTimerHz(8);
}

SmartVoicingInstrumentEditor::~SmartVoicingInstrumentEditor()
{
    stopTimer();
}

void SmartVoicingInstrumentEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto monitorArea = getLocalBounds().reduced(20);
    monitorArea.removeFromTop(88);
    monitorArea.setHeight(190);

    g.setColour(getLookAndFeel().findColour(juce::Label::outlineColourId)
                    .withAlpha(0.35f));
    g.drawRoundedRectangle(monitorArea.toFloat(), 8.0f, 1.0f);
}

void SmartVoicingInstrumentEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    titleLabel.setBounds(area.removeFromTop(45));
    bridgeLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(11);

    chordLabel.setBounds(area.removeFromTop(44).reduced(12, 0));
    keyLabel.setBounds(area.removeFromTop(44).reduced(12, 0));
    timeSignatureLabel.setBounds(area.removeFromTop(44).reduced(12, 0));
    tempoLabel.setBounds(area.removeFromTop(44).reduced(12, 0));

    area.removeFromTop(10);
    positionLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(8);
    debugLabel.setBounds(area);
}

void SmartVoicingInstrumentEditor::timerCallback()
{
    refreshContextMonitor();
}

void SmartVoicingInstrumentEditor::refreshContextMonitor()
{
    const auto context = SharedHarmonicContextBridge::instance().read();

    const auto localSeconds = processor.getLastPositionSecondsForDebug();
    const auto localPpq = processor.getLastPpqPositionForDebug();

    const auto useBridgeTransport = context.transportAvailable && context.transportPpq >= 0.0;
    const auto ppq = useBridgeTransport ? context.transportPpq : localPpq;
    const auto seconds = useBridgeTransport ? context.transportSeconds : localSeconds;

    juce::String chord = "n/a";
    juce::String key = "n/a";
    juce::String timeSignature = "n/a";
    juce::String tempo = "n/a";

    if (ppq >= 0.0)
    {
        const auto chordIndex = smartvoicing::debug::findActiveEventIndex(context.sheetChords,
                                                                         context.sheetChordStoredCount,
                                                                         ppq);
        const auto keyIndex = smartvoicing::debug::findActiveEventIndex(context.keySignatures,
                                                                       context.keySignatureStoredCount,
                                                                       ppq);
        const auto barIndex = smartvoicing::debug::findActiveEventIndex(context.barSignatures,
                                                                       context.barSignatureStoredCount,
                                                                       ppq);

        if (chordIndex >= 0)
            chord = smartvoicing::debug::chordText(context.sheetChords[chordIndex]);

        if (keyIndex >= 0)
            key = smartvoicing::debug::keyText(context.keySignatures[keyIndex]);

        if (barIndex >= 0)
        {
            const auto& signature = context.barSignatures[barIndex];
            timeSignature = juce::String(signature.numerator) + "/" + juce::String(signature.denominator);
        }

        const auto bpm = smartvoicing::debug::tempoBpmAtPpq(context, ppq);
        if (bpm > 0.0)
            tempo = juce::String(bpm, 2) + " BPM";
    }

    chordLabel.setText(juce::String(L"\u0410\u043a\u043a\u043e\u0440\u0434: ") + chord,
                       juce::dontSendNotification);
    keyLabel.setText(juce::String(L"\u0422\u043e\u043d\u0430\u043b\u044c\u043d\u043e\u0441\u0442\u044c: ") + key,
                     juce::dontSendNotification);
    timeSignatureLabel.setText(juce::String(L"\u0420\u0430\u0437\u043c\u0435\u0440: ") + timeSignature,
                               juce::dontSendNotification);
    tempoLabel.setText(juce::String(L"\u0422\u0435\u043c\u043f: ") + tempo,
                       juce::dontSendNotification);

    juce::String bridgeText;
    bridgeText << "Smart Voicing ARA: " << (context.connected ? "CONNECTED" : "WAITING")
               << " | Harmony rev: " << juce::String(static_cast<juce::int64>(context.revision))
               << " | Transport rev: " << juce::String(static_cast<juce::int64>(context.transportRevision));
    bridgeLabel.setText(bridgeText, juce::dontSendNotification);

    juce::String positionText;
    positionText << "Позиция: ";
    if (ppq >= 0.0)
        positionText << "PPQ " << juce::String(ppq, 6);
    else
        positionText << "n/a";

    if (seconds >= 0.0)
        positionText << " | " << juce::String(seconds, 6) << " sec";

    positionText << " | источник: " << (useBridgeTransport ? "Smart Voicing ARA" : "Instrument");
    if (context.transportAvailable)
        positionText << " | " << (context.transportPlaying ? "PLAY" : "STOP");

    positionLabel.setText(positionText, juce::dontSendNotification);

    juce::String debugText;
    debugText << "Техническая диагностика\n";
    debugText << "MIDI input/output: YES / YES\n";
    debugText << "Host content access: " << (context.hostContentAccessAvailable ? "YES" : "NO")
              << " | Musical contexts: " << context.musicalContextCount << "\n";
    debugText << "Key Signatures: "
              << availabilityText(context.keySignaturesAvailable, context.keySignatureEventCount) << "\n";
    debugText << "Sheet Chords: "
              << availabilityText(context.sheetChordsAvailable, context.sheetChordEventCount) << "\n";
    debugText << "Tempo Entries: "
              << availabilityText(context.tempoEntriesAvailable, context.tempoEntryEventCount) << "\n";
    debugText << "Bar Signatures: "
              << availabilityText(context.barSignaturesAvailable, context.barSignatureEventCount) << "\n";
    debugText << smartvoicing::debug::boundaryDiagnostics(context, ppq) << "\n\n";
    debugText << smartvoicing::debug::timelinePreview(context);

    debugLabel.setText(debugText, juce::dontSendNotification);
}

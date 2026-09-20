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

juce::String channelsText(std::uint32_t mask)
{
    if (mask == 0)
        return "none";

    juce::String result;
    for (int channel = 1; channel <= 16; ++channel)
    {
        if ((mask & (1u << static_cast<unsigned int>(channel - 1))) == 0)
            continue;

        if (result.isNotEmpty())
            result << ",";

        result << channel;
    }

    return result;
}

juce::String lastMidiEventText(const SmartVoicingInstrumentProcessor::MidiProbeSnapshot& snapshot)
{
    using Type = SmartVoicingInstrumentProcessor::MidiProbeEventType;

    juce::String result;
    switch (snapshot.lastEventType)
    {
        case Type::noteOn:
            result << "Note On " << snapshot.lastData1 << " vel " << snapshot.lastData2;
            break;
        case Type::noteOff:
            result << "Note Off " << snapshot.lastData1 << " vel " << snapshot.lastData2;
            break;
        case Type::controller:
            result << "CC " << snapshot.lastData1 << " = " << snapshot.lastData2;
            break;
        case Type::pitchBend:
            result << "Pitch Bend " << (snapshot.lastData1 | (snapshot.lastData2 << 7));
            break;
        case Type::otherChannel:
            result << "Other channel message";
            break;
        case Type::systemOrOther:
            result << "System / other message";
            break;
        case Type::none:
        default:
            result << "none";
            break;
    }

    if (snapshot.lastChannel > 0)
        result << " | Ch " << snapshot.lastChannel;

    return result;
}
}

SmartVoicingInstrumentEditor::SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("Smart Voicing 0.1a - MIDI Router Probe", juce::dontSendNotification);
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

    midiProbeTitleLabel.setText("MIDI Router 0.1a | transparent pass-through", juce::dontSendNotification);
    midiProbeTitleLabel.setJustificationType(juce::Justification::centredLeft);
    midiProbeTitleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    addAndMakeVisible(midiProbeTitleLabel);

    midiProbeLabel.setJustificationType(juce::Justification::topLeft);
    midiProbeLabel.setFont(juce::FontOptions(13.5f));
    addAndMakeVisible(midiProbeLabel);

    resetMidiStatsButton.setButtonText(juce::String(L"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c MIDI stats"));
    resetMidiStatsButton.onClick = [this]
    {
        processor.resetMidiProbeStatistics();
        refreshContextMonitor();
    };
    addAndMakeVisible(resetMidiStatsButton);

    debugLabel.setJustificationType(juce::Justification::topLeft);
    debugLabel.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(debugLabel);

    setSize(860, 860);
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

    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(88);

    auto contextArea = area.removeFromTop(190);
    g.setColour(getLookAndFeel().findColour(juce::Label::outlineColourId).withAlpha(0.35f));
    g.drawRoundedRectangle(contextArea.toFloat(), 8.0f, 1.0f);

    area.removeFromTop(54);
    auto midiArea = area.removeFromTop(150);
    g.drawRoundedRectangle(midiArea.toFloat(), 8.0f, 1.0f);
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
    area.removeFromTop(12);

    midiProbeTitleLabel.setBounds(area.removeFromTop(30).reduced(12, 0));
    midiProbeLabel.setBounds(area.removeFromTop(78).reduced(12, 0));
    auto buttonRow = area.removeFromTop(32).reduced(12, 0);
    resetMidiStatsButton.setBounds(buttonRow.removeFromLeft(180));

    area.removeFromTop(12);
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

    const auto midiProbe = processor.getMidiProbeSnapshot();
    juce::String midiText;
    midiText << "Pass-through: ACTIVE | events in/out: " << midiProbe.totalEvents << " / " << midiProbe.totalEvents
             << " | channels seen: " << channelsText(midiProbe.channelMask) << "\n";
    midiText << "Note On: " << midiProbe.noteOnEvents
             << " | Note Off: " << midiProbe.noteOffEvents
             << " | CC: " << midiProbe.controllerEvents
             << " | Pitch Bend: " << midiProbe.pitchBendEvents
             << " | Other: " << midiProbe.otherEvents << "\n";
    midiText << "Last: " << lastMidiEventText(midiProbe)
             << " | MIDI rev: " << midiProbe.revision;
    midiProbeLabel.setText(midiText, juce::dontSendNotification);

    juce::String debugText;
    debugText << "Техническая диагностика\n";
    debugText << "MIDI input/output: YES / YES | routing model under test: 1 VST3 Event Out -> MIDI channels 1-4\n";
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

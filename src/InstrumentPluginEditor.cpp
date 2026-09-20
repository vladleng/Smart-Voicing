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

juce::String counterText(std::uint32_t value)
{
    return juce::String(static_cast<juce::int64>(value));
}

juce::String noteText(int note)
{
    if (note < 0)
        return juce::String::fromUTF8("—");

    const auto name = juce::MidiMessage::getMidiNoteName(note, true, true, 3);
    return name + " (" + juce::String(note) + ")";
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
    titleLabel.setText("Smart Voicing 0.1b - Direct 4 Voice Router", juce::dontSendNotification);
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

    midiProbeTitleLabel.setText("MIDI Router 0.1b | Direct 4 Voice | V1->Ch1 ... V4->Ch4",
                                juce::dontSendNotification);
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

    setSize(900, 920);
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
    auto midiArea = area.removeFromTop(205);
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
    midiProbeLabel.setBounds(area.removeFromTop(132).reduced(12, 0));
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
    midiText << "Router: ACTIVE | events in/out: " << counterText(midiProbe.totalInputEvents)
             << " / " << counterText(midiProbe.totalOutputEvents)
             << " | input channels seen: " << channelsText(midiProbe.channelMask)
             << " | held: " << midiProbe.heldNoteCount << "\n";
    midiText << "V1/Ch1: " << noteText(midiProbe.voiceNotes[0])
             << " | V2/Ch2: " << noteText(midiProbe.voiceNotes[1]) << "\n";
    midiText << "V3/Ch3: " << noteText(midiProbe.voiceNotes[2])
             << " | V4/Ch4: " << noteText(midiProbe.voiceNotes[3]) << "\n";
    midiText << "Note On: " << counterText(midiProbe.noteOnEvents)
             << " | Note Off: " << counterText(midiProbe.noteOffEvents)
             << " | CC: " << counterText(midiProbe.controllerEvents)
             << " | Pitch Bend: " << counterText(midiProbe.pitchBendEvents)
             << " | Other: " << counterText(midiProbe.otherEvents) << "\n";
    midiText << "Last input: " << lastMidiEventText(midiProbe)
             << " | MIDI rev: " << counterText(midiProbe.revision);
    midiProbeLabel.setText(midiText, juce::dontSendNotification);

    juce::String debugText;
    debugText << "Техническая диагностика\n";
    debugText << "MIDI input/output: YES / YES | Direct 4 Voice: highest->Ch1, next->Ch2, next->Ch3, next->Ch4\n";
    debugText << "Channel messages: broadcast to Ch1-4 | >4 held notes: top four routed\n";
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

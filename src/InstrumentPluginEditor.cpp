#include "InstrumentPluginEditor.h"
#include "ChordModel.h"
#include "KeyModel.h"
#include "HarmonicFunction.h"
#include "TensionPolicy.h"
#include "TensionKeyswitch.h"
#include "VoicingKeyswitch.h"
#include "HarmonyModeKeyswitch.h"
#include "VoicingStrategy.h"
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

juce::String distributionModeText(SmartVoicingInstrumentProcessor::DistributionMode mode)
{
    using Mode = SmartVoicingInstrumentProcessor::DistributionMode;
    switch (mode)
    {
        case Mode::topDown:   return juce::String::fromUTF8("Сверху вниз");
        case Mode::bottomUp:  return juce::String::fromUTF8("Снизу вверх");
        case Mode::fillFour:  return juce::String::fromUTF8("Заполнить 4 голоса");
        default:              return "?";
    }
}

juce::String harmonyModeText(SmartVoicingInstrumentProcessor::HarmonyMode mode)
{
    using Mode = SmartVoicingInstrumentProcessor::HarmonyMode;
    switch (mode)
    {
        case Mode::directRouter:    return "Direct Router";
        case Mode::melodyHarmonize: return "Melody Harmonize";
        default:                    return "?";
    }
}

juce::String tensionLevelText(smartvoicing::harmony::TensionLevel level)
{
    juce::String result;
    result << static_cast<int>(level) << " - " << smartvoicing::harmony::tensionLevelName(level);
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
    titleLabel.setText("Smart Voicing 0.4f - Spread",
                       juce::dontSendNotification);
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

    harmonyModeLabel.setText(juce::String::fromUTF8("Режим:"), juce::dontSendNotification);
    harmonyModeLabel.setJustificationType(juce::Justification::centredLeft);
    harmonyModeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(harmonyModeLabel);

    harmonyModeBox.addItem("Direct Router", 1);
    harmonyModeBox.addItem("Melody Harmonize", 2);
    harmonyModeBox.setSelectedId(static_cast<int>(processor.getHarmonyMode()) + 1,
                                 juce::dontSendNotification);
    harmonyModeBox.onChange = [this]
    {
        const auto value = juce::jlimit(0, 1, harmonyModeBox.getSelectedId() - 1);
        processor.setHarmonyMode(static_cast<SmartVoicingInstrumentProcessor::HarmonyMode>(value));
        refreshContextMonitor();
    };
    addAndMakeVisible(harmonyModeBox);

    voicingTypeLabel.setText("Voicing Type:", juce::dontSendNotification);
    voicingTypeLabel.setJustificationType(juce::Justification::centredLeft);
    voicingTypeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(voicingTypeLabel);

    voicingTypeBox.addItem("Closed", static_cast<int>(smartvoicing::harmony::VoicingType::closed) + 1);
    voicingTypeBox.addItem("Drop 2", static_cast<int>(smartvoicing::harmony::VoicingType::drop2) + 1);
    voicingTypeBox.addItem("Drop 3", static_cast<int>(smartvoicing::harmony::VoicingType::drop3) + 1);
    voicingTypeBox.addItem("Drop 2+4", static_cast<int>(smartvoicing::harmony::VoicingType::drop24) + 1);
    voicingTypeBox.addItem("Spread", static_cast<int>(smartvoicing::harmony::VoicingType::spread) + 1);
    voicingTypeBox.addItem("Quartal", static_cast<int>(smartvoicing::harmony::VoicingType::quartal) + 1);
    voicingTypeBox.addItem("UST", static_cast<int>(smartvoicing::harmony::VoicingType::ust) + 1);
    voicingTypeBox.addItem("Cluster", static_cast<int>(smartvoicing::harmony::VoicingType::cluster) + 1);
    voicingTypeBox.addItem("Unison", static_cast<int>(smartvoicing::harmony::VoicingType::unison) + 1);
    voicingTypeBox.addItem("Octaves", static_cast<int>(smartvoicing::harmony::VoicingType::octaves) + 1);
    voicingTypeBox.addItem("Doubling", static_cast<int>(smartvoicing::harmony::VoicingType::doubling) + 1);
    voicingTypeBox.setSelectedId(static_cast<int>(processor.getVoicingType()) + 1,
                                 juce::dontSendNotification);
    voicingTypeBox.onChange = [this]
    {
        const auto value = juce::jlimit(
            static_cast<int>(smartvoicing::harmony::VoicingType::closed),
            static_cast<int>(smartvoicing::harmony::VoicingType::cluster),
            voicingTypeBox.getSelectedId() - 1);
        processor.setVoicingType(static_cast<smartvoicing::harmony::VoicingType>(value));
        refreshContextMonitor();
    };
    addAndMakeVisible(voicingTypeBox);

    tensionLevelLabel.setText("Tensions:", juce::dontSendNotification);
    tensionLevelLabel.setJustificationType(juce::Justification::centredLeft);
    tensionLevelLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(tensionLevelLabel);

    tensionLevelBox.addItem("1 - Clean", static_cast<int>(smartvoicing::harmony::TensionLevel::clean));
    tensionLevelBox.addItem("2 - Color", static_cast<int>(smartvoicing::harmony::TensionLevel::color));
    tensionLevelBox.addItem("3 - Rich", static_cast<int>(smartvoicing::harmony::TensionLevel::rich));
    tensionLevelBox.setSelectedId(static_cast<int>(processor.getTensionLevel()),
                                  juce::dontSendNotification);
    tensionLevelBox.onChange = [this]
    {
        const auto value = juce::jlimit(
            static_cast<int>(smartvoicing::harmony::TensionLevel::clean),
            static_cast<int>(smartvoicing::harmony::TensionLevel::rich),
            tensionLevelBox.getSelectedId());
        processor.setTensionLevel(static_cast<smartvoicing::harmony::TensionLevel>(value));
        refreshContextMonitor();
    };
    addAndMakeVisible(tensionLevelBox);

    distributionModeLabel.setText(juce::String::fromUTF8("Распределение:"), juce::dontSendNotification);
    distributionModeLabel.setJustificationType(juce::Justification::centredLeft);
    distributionModeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(distributionModeLabel);

    distributionModeBox.addItem(juce::String::fromUTF8("Сверху вниз"), 1);
    distributionModeBox.addItem(juce::String::fromUTF8("Снизу вверх"), 2);
    distributionModeBox.addItem(juce::String::fromUTF8("Заполнить 4 голоса"), 3);
    distributionModeBox.setSelectedId(static_cast<int>(processor.getDistributionMode()) + 1,
                                      juce::dontSendNotification);
    distributionModeBox.onChange = [this]
    {
        const auto value = juce::jlimit(0, 2, distributionModeBox.getSelectedId() - 1);
        processor.setDistributionMode(static_cast<SmartVoicingInstrumentProcessor::DistributionMode>(value));
        refreshContextMonitor();
    };
    addAndMakeVisible(distributionModeBox);

    diagnosticsButton.onClick = [this]
    {
        diagnosticsExpanded = ! diagnosticsExpanded;
        updateDiagnosticsVisibility();
    };
    addAndMakeVisible(diagnosticsButton);

    midiProbeTitleLabel.setText("MIDI Engine 0.4e | V1->Ch1 ... V4->Ch4",
                                juce::dontSendNotification);
    midiProbeTitleLabel.setJustificationType(juce::Justification::centredLeft);
    midiProbeTitleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    addAndMakeVisible(midiProbeTitleLabel);

    midiProbeLabel.setJustificationType(juce::Justification::topLeft);
    midiProbeLabel.setFont(juce::FontOptions(13.5f));
    addAndMakeVisible(midiProbeLabel);

    resetMidiStatsButton.setButtonText(juce::String::fromUTF8("Сбросить MIDI stats"));
    resetMidiStatsButton.onClick = [this]
    {
        processor.resetMidiProbeStatistics();
        refreshContextMonitor();
    };
    addAndMakeVisible(resetMidiStatsButton);

    debugLabel.setJustificationType(juce::Justification::topLeft);
    debugLabel.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(debugLabel);

    updateDiagnosticsVisibility();
    refreshContextMonitor();
    startTimerHz(8);
}

SmartVoicingInstrumentEditor::~SmartVoicingInstrumentEditor()
{
    stopTimer();
}

void SmartVoicingInstrumentEditor::updateDiagnosticsVisibility()
{
    diagnosticsButton.setButtonText(diagnosticsExpanded ? "Diagnostics [-]" : "Diagnostics [+]");

    midiProbeTitleLabel.setVisible(diagnosticsExpanded);
    midiProbeLabel.setVisible(diagnosticsExpanded);
    resetMidiStatsButton.setVisible(diagnosticsExpanded);
    debugLabel.setVisible(diagnosticsExpanded);

    setSize(editorWidth, diagnosticsExpanded ? expandedEditorHeight : compactEditorHeight);
    repaint();
}

void SmartVoicingInstrumentEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(88);

    auto contextArea = area.removeFromTop(228);
    g.setColour(getLookAndFeel().findColour(juce::Label::outlineColourId).withAlpha(0.35f));
    g.drawRoundedRectangle(contextArea.toFloat(), 8.0f, 1.0f);

    area.removeFromTop(12);
    auto modeArea = area.removeFromTop(168);
    g.drawRoundedRectangle(modeArea.toFloat(), 8.0f, 1.0f);

    area.removeFromTop(12);
    area.removeFromTop(32);

    if (diagnosticsExpanded)
    {
        area.removeFromTop(12);
        auto midiArea = area.removeFromTop(250);
        g.drawRoundedRectangle(midiArea.toFloat(), 8.0f, 1.0f);
    }
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

    auto harmonyRow = area.removeFromTop(42).reduced(12, 4);
    harmonyModeLabel.setBounds(harmonyRow.removeFromLeft(130));
    harmonyModeBox.setBounds(harmonyRow.removeFromLeft(280));

    auto voicingRow = area.removeFromTop(42).reduced(12, 4);
    voicingTypeLabel.setBounds(voicingRow.removeFromLeft(130));
    voicingTypeBox.setBounds(voicingRow.removeFromLeft(280));

    auto tensionRow = area.removeFromTop(42).reduced(12, 4);
    tensionLevelLabel.setBounds(tensionRow.removeFromLeft(130));
    tensionLevelBox.setBounds(tensionRow.removeFromLeft(280));

    auto distributionRow = area.removeFromTop(42).reduced(12, 4);
    distributionModeLabel.setBounds(distributionRow.removeFromLeft(130));
    distributionModeBox.setBounds(distributionRow.removeFromLeft(280));

    area.removeFromTop(12);
    diagnosticsButton.setBounds(area.removeFromTop(32).reduced(12, 0));

    if (! diagnosticsExpanded)
        return;

    area.removeFromTop(12);
    midiProbeTitleLabel.setBounds(area.removeFromTop(30).reduced(12, 0));
    midiProbeLabel.setBounds(area.removeFromTop(177).reduced(12, 0));
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

    const auto neutralContext = ppq >= 0.0
        ? harmonicContextProvider.contextAt(ppq)
        : harmonicContextProvider.currentContext();
    const auto normalizedChord = smartvoicing::harmony::normalizeChord(neutralContext.chord);
    const auto normalizedSymbol = smartvoicing::harmony::normalizedChordSymbol(normalizedChord);
    const auto normalizedKey = smartvoicing::harmony::normalizeKey(neutralContext.key);

    const auto nextChordPpq = ppq >= 0.0
        ? harmonicContextProvider.nextChordStartAfter(ppq)
        : -1.0;
    const auto nextContext = nextChordPpq >= 0.0
        ? harmonicContextProvider.contextAt(nextChordPpq)
        : smartvoicing::harmony::HarmonicContext {};
    const auto nextChord = smartvoicing::harmony::normalizeChord(nextContext.chord);
    const auto nextSymbol = smartvoicing::harmony::normalizedChordSymbol(nextChord);

    const auto harmonicAnalysis = nextChord.valid
        ? smartvoicing::harmony::analyzeHarmonicFunction(normalizedChord, normalizedKey, nextChord)
        : smartvoicing::harmony::analyzeHarmonicFunction(normalizedChord, normalizedKey);
    const auto tensionPolicy = smartvoicing::harmony::buildTensionPolicy(
        normalizedChord, normalizedKey, harmonicAnalysis);

    juce::String chord = "n/a";
    juce::String hostChord = "n/a";
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
            hostChord = smartvoicing::debug::chordText(context.sheetChords[chordIndex]);

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

    chord = normalizedChord.valid
        ? juce::String::fromUTF8(normalizedSymbol.c_str())
        : hostChord;

    chordLabel.setText(juce::String::fromUTF8("Аккорд: ") + chord,
                       juce::dontSendNotification);
    keyLabel.setText(juce::String::fromUTF8("Тональность: ") + key,
                     juce::dontSendNotification);
    timeSignatureLabel.setText(juce::String::fromUTF8("Размер: ") + timeSignature,
                               juce::dontSendNotification);
    tempoLabel.setText(juce::String::fromUTF8("Темп: ") + tempo,
                       juce::dontSendNotification);

    juce::String bridgeText;
    bridgeText << "Smart Voicing ARA: " << (context.connected ? "CONNECTED" : "WAITING")
               << " | Neutral provider: " << (neutralContext.providerConnected ? "READY" : "WAITING")
               << " | Chord model: " << (normalizedChord.valid ? "READY" : "N/A")
               << " | Key model: " << (normalizedKey.valid ? "READY" : "N/A")
               << " | Harmony rev: " << juce::String(static_cast<juce::int64>(neutralContext.harmonicRevision))
               << " | Transport rev: " << juce::String(static_cast<juce::int64>(neutralContext.transportRevision));
    bridgeLabel.setText(bridgeText, juce::dontSendNotification);

    juce::String positionText;
    positionText << juce::String::fromUTF8("Позиция: ");
    if (ppq >= 0.0)
        positionText << "PPQ " << juce::String(ppq, 6);
    else
        positionText << "n/a";

    if (seconds >= 0.0)
        positionText << " | " << juce::String(seconds, 6) << " sec";

    positionText << juce::String::fromUTF8(" | источник: ")
                 << (useBridgeTransport ? "Smart Voicing ARA" : "Instrument");
    if (context.transportAvailable)
        positionText << " | " << (context.transportPlaying ? "PLAY" : "STOP");

    positionLabel.setText(positionText, juce::dontSendNotification);

    const auto midiProbe = processor.getMidiProbeSnapshot();

    const auto desiredHarmonyId = static_cast<int>(midiProbe.harmonyMode) + 1;
    if (harmonyModeBox.getSelectedId() != desiredHarmonyId)
        harmonyModeBox.setSelectedId(desiredHarmonyId, juce::dontSendNotification);

    const auto desiredVoicingId = static_cast<int>(midiProbe.voicingType) + 1;
    if (voicingTypeBox.getSelectedId() != desiredVoicingId)
        voicingTypeBox.setSelectedId(desiredVoicingId, juce::dontSendNotification);

    const auto desiredTensionId = static_cast<int>(midiProbe.tensionLevel);
    if (tensionLevelBox.getSelectedId() != desiredTensionId)
        tensionLevelBox.setSelectedId(desiredTensionId, juce::dontSendNotification);

    const auto desiredDistributionId = static_cast<int>(midiProbe.distributionMode) + 1;
    if (distributionModeBox.getSelectedId() != desiredDistributionId)
        distributionModeBox.setSelectedId(desiredDistributionId, juce::dontSendNotification);

    const auto directRouter = midiProbe.harmonyMode == SmartVoicingInstrumentProcessor::HarmonyMode::directRouter;
    distributionModeBox.setEnabled(directRouter);
    distributionModeLabel.setEnabled(directRouter);
    voicingTypeBox.setEnabled(! directRouter);
    voicingTypeLabel.setEnabled(! directRouter);
    tensionLevelBox.setEnabled(! directRouter);
    tensionLevelLabel.setEnabled(! directRouter);

    juce::String midiText;
    midiText << "Mode: " << harmonyModeText(midiProbe.harmonyMode);
    if (directRouter)
        midiText << " | distribution: " << distributionModeText(midiProbe.distributionMode);
    else
        midiText << " | " << smartvoicing::harmony::voicingTypeName(midiProbe.voicingType)
                 << " Voicing | tension: " << tensionLevelText(midiProbe.tensionLevel)
                 << " | live Chord Track reharmonization";
    midiText << " | ownership: " << (midiProbe.stableOwnership ? "STABLE" : "FRAME") << "\n";
    midiText << "sustain: " << (midiProbe.sustainDown ? "DOWN" : "UP")
             << " | keys held: " << midiProbe.heldNoteCount
             << " | extra chord notes ignored: " << midiProbe.ignoredExtraNoteCount << "\n";
    midiText << "reharmonizations: " << counterText(midiProbe.reharmonizationCount);
    if (midiProbe.lastReharmonizationPpq >= 0.0)
        midiText << " | last PPQ: " << juce::String(midiProbe.lastReharmonizationPpq, 6);
    else
        midiText << " | last PPQ: n/a";
    midiText << "\n";
    midiText << "events in/out: " << counterText(midiProbe.totalInputEvents)
             << " / " << counterText(midiProbe.totalOutputEvents)
             << " | input channels seen: " << channelsText(midiProbe.channelMask) << "\n";
    midiText << "V1/Ch1: " << noteText(midiProbe.voiceNotes[0]) << " [stack " << midiProbe.voiceStackDepths[0] << "]"
             << " | V2/Ch2: " << noteText(midiProbe.voiceNotes[1]) << " [stack " << midiProbe.voiceStackDepths[1] << "]\n";
    midiText << "V3/Ch3: " << noteText(midiProbe.voiceNotes[2]) << " [stack " << midiProbe.voiceStackDepths[2] << "]"
             << " | V4/Ch4: " << noteText(midiProbe.voiceNotes[3]) << " [stack " << midiProbe.voiceStackDepths[3] << "]\n";
    midiText << "Note On: " << counterText(midiProbe.noteOnEvents)
             << " | Note Off: " << counterText(midiProbe.noteOffEvents)
             << " | CC: " << counterText(midiProbe.controllerEvents)
             << " | Pitch Bend: " << counterText(midiProbe.pitchBendEvents)
             << " | Other: " << counterText(midiProbe.otherEvents) << "\n";
    midiText << "Last input: " << lastMidiEventText(midiProbe)
             << " | MIDI rev: " << counterText(midiProbe.revision);
    midiProbeLabel.setText(midiText, juce::dontSendNotification);

    juce::String debugText;
    debugText << juce::String::fromUTF8("Техническая диагностика\n");
    debugText << "Stage 5 / 0.4f: Spread active | Voicing MIDI 32..42 | Tension 43..45 | Harmony Mode 46..47\n";
    debugText << "Neutral context: position " << (neutralContext.positionAvailable ? "YES" : "NO")
              << " | chord " << (neutralContext.chord.available ? (neutralContext.chord.defined ? "DEFINED" : "NO CHORD") : "N/A")
              << " | key " << (neutralContext.key.available ? "AVAILABLE" : "N/A")
              << " | time signature " << (neutralContext.timeSignature.available ? "AVAILABLE" : "N/A") << "\n";

    if (neutralContext.chord.available)
    {
        debugText << "Neutral chord: start PPQ " << juce::String(neutralContext.chord.startPpq, 6)
                  << " | root fifths " << neutralContext.chord.root
                  << " | bass fifths " << neutralContext.chord.bass
                  << " | interval mask ";
        for (int i = 0; i < smartvoicing::harmony::kPitchClassCount; ++i)
            if (neutralContext.chord.intervals.values[static_cast<std::size_t>(i)] != 0)
                debugText << i << " ";
        debugText << "\n";
    }

    debugText << "Chord model: " << (normalizedChord.valid ? "VALID" : "N/A")
              << " | symbol " << normalizedSymbol
              << " | quality " << smartvoicing::harmony::chordQualityName(normalizedChord.quality)
              << " | root PC " << normalizedChord.rootPitchClass
              << " | bass PC " << normalizedChord.bassPitchClass
              << " | slash " << (normalizedChord.slashBass ? "YES" : "NO")
              << " | ext flags " << normalizedChord.extensions
              << " | alt flags " << normalizedChord.alterations << "\n";

    debugText << "Key model: " << (normalizedKey.valid ? "VALID" : "N/A");
    if (normalizedKey.valid)
        debugText << " | root PC " << normalizedKey.rootPitchClass
                  << " | mode " << smartvoicing::harmony::keyModeName(normalizedKey.mode);
    debugText << "\n";

    debugText << "Function analysis: " << (harmonicAnalysis.valid ? "VALID" : "N/A");
    if (harmonicAnalysis.valid)
    {
        debugText << " | degree " << smartvoicing::harmony::scaleDegreeName(harmonicAnalysis.rootScaleDegree)
                  << " | root function " << smartvoicing::harmony::harmonicFunctionName(harmonicAnalysis.rootFunction)
                  << " | effective " << smartvoicing::harmony::harmonicFunctionName(harmonicAnalysis.effectiveFunction)
                  << " | relation " << smartvoicing::harmony::harmonicRelationName(harmonicAnalysis.relation);

        if (harmonicAnalysis.appliedDominantCandidate)
        {
            debugText << " | applied V/"
                      << smartvoicing::harmony::scaleDegreeName(harmonicAnalysis.appliedTargetScaleDegree)
                      << " " << (harmonicAnalysis.appliedDominantConfirmed ? "CONFIRMED" : "candidate");
        }

        if (harmonicAnalysis.dominantResolutionConfirmed)
        {
            debugText << " | dominant target CONFIRMED: PC "
                      << harmonicAnalysis.dominantTargetPitchClass
                      << " / "
                      << smartvoicing::harmony::chordQualityName(harmonicAnalysis.dominantTargetQuality);
        }

        if (harmonicAnalysis.modalInterchangeCandidate)
        {
            debugText << " | modal interchange candidate: parallel "
                      << smartvoicing::harmony::keyModeName(harmonicAnalysis.modalInterchangeSource);
        }
    }
    debugText << "\n";

    debugText << "Functional tension: "
              << smartvoicing::harmony::functionalTensionProfileName(tensionPolicy.functionalProfile)
              << " | resolution " << (tensionPolicy.resolutionConfirmed ? "CONFIRMED" : "not confirmed")
              << "\n";

    debugText << "Resolution context: ";
    if (nextChord.valid)
        debugText << "next @ PPQ " << juce::String(nextChordPpq, 6)
                  << " = " << nextSymbol
                  << " | root PC " << nextChord.rootPitchClass
                  << " | quality " << smartvoicing::harmony::chordQualityName(nextChord.quality);
    else
        debugText << "no next chord -> unresolved, no target inference";
    debugText << "\n";

    debugText << "Host chord text: " << hostChord << " | NormalizedChord is authoritative\n";
    debugText << "Priority: Melody > Explicit Chord > Characteristic tones > Key > Function/Real Target > Functional Profile > Tension Level > Strategy\n";
    debugText << "Harmony mode: " << harmonyModeText(midiProbe.harmonyMode)
              << " | Voicing Type: " << smartvoicing::harmony::voicingTypeName(midiProbe.voicingType)
              << " | Tension Level: " << tensionLevelText(midiProbe.tensionLevel)
              << " | V1 melody immutable\n";
    debugText << "Voicing keyswitches: 32=UST(R), 33=Cluster(R), 34=Quartal(R), 35=Spread(R), 36=Closed, 37=Drop2, 38=Drop3, 39=Drop2+4, 40=Unison, 41=Octaves, 42=Doubling\n";
    debugText << "Tension keyswitches: MIDI "
              << smartvoicing::harmony::kCleanTensionKeyswitchNote << "=Clean, "
              << smartvoicing::harmony::kColorTensionKeyswitchNote << "=Color, "
              << smartvoicing::harmony::kRichTensionKeyswitchNote << "=Rich; swallowed in Melody Harmonize\n";
    debugText << "Harmony Mode keyswitches: MIDI "
              << smartvoicing::harmony::kDirectRouterModeKeyswitchNote << "=Direct Router, "
              << smartvoicing::harmony::kMelodyHarmonizeModeKeyswitchNote << "=Melody Harmonize; swallowed in both modes\n";
    debugText << "Closed policy: guide + characteristic tones, contextual omissions, soft Upper Voice Spacing\n";
    debugText << "Drop 2 policy: same Closed pitch classes; second voice from top lowered one octave; slash bass falls back to Closed\n";
    debugText << "Drop 3 policy: same Closed pitch classes; third voice from top lowered one octave; slash bass falls back to Closed\n";
    debugText << "Drop 2+4 policy: same Closed pitch classes; second and fourth voices lowered one octave; slash bass pitch class remains authoritative\n";
    debugText << "Unison policy: V1-V4 same performer melody pitch on independent MIDI channels; no harmonic generation\n";
    debugText << "Octaves policy: V1=melody, V2/V3=melody-12, V4=melody-24; out-of-range lower voices inactive\n";
    debugText << "Doubling policy: V1/V2=melody, V3/V4=melody-12; two independent unison pairs, no harmonic generation\n";
    debugText << "Tension levels: Clean=structural | Color=target-aware inside colour | Rich=functionally intensified tension\n";
    debugText << "Tension policy: Explicit authoritative; Avoid/Unavailable excluded from generated V2-V4\n";
    debugText << "Resolution policy: ONLY actual next Chord root+quality drives target-aware dominant profile\n";
    debugText << "Live reharmonization count: " << counterText(midiProbe.reharmonizationCount)
              << " | chord boundaries are scheduled inside the current audio block\n";
    debugText << "MIDI input/output: YES / YES | Direct Router 0.2 remains available\n";
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

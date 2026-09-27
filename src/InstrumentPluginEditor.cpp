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
using VoicingType = smartvoicing::harmony::VoicingType;
constexpr std::array<VoicingType, 11> kVoicingOrder {
    VoicingType::closed, VoicingType::drop2, VoicingType::drop3,
    VoicingType::drop24, VoicingType::spread, VoicingType::quartal,
    VoicingType::ust, VoicingType::cluster, VoicingType::unison,
    VoicingType::octaves, VoicingType::doubling
};

const juce::Colour background { 0xff17191c }, surface { 0xff24272a },
    panel { 0xff292c2f }, border { 0xff404549 }, muted { 0xff95a0a4 },
    accent { 0xff94c8d5 }, selected { 0xff35454b };

void drawSection(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(panel);
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(border);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
}

void drawCaption(juce::Graphics& g, const juce::String& caption, int x, int y,
                 int width = 250)
{
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(caption, x, y, width, 18, juce::Justification::centredLeft);
}

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
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe5e9e9));
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

void SmartVoicingChoiceButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    const bool active = getToggleState();
    if (active || highlighted || down)
    {
        g.setColour(active ? selected : panel.brighter(0.08f));
        g.fillRoundedRectangle(bounds, 3.0f);
    }

    const auto alpha = isEnabled() ? 1.0f : 0.42f;
    g.setColour((active ? accent : juce::Colour(0xff687377)).withMultipliedAlpha(alpha));
    g.drawEllipse(12.5f, bounds.getCentreY() - 5.5f, 11.0f, 11.0f, 1.3f);
    if (active)
        g.fillEllipse(15.8f, bounds.getCentreY() - 2.2f, 4.4f, 4.4f);

    g.setColour((active ? juce::Colour(0xfff1f4f4) : juce::Colour(0xffb9c2c5))
                    .withMultipliedAlpha(alpha));
    g.setFont(juce::FontOptions(14.0f, active ? juce::Font::bold : juce::Font::plain));
    g.drawText(getButtonText(), 37, 0, detail.isEmpty() ? getWidth() - 110 : 110,
               getHeight(), juce::Justification::centredLeft);
    if (detail.isNotEmpty())
    {
        g.setColour(muted.withMultipliedAlpha(alpha));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(detail, 147, 0, getWidth() - 215, getHeight(),
                   juce::Justification::centredLeft);
    }
    g.setColour((active ? accent : muted.darker(0.25f)).withMultipliedAlpha(alpha));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(midiLabel, getWidth() - 67, 0, 48, getHeight(),
               juce::Justification::centredRight);
}

SmartVoicingInstrumentEditor::SmartVoicingInstrumentEditor(SmartVoicingInstrumentProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    titleLabel.setText("HARMA WAVES", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    vendorLabel.setText("Moon River Studio", juce::dontSendNotification);
    vendorLabel.setJustificationType(juce::Justification::centredRight);
    vendorLabel.setFont(juce::FontOptions(11.0f));
    vendorLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(vendorLabel);

    bridgeLabel.setJustificationType(juce::Justification::centredLeft);
    bridgeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    bridgeLabel.setColour(juce::Label::textColourId, accent);
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

    harmonyModeLabel.setText(juce::String::fromUTF8("РЕЖИМ РАБОТЫ"), juce::dontSendNotification);
    harmonyModeLabel.setJustificationType(juce::Justification::centredLeft);
    harmonyModeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    harmonyModeLabel.setColour(juce::Label::textColourId, muted);
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

    const std::array<juce::String, 11> voicingNames {
        "Closed", "Drop 2", "Drop 3", "Drop 2+4", "Spread", "Quartal",
        "UST", "Cluster", "Unison", "Octaves", "Doubling"
    };
    const std::array<juce::String, 11> voicingKeys {
        "C1", "C#1", "D1", "D#1", "B0", "A#0", "G#0", "A0", "E1", "F1", "F#1"
    };
    for (std::size_t i = 0; i < voicingButtons.size(); ++i)
    {
        voicingButtons[i] = std::make_unique<SmartVoicingChoiceButton>(voicingNames[i], voicingKeys[i]);
        voicingButtons[i]->onClick = [this, i]
        {
            processor.setVoicingType(kVoicingOrder[i]);
            refreshContextMonitor();
        };
        addAndMakeVisible(*voicingButtons[i]);
    }

    const std::array<juce::String, 3> tensionNames { "Clean", "Color", "Rich" };
    const std::array<juce::String, 3> tensionKeys { "G1", "G#1", "A1" };
    const std::array<juce::String, 3> tensionDetails {
        juce::String::fromUTF8("структура"),
        juce::String::fromUTF8("умеренный цвет"),
        juce::String::fromUTF8("выраженное напряжение")
    };
    for (std::size_t i = 0; i < tensionButtons.size(); ++i)
    {
        tensionButtons[i] = std::make_unique<SmartVoicingChoiceButton>(
            tensionNames[i], tensionKeys[i], tensionDetails[i]);
        tensionButtons[i]->onClick = [this, i]
        {
            processor.setTensionLevel(static_cast<smartvoicing::harmony::TensionLevel>(i + 1));
            refreshContextMonitor();
        };
        addAndMakeVisible(*tensionButtons[i]);
    }

    distributionModeLabel.setText(juce::String::fromUTF8("РАСПРЕДЕЛЕНИЕ"), juce::dontSendNotification);
    distributionModeLabel.setJustificationType(juce::Justification::centredLeft);
    distributionModeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    distributionModeLabel.setColour(juce::Label::textColourId, muted);
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

    for (auto* box : { &harmonyModeBox, &distributionModeBox })
    {
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff34383b));
        box->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff50575b));
        box->setColour(juce::ComboBox::textColourId, juce::Colour(0xffe4e8e8));
        box->setColour(juce::ComboBox::arrowColourId, muted);
    }

    diagnosticsButton.onClick = [this]
    {
        diagnosticsExpanded = ! diagnosticsExpanded;
        updateDiagnosticsVisibility();
    };
    addAndMakeVisible(diagnosticsButton);
    diagnosticsButton.setColour(juce::TextButton::buttonColourId, panel);
    diagnosticsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb4bdc0));

    midiProbeTitleLabel.setText("MIDI Engine 0.5 | V1->Ch1 ... V4->Ch4",
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
    diagnosticsButton.setButtonText(diagnosticsExpanded ? "▾  DIAGNOSTICS" : "▸  DIAGNOSTICS");

    positionLabel.setVisible(diagnosticsExpanded);
    midiProbeTitleLabel.setVisible(diagnosticsExpanded);
    midiProbeLabel.setVisible(diagnosticsExpanded);
    resetMidiStatsButton.setVisible(diagnosticsExpanded);
    debugLabel.setVisible(diagnosticsExpanded);

    setSize(editorWidth, diagnosticsExpanded ? expandedEditorHeight : compactEditorHeight);
    repaint();
}

void SmartVoicingInstrumentEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(surface);
    g.fillRoundedRectangle(26.0f, 22.0f, 1148.0f,
                           static_cast<float>(getHeight() - 45), 8.0f);
    g.setColour(border);
    g.drawRoundedRectangle(26.0f, 22.0f, 1148.0f,
                           static_cast<float>(getHeight() - 45), 8.0f, 1.0f);
    g.setColour(juce::Colour(0xff2d3033));
    g.fillRect(27, 23, 1146, 67);
    g.setColour(border);
    g.drawHorizontalLine(90, 27.0f, 1173.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(51.0f, 43.0f, 4.0f, 29.0f, 2.0f);
    drawCaption(g, "0.5  /  MIDI ARRANGER", 315, 46);

    drawCaption(g, juce::String::fromUTF8("ТЕКУЩИЙ КОНТЕКСТ"), 53, 105);
    drawSection(g, { 51.0f, 137.0f, 1098.0f, 104.0f });
    g.setColour(border);
    for (int x : { 303, 572, 841 })
        g.drawVerticalLine(x, 155.0f, 223.0f);
    const std::array<juce::String, 4> captions {
        juce::String::fromUTF8("АККОРД"), juce::String::fromUTF8("ТОНАЛЬНОСТЬ"),
        juce::String::fromUTF8("РАЗМЕР"), juce::String::fromUTF8("ТЕМП")
    };
    for (int i = 0; i < 4; ++i)
        drawCaption(g, captions[static_cast<std::size_t>(i)],
                    std::array<int, 4> { 74, 326, 595, 864 }[static_cast<std::size_t>(i)], 145);

    drawCaption(g, juce::String::fromUTF8("ГАРМОНИЗАЦИЯ"), 53, 258);
    drawSection(g, { 51.0f, 289.0f, 1098.0f, 384.0f });
    g.setColour(border);
    g.drawVerticalLine(588, 305.0f, 655.0f);
    g.drawHorizontalLine(337, 73.0f, 564.0f);
    g.drawHorizontalLine(337, 611.0f, 1127.0f);
    g.drawHorizontalLine(523, 611.0f, 1127.0f);
    drawCaption(g, juce::String::fromUTF8("ВОЙСИНГ"), 73, 306);
    drawCaption(g, "TENSIONS", 611, 306);
    drawCaption(g, "MIDI", 520, 306, 50);
    drawCaption(g, "MIDI", 1090, 306, 48);

    drawSection(g, { 51.0f, 686.0f, 1098.0f, 35.0f });
    drawCaption(g, "V1–V4  →  CH 1–4", 1000, 693, 130);
    if (diagnosticsExpanded)
        drawSection(g, { 51.0f, 735.0f, 1098.0f, 890.0f });
}

void SmartVoicingInstrumentEditor::resized()
{
    titleLabel.setBounds(69, 40, 242, 34);
    vendorLabel.setBounds(953, 35, 191, 22);
    bridgeLabel.setBounds(973, 57, 171, 25);
    chordLabel.setBounds(74, 174, 215, 48);
    keyLabel.setBounds(326, 174, 235, 48);
    timeSignatureLabel.setBounds(595, 174, 230, 48);
    tempoLabel.setBounds(864, 174, 250, 48);

    for (std::size_t i = 0; i < voicingButtons.size(); ++i)
        voicingButtons[i]->setBounds(69, 341 + static_cast<int>(i) * 27, 499, 27);
    for (std::size_t i = 0; i < tensionButtons.size(); ++i)
        tensionButtons[i]->setBounds(607, 342 + static_cast<int>(i) * 50, 524, 44);

    harmonyModeLabel.setBounds(611, 531, 255, 24);
    harmonyModeBox.setBounds(611, 562, 516, 38);
    distributionModeLabel.setBounds(611, 610, 154, 28);
    distributionModeBox.setBounds(766, 611, 361, 28);
    diagnosticsButton.setBounds(63, 689, 220, 29);
    if (! diagnosticsExpanded)
        return;
    positionLabel.setBounds(73, 750, 1050, 28);
    midiProbeTitleLabel.setBounds(73, 786, 1050, 30);
    midiProbeLabel.setBounds(73, 820, 1050, 180);
    resetMidiStatsButton.setBounds(73, 1005, 180, 32);
    debugLabel.setBounds(73, 1050, 1050, 560);
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

    chordLabel.setText(chord, juce::dontSendNotification);
    keyLabel.setText(key, juce::dontSendNotification);
    timeSignatureLabel.setText(timeSignature, juce::dontSendNotification);
    tempoLabel.setText(tempo, juce::dontSendNotification);

    juce::String bridgeText = context.connected ? "●  ARA CONNECTED" : "○  ARA WAITING";
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

    for (std::size_t i = 0; i < voicingButtons.size(); ++i)
        voicingButtons[i]->setToggleState(midiProbe.voicingType == kVoicingOrder[i],
                                          juce::dontSendNotification);
    for (std::size_t i = 0; i < tensionButtons.size(); ++i)
        tensionButtons[i]->setToggleState(
            static_cast<int>(midiProbe.tensionLevel) == static_cast<int>(i + 1),
            juce::dontSendNotification);

    const auto desiredDistributionId = static_cast<int>(midiProbe.distributionMode) + 1;
    if (distributionModeBox.getSelectedId() != desiredDistributionId)
        distributionModeBox.setSelectedId(desiredDistributionId, juce::dontSendNotification);

    const auto directRouter = midiProbe.harmonyMode == SmartVoicingInstrumentProcessor::HarmonyMode::directRouter;
    distributionModeBox.setEnabled(directRouter);
    distributionModeLabel.setEnabled(directRouter);
    for (auto& button : voicingButtons)
        button->setEnabled(! directRouter);
    for (auto& button : tensionButtons)
        button->setEnabled(! directRouter);

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
    debugText << "Stage 5 / 0.5: 8 harmonic types + 3 melodic textures | Voicing MIDI 32..42 | Tension 43..45 | Harmony Mode 46..47\n";
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

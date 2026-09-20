#include "ARAContextDocumentController.h"
#include "ARAContextDebugState.h"
#include "SharedHarmonicContext.h"

#if JucePlugin_Enable_ARA

#include <ARA_Library/PlugIn/ARAPlug.h>

#include <algorithm>
#include <cstring>

namespace
{
template <std::size_t size>
void copyARAName(const char* source, char (&destination)[size]) noexcept
{
    destination[0] = '\0';

    if (source == nullptr || size == 0)
        return;

    const auto bytesToCopy = (std::min<std::size_t>)(std::strlen(source), size - 1);
    std::memcpy(destination, source, bytesToCopy);
    destination[bytesToCopy] = '\0';
}

template <typename SourceArray>
void copyIntervals(const SourceArray& source, std::uint8_t (&destination)[12]) noexcept
{
    for (int i = 0; i < 12; ++i)
        destination[i] = static_cast<std::uint8_t>(source[i]);
}

void publishBestSharedSnapshot()
{
    const auto best = ARAContextDebugState::instance().getSnapshot();
    SharedHarmonicContextBridge::instance().publish(best.harmonicContext);
}
}

SmartVoicingARADocumentController::SmartVoicingARADocumentController(
    const ARA::PlugIn::PlugInEntry* entry,
    const ARA::ARADocumentControllerHostInstance* instance)
    : juce::ARADocumentControllerSpecialisation(entry, instance)
{
    refreshDebugSnapshot();
}

SmartVoicingARADocumentController::~SmartVoicingARADocumentController()
{
    auto& state = ARAContextDebugState::instance();
    state.removeSource(this);
    publishBestSharedSnapshot();
}

bool SmartVoicingARADocumentController::doRestoreObjectsFromStream(
    juce::ARAInputStream& input,
    const juce::ARARestoreObjectsFilter* filter)
{
    juce::ignoreUnused(input, filter);
    return true;
}

bool SmartVoicingARADocumentController::doStoreObjectsToStream(
    juce::ARAOutputStream& output,
    const juce::ARAStoreObjectsFilter* filter)
{
    juce::ignoreUnused(output, filter);
    return true;
}

void SmartVoicingARADocumentController::didAddMusicalContextToDocument(
    juce::ARADocument* document,
    juce::ARAMusicalContext* musicalContext)
{
    juce::ignoreUnused(document, musicalContext);
    refreshDebugSnapshot();
}

void SmartVoicingARADocumentController::didUpdateMusicalContextProperties(
    juce::ARAMusicalContext* musicalContext)
{
    juce::ignoreUnused(musicalContext);
    refreshDebugSnapshot();
}

void SmartVoicingARADocumentController::doUpdateMusicalContextContent(
    juce::ARAMusicalContext* musicalContext,
    juce::ARAContentUpdateScopes scopeFlags)
{
    juce::ignoreUnused(musicalContext, scopeFlags);
    refreshDebugSnapshot();
}

void SmartVoicingARADocumentController::didReorderMusicalContextsInDocument(
    juce::ARADocument* document)
{
    juce::ignoreUnused(document);
    refreshDebugSnapshot();
}

void SmartVoicingARADocumentController::refreshDebugSnapshot()
{
    ARAContextDebugSnapshot snapshot;
    snapshot.documentControllerCreated = true;

    auto* documentController = getDocumentController();
    snapshot.hostContentAccessAvailable =
        documentController != nullptr
        && documentController->getHostContentAccessController() != nullptr;

    SharedHarmonicContextSnapshot shared;
    shared.hostContentAccessAvailable = snapshot.hostContentAccessAvailable;

    auto* document = getDocument();
    if (document == nullptr)
    {
        snapshot.harmonicContext = shared;
        auto& state = ARAContextDebugState::instance();
        state.publishSnapshot(this, snapshot);
        publishBestSharedSnapshot();
        return;
    }

    const auto& contexts = document->getMusicalContexts();
    snapshot.musicalContextCount = static_cast<int>(contexts.size());
    shared.musicalContextCount = snapshot.musicalContextCount;

    if (contexts.empty())
    {
        snapshot.harmonicContext = shared;
        auto& state = ARAContextDebugState::instance();
        state.publishSnapshot(this, snapshot);
        publishBestSharedSnapshot();
        return;
    }

    // Most hosts expose one project-level Musical Context. If a host exposes
    // several contexts, choose the one with the richest host content for this
    // proof of concept instead of merging independent timelines.
    const juce::ARAMusicalContext* selectedContext = nullptr;
    int selectedScore = -1;

    for (const auto* context : contexts)
    {
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeKeySignatures> keyReader(context);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeSheetChords> chordReader(context);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempoReader(context);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeBarSignatures> barReader(context);

        const auto availableTypes = static_cast<int>(static_cast<bool>(keyReader))
                                  + static_cast<int>(static_cast<bool>(chordReader))
                                  + static_cast<int>(static_cast<bool>(tempoReader))
                                  + static_cast<int>(static_cast<bool>(barReader));

        const auto eventCount = keyReader.getEventCount()
                              + chordReader.getEventCount()
                              + tempoReader.getEventCount()
                              + barReader.getEventCount();

        const auto score = availableTypes * 100000 + eventCount;
        if (score > selectedScore)
        {
            selectedContext = context;
            selectedScore = score;
        }
    }

    if (selectedContext != nullptr)
    {
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeKeySignatures> keyReader(selectedContext);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeSheetChords> chordReader(selectedContext);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempoReader(selectedContext);
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeBarSignatures> barReader(selectedContext);

        snapshot.keySignaturesAvailable = static_cast<bool>(keyReader);
        snapshot.keySignatureEventCount = keyReader.getEventCount();
        shared.keySignaturesAvailable = snapshot.keySignaturesAvailable;
        shared.keySignatureEventCount = snapshot.keySignatureEventCount;
        shared.keySignatureStoredCount = (std::min)(snapshot.keySignatureEventCount,
                                                    kSmartVoicingMaxKeyEvents);

        for (int i = 0; i < shared.keySignatureStoredCount; ++i)
        {
            const auto event = keyReader.getDataForEvent(i);
            auto& destination = shared.keySignatures[i];
            destination.position = event.position;
            destination.root = event.root;
            copyIntervals(event.intervals, destination.intervals);
            copyARAName(event.name, destination.name);
        }

        snapshot.sheetChordsAvailable = static_cast<bool>(chordReader);
        snapshot.sheetChordEventCount = chordReader.getEventCount();
        shared.sheetChordsAvailable = snapshot.sheetChordsAvailable;
        shared.sheetChordEventCount = snapshot.sheetChordEventCount;
        shared.sheetChordStoredCount = (std::min)(snapshot.sheetChordEventCount,
                                                  kSmartVoicingMaxChordEvents);

        for (int i = 0; i < shared.sheetChordStoredCount; ++i)
        {
            const auto event = chordReader.getDataForEvent(i);
            auto& destination = shared.sheetChords[i];
            destination.position = event.position;
            destination.root = event.root;
            destination.bass = event.bass;
            copyIntervals(event.intervals, destination.intervals);
            copyARAName(event.name, destination.name);
        }

        snapshot.tempoEntriesAvailable = static_cast<bool>(tempoReader);
        snapshot.tempoEntryEventCount = tempoReader.getEventCount();
        shared.tempoEntriesAvailable = snapshot.tempoEntriesAvailable;
        shared.tempoEntryEventCount = snapshot.tempoEntryEventCount;
        shared.tempoEntryStoredCount = (std::min)(snapshot.tempoEntryEventCount,
                                                  kSmartVoicingMaxTempoEvents);

        for (int i = 0; i < shared.tempoEntryStoredCount; ++i)
        {
            const auto event = tempoReader.getDataForEvent(i);
            shared.tempoEntries[i].timePosition = event.timePosition;
            shared.tempoEntries[i].quarterPosition = event.quarterPosition;
        }

        snapshot.barSignaturesAvailable = static_cast<bool>(barReader);
        snapshot.barSignatureEventCount = barReader.getEventCount();
        shared.barSignaturesAvailable = snapshot.barSignaturesAvailable;
        shared.barSignatureEventCount = snapshot.barSignatureEventCount;
        shared.barSignatureStoredCount = (std::min)(snapshot.barSignatureEventCount,
                                                    kSmartVoicingMaxBarEvents);

        for (int i = 0; i < shared.barSignatureStoredCount; ++i)
        {
            const auto event = barReader.getDataForEvent(i);
            shared.barSignatures[i].position = event.position;
            shared.barSignatures[i].numerator = event.numerator;
            shared.barSignatures[i].denominator = event.denominator;
        }
    }

    shared.connected = shared.hostContentAccessAvailable
                    && shared.musicalContextCount > 0
                    && (shared.keySignaturesAvailable
                        || shared.sheetChordsAvailable
                        || shared.tempoEntriesAvailable
                        || shared.barSignaturesAvailable);

    snapshot.harmonicContext = shared;

    auto& state = ARAContextDebugState::instance();
    state.publishSnapshot(this, snapshot);
    publishBestSharedSnapshot();
}

#endif

#include "ARAContextDocumentController.h"
#include "ARAContextDebugState.h"

#if JucePlugin_Enable_ARA

#include <ARA_Library/PlugIn/ARAPlug.h>

SmartVoicingARADocumentController::SmartVoicingARADocumentController(
    const ARA::PlugIn::PlugInEntry* entry,
    const ARA::ARADocumentControllerHostInstance* instance)
    : juce::ARADocumentControllerSpecialisation(entry, instance)
{
    refreshDebugSnapshot();
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

    auto* document = getDocument();
    if (document == nullptr)
    {
        ARAContextDebugState::instance().setSnapshot(snapshot);
        return;
    }

    const auto& contexts = document->getMusicalContexts();
    snapshot.musicalContextCount = static_cast<int>(contexts.size());

    for (const auto* context : contexts)
    {
        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeKeySignatures> keyReader(context);
        snapshot.keySignaturesAvailable = snapshot.keySignaturesAvailable || static_cast<bool>(keyReader);
        snapshot.keySignatureEventCount += keyReader.getEventCount();

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeSheetChords> chordReader(context);
        snapshot.sheetChordsAvailable = snapshot.sheetChordsAvailable || static_cast<bool>(chordReader);
        snapshot.sheetChordEventCount += chordReader.getEventCount();

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempoReader(context);
        snapshot.tempoEntriesAvailable = snapshot.tempoEntriesAvailable || static_cast<bool>(tempoReader);
        snapshot.tempoEntryEventCount += tempoReader.getEventCount();

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeBarSignatures> barReader(context);
        snapshot.barSignaturesAvailable = snapshot.barSignaturesAvailable || static_cast<bool>(barReader);
        snapshot.barSignatureEventCount += barReader.getEventCount();
    }

    ARAContextDebugState::instance().setSnapshot(snapshot);
}

#endif

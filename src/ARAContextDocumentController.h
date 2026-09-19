#pragma once

#include <JuceHeader.h>

#if JucePlugin_Enable_ARA

class SmartVoicingARADocumentController final : public juce::ARADocumentControllerSpecialisation
{
public:
    SmartVoicingARADocumentController(const ARA::PlugIn::PlugInEntry* entry,
                                      const ARA::ARADocumentControllerHostInstance* instance);

protected:
    bool doRestoreObjectsFromStream(juce::ARAInputStream& input,
                                    const juce::ARARestoreObjectsFilter* filter) override;
    bool doStoreObjectsToStream(juce::ARAOutputStream& output,
                                const juce::ARAStoreObjectsFilter* filter) override;

    void didAddMusicalContextToDocument(juce::ARADocument* document,
                                        juce::ARAMusicalContext* musicalContext) override;
    void didUpdateMusicalContextProperties(juce::ARAMusicalContext* musicalContext) override;
    void doUpdateMusicalContextContent(juce::ARAMusicalContext* musicalContext,
                                       juce::ARAContentUpdateScopes scopeFlags) override;
    void didReorderMusicalContextsInDocument(juce::ARADocument* document) override;

private:
    void refreshDebugSnapshot();
};

#endif

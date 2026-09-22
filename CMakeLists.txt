cmake_minimum_required(VERSION 3.22)

project(SmartVoicing
    VERSION 0.3.6
    DESCRIPTION "Smart Voicing - ARA-aware MIDI harmonizer and voice allocator"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(
    ARA_SDK
    GIT_REPOSITORY https://github.com/Celemony/ARA_SDK.git
    GIT_TAG releases/2.3.0
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_GetProperties(ARA_SDK)
if(NOT ara_sdk_POPULATED)
    FetchContent_Populate(ARA_SDK)
endif()

juce_set_ara_sdk_path("${ara_sdk_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# Main plug-in: Instrument / MIDI engine.
# It deliberately does not request ARA from the host. Harmonic context is read
# from the lightweight bridge published by Smart Voicing ARA.
juce_add_plugin(SmartVoicing
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.smartvoicing"
    PLUGIN_MANUFACTURER_CODE MNRV
    PLUGIN_CODE SmVo
    FORMATS VST3
    PRODUCT_NAME "Smart Voicing"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(SmartVoicing
    PRIVATE
        src/InstrumentPluginProcessor.cpp
        src/InstrumentPluginProcessor.h
        src/InstrumentPluginEditor.cpp
        src/InstrumentPluginEditor.h
        src/SharedHarmonicContext.cpp
        src/SharedHarmonicContext.h
        src/HarmonicContextProvider.cpp
        src/HarmonicContextProvider.h
        src/HarmonicContextDebugText.cpp
        src/HarmonicContextDebugText.h
        src/ChordModel.cpp
        src/ChordModel.h
        src/KeyModel.cpp
        src/KeyModel.h
        src/HarmonicFunction.cpp
        src/HarmonicFunction.h
        src/TensionPolicy.cpp
        src/TensionPolicy.h
        src/CloseVoicingHarmonizer.cpp
        src/CloseVoicingHarmonizer.h
        src/LiveReharmonizer.cpp
        src/LiveReharmonizer.h
        src/VoiceAllocator.cpp
        src/VoiceAllocator.h
)

target_compile_definitions(SmartVoicing
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SmartVoicing
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_events
        juce::juce_core
)

# -----------------------------------------------------------------------------
# Companion ARA plug-in: host timeline/content reader.
juce_add_plugin(SmartVoicingARA
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.smartvoicing.ara"
    PLUGIN_MANUFACTURER_CODE MNRV
    PLUGIN_CODE SmVA
    FORMATS VST3
    PRODUCT_NAME "Smart Voicing ARA"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_AUTO_MANIFEST FALSE
    ARA_FACTORY_ID "com.moonriverstudio.smartvoicing.ara.factory"
    ARA_DOCUMENT_ARCHIVE_ID "com.moonriverstudio.smartvoicing.ara.document"
    ARA_COMPATIBLE_ARCHIVE_IDS "com.moonriverstudio.smartvoicing.ara.document"
    ARA_CONTENT_TYPES Notes TempoEntries BarSignatures StaticTuning KeySignatures SheetChords
    ARA_TRANSFORMATION_FLAGS Timestretch ReflectTimingContent ReflectTuningContent
)

target_sources(SmartVoicingARA
    PRIVATE
        src/ARAPluginProcessor.cpp
        src/ARAPluginProcessor.h
        src/ARAPluginEditor.cpp
        src/ARAPluginEditor.h
        src/ARAPlaybackRenderer.cpp
        src/ARAPlaybackRenderer.h
        src/ARAModelObjects.cpp
        src/ARAModelObjects.h
        src/ARAContentReader.cpp
        src/ARAContentReader.h
        src/SharedHarmonicContext.cpp
        src/SharedHarmonicContext.h
        src/HarmonicContextDebugText.cpp
        src/HarmonicContextDebugText.h
)

target_compile_definitions(SmartVoicingARA
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SmartVoicingARA
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_events
        juce::juce_core
)

# -----------------------------------------------------------------------------
# Host-neutral regression targets.
add_executable(SmartVoicingCoreTests
    tests/VoiceAllocatorTests.cpp
    src/VoiceAllocator.cpp
)
target_include_directories(SmartVoicingCoreTests PRIVATE src)

add_executable(SmartVoicingHarmonizerTests
    tests/CloseVoicingHarmonizerTests.cpp
    src/ChordModel.cpp
    src/KeyModel.cpp
    src/HarmonicFunction.cpp
    src/TensionPolicy.cpp
    src/CloseVoicingHarmonizer.cpp
)
target_include_directories(SmartVoicingHarmonizerTests PRIVATE src)

add_executable(SmartVoicingLiveReharmonizerTests
    tests/LiveReharmonizerTests.cpp
    src/ChordModel.cpp
    src/KeyModel.cpp
    src/HarmonicFunction.cpp
    src/TensionPolicy.cpp
    src/CloseVoicingHarmonizer.cpp
    src/LiveReharmonizer.cpp
)
target_include_directories(SmartVoicingLiveReharmonizerTests PRIVATE src)

add_executable(SmartVoicingKeyAwareTests
    tests/KeyAwareTests.cpp
    src/ChordModel.cpp
    src/KeyModel.cpp
    src/HarmonicFunction.cpp
)
target_include_directories(SmartVoicingKeyAwareTests PRIVATE src)

add_executable(SmartVoicingTensionPolicyTests
    tests/TensionPolicyTests.cpp
    src/ChordModel.cpp
    src/KeyModel.cpp
    src/HarmonicFunction.cpp
    src/TensionPolicy.cpp
)
target_include_directories(SmartVoicingTensionPolicyTests PRIVATE src)

if(MSVC)
    target_compile_options(SmartVoicingCoreTests PRIVATE /utf-8)
    target_compile_options(SmartVoicingHarmonizerTests PRIVATE /utf-8)
    target_compile_options(SmartVoicingLiveReharmonizerTests PRIVATE /utf-8)
    target_compile_options(SmartVoicingKeyAwareTests PRIVATE /utf-8)
    target_compile_options(SmartVoicingTensionPolicyTests PRIVATE /utf-8)
endif()

add_test(NAME SmartVoicingCoreTests COMMAND SmartVoicingCoreTests)
add_test(NAME SmartVoicingHarmonizerTests COMMAND SmartVoicingHarmonizerTests)
add_test(NAME SmartVoicingLiveReharmonizerTests COMMAND SmartVoicingLiveReharmonizerTests)
add_test(NAME SmartVoicingKeyAwareTests COMMAND SmartVoicingKeyAwareTests)
add_test(NAME SmartVoicingTensionPolicyTests COMMAND SmartVoicingTensionPolicyTests)

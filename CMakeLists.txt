cmake_minimum_required(VERSION 3.22)

project(SmartVoicing
    VERSION 0.1.4
    DESCRIPTION "Smart Voicing - ARA-aware MIDI harmonizer and voice allocator"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

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
    VST3_CATEGORIES Instrument
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    IS_ARA_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(SmartVoicing)

target_sources(SmartVoicing
    PRIVATE
        src/SharedHarmonicContext.h
        src/HarmonicContextDebugText.h
        src/InstrumentPluginProcessor.cpp
        src/InstrumentPluginProcessor.h
        src/InstrumentPluginEditor.cpp
        src/InstrumentPluginEditor.h
)

# -----------------------------------------------------------------------------
# Helper plug-in: ARA / Event FX context reader.
# It passes audio through unchanged and only publishes DAW harmonic context.
juce_add_plugin(SmartVoicingARA
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.smartvoicingara"
    PLUGIN_MANUFACTURER_CODE MNRV
    PLUGIN_CODE SmAr
    FORMATS VST3
    PRODUCT_NAME "Smart Voicing ARA"
    IS_SYNTH FALSE
    VST3_CATEGORIES Fx
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_ARA_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(SmartVoicingARA)

target_sources(SmartVoicingARA
    PRIVATE
        src/SharedHarmonicContext.h
        src/HarmonicContextDebugText.h
        src/ARAContextDebugState.h
        src/ARAContextDocumentController.cpp
        src/ARAContextDocumentController.h
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h
)

foreach(target SmartVoicing SmartVoicingARA)
    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endforeach()

cmake_minimum_required(VERSION 3.22)

project(SmartVoicing
    VERSION 0.0.1
    DESCRIPTION "Smart Voicing — ARA-aware MIDI harmonizer and voice allocator"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# JUCE: фиксируем стабильную версию, чтобы сборки были воспроизводимыми.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# ARA SDK: официальный публичный release Celemony.
# На Этапе 0 SDK подключается к сборочной системе, а сам ARA Document Controller
# будет реализован на Этапе 1 (ARA Context Proof of Concept).
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

juce_add_plugin(SmartVoicing
    COMPANY_NAME "Moon River"
    BUNDLE_ID "com.moonriver.smartvoicing"
    PLUGIN_MANUFACTURER_CODE MnRv
    PLUGIN_CODE SmVo
    FORMATS VST3
    PRODUCT_NAME "Smart Voicing"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(SmartVoicing)

target_sources(SmartVoicing
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginProcessor.h
        src/PluginEditor.cpp
        src/PluginEditor.h
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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

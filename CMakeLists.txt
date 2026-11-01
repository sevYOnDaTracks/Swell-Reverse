cmake_minimum_required(VERSION 3.22)

# Mac : binaire universel (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
endif()

project(SwellVerb VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

set(SWELL_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SWELL_FORMATS AU)
endif()

juce_add_plugin(SwellVerb
    COMPANY_NAME "SevY Audio"
    BUNDLE_ID com.sevyaudio.swell
    PLUGIN_MANUFACTURER_CODE Sevy
    PLUGIN_CODE Swl1
    FORMATS ${SWELL_FORMATS}
    PRODUCT_NAME "Swell Reverse"
    DESCRIPTION "Reverse reverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Reverb)

set(SWELL_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/IRBuilder.cpp
    Source/SwellRenderer.cpp)

juce_add_binary_data(SwellAssets SOURCES
    Resources/logo_s.png
    Resources/logo_word.png)

set(SWELL_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    SWELL_VERSION="${PROJECT_VERSION}")

target_sources(SwellVerb PRIVATE ${SWELL_SOURCES})
target_compile_definitions(SwellVerb PUBLIC ${SWELL_DEFS})
target_link_libraries(SwellVerb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        SwellAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Banc de test hors-ligne (rendu WAV), optionnel
option(SWELL_BUILD_TESTS "Build offline render test" OFF)
if(SWELL_BUILD_TESTS)
    juce_add_console_app(SwellTest PRODUCT_NAME "SwellTest")
    target_sources(SwellTest PRIVATE Tests/RenderTest.cpp ${SWELL_SOURCES})
    target_compile_definitions(SwellTest PRIVATE ${SWELL_DEFS} JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(SwellTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            SwellAssets
            juce::juce_recommended_config_flags)
endif()

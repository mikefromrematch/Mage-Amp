cmake_minimum_required(VERSION 3.22)
project(MAGE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
)
FetchContent_MakeAvailable(JUCE)

# Build one JUCE plugin project with its generated Standalone app and VST3.
# JUCE's Standalone wrapper also provides audio-device setup for the app.
juce_add_plugin(MAGE
    COMPANY_NAME "MAGE Audio"
    PRODUCT_NAME "MAGE"
    PLUGIN_MANUFACTURER_CODE MgAu
    PLUGIN_CODE Mage
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

juce_generate_juce_header(MAGE)

target_sources(MAGE PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(MAGE PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(MAGE PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

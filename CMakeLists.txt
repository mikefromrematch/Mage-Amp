cmake_minimum_required(VERSION 3.22)
project(MAGE VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
)
FetchContent_MakeAvailable(JUCE)

# One JUCE plugin project generates both the VST3 and the standalone app.
# The standalone wrapper also provides the audio-device setup window.
juce_add_plugin(MAGE
    COMPANY_NAME "MAGE Audio"
    PRODUCT_NAME "MAGE"
    BUNDLE_ID "com.mageaudio.mage"
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
    src/PresetManager.cpp
    src/ui/Widgets.cpp
)

target_include_directories(MAGE PRIVATE src)

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

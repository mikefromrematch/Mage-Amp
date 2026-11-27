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
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    # Standalone app: macOS gives silence from every audio input (interfaces included)
    # unless the app declares why it needs the input.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "MAGE needs audio input access so you can play your guitar through it."
)

juce_generate_juce_header(MAGE)

target_sources(MAGE PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/PresetManager.cpp
    src/MidiLearn.cpp
    src/ui/Widgets.cpp
)

target_include_directories(MAGE PRIVATE src)

target_compile_definitions(MAGE PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

juce_add_binary_data(MageAssets SOURCES
    assets/mage_mark_ink.png assets/mage_mark_glow.png
    assets/mage_wordmark_ink.png assets/mage_wordmark_glow.png
    assets/spell_sigil_ink.png assets/spell_sigil_glow.png)

target_link_libraries(MAGE PRIVATE
    MageAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# MAGE Spell: Glitch / Pitch / Swell multi-effect, same suite and shared UI code.
juce_add_plugin(MAGESpell
    COMPANY_NAME "MAGE Audio"
    PRODUCT_NAME "MAGE Spell"
    BUNDLE_ID "com.mageaudio.spell"
    PLUGIN_MANUFACTURER_CODE MgAu
    PLUGIN_CODE Mspl
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "MAGE Spell needs audio input access so you can play your guitar through it."
)

juce_generate_juce_header(MAGESpell)

target_sources(MAGESpell PRIVATE
    src/spell/SpellProcessor.cpp
    src/spell/SpellEditor.cpp
    src/PresetManager.cpp
    src/MidiLearn.cpp
    src/ui/Widgets.cpp
)

target_include_directories(MAGESpell PRIVATE src)

target_compile_definitions(MAGESpell PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(MAGESpell PRIVATE
    MageAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# JUCE ad-hoc signs each bundle and only then writes moduleinfo.json into the VST3
# (and touches the standalone app's resources), so the bundles come out with a broken
# signature that macOS and some DAWs reject. Re-sign each one as the very last
# post-build step; these run after JUCE's own steps because they are added later.
if(APPLE)
    foreach(bundle_target IN ITEMS MAGE_VST3 MAGE_Standalone MAGESpell_VST3 MAGESpell_Standalone)
        add_custom_command(TARGET ${bundle_target} POST_BUILD
            COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${bundle_target}>"
            COMMAND codesign --verify --deep --strict "$<TARGET_BUNDLE_DIR:${bundle_target}>"
            VERBATIM)
    endforeach()
endif()

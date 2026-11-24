#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MageKnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider&) override;
};

class MageAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MageAudioProcessorEditor(MageAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void chooseCabinetIR();

    MageAudioProcessor& processor;
    MageKnobLookAndFeel knobLookAndFeel;
    juce::Label title, subtitle;
    juce::Slider controls[23];
    juce::Label labels[23];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[23];
    juce::ToggleButton toggles[5];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachments[5];
    juce::TextButton pedalSwitches[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pedalSwitchAttachments[4];
    juce::TextButton loadIRButton { "LOAD IR" };
    juce::TextButton clearIRButton { "BYPASS IR" };
    juce::Label irStatus;
    std::unique_ptr<juce::FileChooser> irFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};

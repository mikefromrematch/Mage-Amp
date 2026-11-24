#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MageAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MageAudioProcessorEditor(MageAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MageAudioProcessor& processor;
    juce::Label title, subtitle;
    juce::Slider controls[23];
    juce::Label labels[23];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[23];
    juce::ToggleButton toggles[5];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachments[5];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};

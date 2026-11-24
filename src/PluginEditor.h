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
    juce::Slider controls[21];
    juce::Label labels[21];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[21];
    juce::ToggleButton toggles[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachments[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};

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
    juce::Label title;
    juce::Label subtitle;
    juce::Slider controls[11];
    juce::Label labels[11];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[11];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};

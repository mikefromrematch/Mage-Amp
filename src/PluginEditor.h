#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class MageAudioProcessorEditor final:public juce::AudioProcessorEditor{
public: explicit MageAudioProcessorEditor(MageAudioProcessor&); void paint(juce::Graphics&)override; void resized()override;
private: MageAudioProcessor& p; juce::Label title; juce::Slider knobs[7]; juce::Label labels[7]; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[7]; juce::TextButton settings{"⚙"}; JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessorEditor)
};

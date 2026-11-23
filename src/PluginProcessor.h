#pragma once
#include <JuceHeader.h>
class MageAudioProcessor final : public juce::AudioProcessor {
public:
 MageAudioProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "MAGE"; }
 bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; } bool isMidiEffect() const override { return false; }
 double getTailLengthSeconds() const override { return 2.0; }
 int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; } void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; } void changeProgramName(int,const juce::String&) override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
 juce::dsp::ProcessSpec spec{}; juce::dsp::Reverb reverb; juce::dsp::DelayLine<float,juce::dsp::DelayLineInterpolationTypes::Linear> delay{48000};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessor)
};

#pragma once
#include <JuceHeader.h>
#include "dsp/SpellDSP.h"
#include "MidiLearn.h"
#include "PresetManager.h"

// MAGE Spell: Glitch, Pitch and Swell in a reorderable chain, blended with a morph
// pad and performed with the CAST button.
class SpellAudioProcessor final : public juce::AudioProcessor
{
public:
    SpellAudioProcessor();
    ~SpellAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MAGE Spell"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String stepID(int step) { return "glitchStep" + juce::String(step + 1); }
    // Chain orders, indexed by the "order" parameter. 0 = Glitch, 1 = Pitch, 2 = Swell.
    static constexpr int orders[6][3] = { { 0, 1, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 0, 2, 1 }, { 1, 0, 2 }, { 2, 1, 0 } };

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState state;
    mage::PresetManager presets;
    mage::MidiLearn midiLearn;

    // For the UI.
    std::atomic<float> hostBpm { 120.0f };
    std::atomic<int> glitchStep { -1 };
    std::atomic<bool> casting { false };
    std::atomic<float> outputPeak { 0.0f };

private:
    float value(const char* id) const { return state.getRawParameterValue(id)->load(); }

    mage::spell::Glitch glitch;
    mage::spell::Pitch pitch;
    mage::spell::Swell swell;
    std::array<juce::SmoothedValue<float>, 3> enabled;
    juce::SmoothedValue<float> mixS, outS;
    juce::AudioBuffer<float> dry, scratch;
    double sampleRate = 44100.0;
    bool lastCast = false;
    double burstLeft = 0.0;   // samples of CAST left in Burst mode

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpellAudioProcessor)
};

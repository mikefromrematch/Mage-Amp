#pragma once
#include <JuceHeader.h>
#include "dsp/AmpDSP.h"
#include "PresetManager.h"
#include "MidiLearn.h"
#include "Pedals.h"

class MageAudioProcessor final : public juce::AudioProcessor
{
public:
    MageAudioProcessor();
    ~MageAudioProcessor() override = default;

    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MAGE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& delayDivisionNames();
    static const juce::StringArray& channelNames();   // { "Arcane", "Fae" }

    // Declared before `state` so it exists when the tree is built.
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState state;
    mage::PresetManager presets;
    mage::MidiLearn midiLearn { *this };   // CC -> parameter map (right-click a control to learn)

    // Cabinet impulse response (message thread only).
    bool loadCabinetIR(const juce::File& file);
    void bypassCabinetIR();
    juce::String getCabinetIRName() const { return cabinetIRName; }
    bool hasCabinetIR() const { return cabinetIRLoaded.load(); }

    // Metering, read by the editor's timer.
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    std::atomic<bool> gateIsOpen { false };

private:
    struct Params; // cached raw parameter pointers
    std::unique_ptr<Params> p;

    void updateFilters(bool force);
    // Pedalboard: run one pedal in place (R == nullptr means mono), with a short
    // crossfade when it's switched on or off.
    void runPedal(int type, float* L, float* R, int n);
    void processPedal(mage::Pedal, float* L, float* R, int n);
    void processFreeze(float* L, float* R, int n);
    void resetPedalState(int type);

    using Biquad = mage::dsp::Biquad;
    static constexpr int oversamplingOrder = 2; // 2^2 = 4x

    double sampleRate = 44100.0;
    int maxBlock = 512;

    juce::dsp::Oversampling<float> oversampler { 1, oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };

    juce::AudioBuffer<float> mono, gateGains;

    mage::dsp::NoiseGate gate;
    mage::dsp::OctavePedal octave;
    mage::dsp::DrivePedal drive;
    mage::dsp::Preamp preamp;
    Biquad bass, middle, treble, presence, depth;   // run at oversampled rate
    mage::dsp::DCBlocker powerDC;
    mage::dsp::CleanPreamp cleanPreamp;             // Fae channel (oversampled)
    Biquad cleanPresence, cleanDepth;
    mage::dsp::DCBlocker cleanPowerDC;
    mage::dsp::FreezePedal freeze;                  // base rate
    mage::dsp::ShimmerReverb shimmer;
    mage::dsp::Phaser phaser;
    mage::dsp::ChorusVibrato chorus;
    mage::dsp::Compressor compressor;               // front of the chain
    // The drive pedal can sit anywhere on the board, so it has its own 4x oversampling.
    juce::dsp::Oversampling<float> driveOversampler { 1, oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    juce::AudioBuffer<float> scratch;               // dry L, dry R, mono mid
    std::array<juce::SmoothedValue<float>, mage::numPedals> pedalOn;
    std::array<int, mage::numPedals> placement {};  // 0 = off the board, 1 = before the amp, 2 = after
    std::array<bool, mage::numPedals> pedalWasOn {};
    juce::SmoothedValue<float> compOnS;
    mage::dsp::CabSim cabSim;
    Biquad lowCut1, lowCut2, highCut1, highCut2;     // base rate
    juce::dsp::Convolution cabinetIR;
    juce::String cabinetIRPath, cabinetIRName;
    std::atomic<bool> cabinetIRLoaded { false };

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 96000 };
    mage::dsp::OnePole delayDamp[2];
    juce::dsp::Reverb reverb;

    juce::SmoothedValue<float> stage1Gain, stage2Gain, powerDrive;      // oversampled rate
    juce::SmoothedValue<float> channelMix, cleanGain, cleanPower, cleanMakeup;      // oversampled rate
    juce::SmoothedValue<float> shimmerMixS, freezeMixS;                // base rate
    juce::SmoothedValue<float> outputGain, octaveMixS, delayMixS, delayFeedbackS, delaySamplesS; // base rate

    // Last values used to compute filter coefficients (avoid recomputing every block).
    float lastBass = -1, lastMid = -1, lastTreble = -1, lastPresence = -1, lastDepth = -1,
          lastLowCut = -1, lastHighCut = -1, lastDriveTone = -1, lastOctTone = -1,
          lastCleanBass = -1, lastCleanMid = -1, lastCleanTreble = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MageAudioProcessor)
};

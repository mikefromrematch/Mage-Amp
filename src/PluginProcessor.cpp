#include "PluginProcessor.h"
#include "PluginEditor.h"

MageAudioProcessor::MageAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "MAGE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MageAudioProcessor::createLayout()
{
    using FloatParameter = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout parameters;
    auto addFloat = [&parameters](const char* id, float minimum, float maximum, float initial)
    {
        parameters.add(std::make_unique<FloatParameter>(id, id, minimum, maximum, initial));
    };

    addFloat("gain", 0.0f, 10.0f, 6.0f);
    addFloat("master", 0.0f, 10.0f, 5.0f);
    addFloat("bass", 0.0f, 10.0f, 5.0f);
    addFloat("mid", 0.0f, 10.0f, 5.0f);
    addFloat("treble", 0.0f, 10.0f, 5.0f);
    addFloat("presence", 0.0f, 10.0f, 5.0f);
    addFloat("depth", 0.0f, 10.0f, 5.0f);
    addFloat("gate", -90.0f, 0.0f, -45.0f);
    addFloat("drive", 0.0f, 10.0f, 0.0f);
    addFloat("delayMix", 0.0f, 1.0f, 0.0f);
    addFloat("reverbMix", 0.0f, 1.0f, 0.0f);
    return parameters;
}

void MageAudioProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    spec = { sampleRate,
             static_cast<juce::uint32>(blockSize),
             static_cast<juce::uint32>(getTotalNumOutputChannels()) };

    delay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.0));
    delay.prepare(spec);
    delay.reset();
    reverb.prepare(spec);
    reverb.reset();
}

void MageAudioProcessor::releaseResources()
{
    delay.reset();
    reverb.reset();
}

bool MageAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet() == output
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void MageAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto gain = state.getRawParameterValue("gain")->load();
    const auto master = state.getRawParameterValue("master")->load();
    const auto drive = state.getRawParameterValue("drive")->load();
    const auto delayMix = state.getRawParameterValue("delayMix")->load();
    const auto reverbMix = state.getRawParameterValue("reverbMix")->load();

    delay.setDelay(static_cast<float>(spec.sampleRate * 0.35));

    juce::dsp::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = 0.5f;
    reverbParameters.wetLevel = reverbMix;
    reverbParameters.dryLevel = 1.0f;
    reverb.setParameters(reverbParameters);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto processed = std::tanh(samples[sample] * (1.0f + gain * 0.8f + drive));
            processed *= juce::Decibels::decibelsToGain((master - 5.0f) * 2.0f);

            const auto delayed = delay.popSample(channel);
            delay.pushSample(channel, processed + delayed * 0.25f);
            samples[sample] = processed * (1.0f - delayMix) + delayed * delayMix;
        }
    }

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    reverb.process(context);
}

void MageAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    juce::MemoryOutputStream stream(destination, false);
    tree.writeToStream(stream);
}

void MageAudioProcessor::setStateInformation(const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(size));
    if (tree.isValid())
        state.replaceState(tree);
}

juce::AudioProcessorEditor* MageAudioProcessor::createEditor()
{
    return new MageAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MageAudioProcessor();
}

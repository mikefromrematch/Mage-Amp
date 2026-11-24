#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    float dbToGain(float db) { return juce::Decibels::decibelsToGain(db); }
    float knobDb(float value) { return (value - 5.0f) * 2.4f; }
}

MageAudioProcessor::MageAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "MAGE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MageAudioProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto add = [&p](const char* id, float lo, float hi, float initial, const char* unit = "")
    {
        p.add(std::make_unique<Float>(juce::ParameterID { id, 1 }, id,
                                      juce::NormalisableRange<float>(lo, hi), initial, unit));
    };
    auto toggle = [&p](const char* id, const char* name, bool initial)
    {
        p.add(std::make_unique<Bool>(juce::ParameterID { id, 1 }, name, initial));
    };

    // Amplifier controls
    add("gain", 0.0f, 10.0f, 5.8f);
    add("master", 0.0f, 10.0f, 5.0f);
    add("bass", 0.0f, 10.0f, 5.0f);
    add("mid", 0.0f, 10.0f, 5.0f);
    add("treble", 0.0f, 10.0f, 5.0f);
    add("presence", 0.0f, 10.0f, 5.0f);
    add("depth", 0.0f, 10.0f, 5.0f);

    // Noise gate
    add("gate", -90.0f, 0.0f, -55.0f, " dB");
    add("gateDecay", 5.0f, 500.0f, 90.0f, " ms");
    toggle("gateEnabled", "Gate Enabled", true);
    toggle("gatePost", "Gate Post Amp", false);

    // Octave and drive pedals
    add("octaveMix", 0.0f, 1.0f, 0.0f);
    add("octaveTone", 0.0f, 10.0f, 5.0f);
    add("octaveLevel", 0.0f, 10.0f, 5.0f);
    toggle("octaveEnabled", "Octave Enabled", false);
    toggle("octaveDown", "Octave Down", false);
    add("drive", 0.0f, 10.0f, 0.0f);
    toggle("driveEnabled", "Drive Enabled", false);
    add("driveTone", 0.0f, 10.0f, 5.0f);
    add("driveLevel", 0.0f, 10.0f, 5.0f);
    toggle("driveBoost", "Drive Boost", false);

    // Delay
    add("delayTime", 40.0f, 1200.0f, 350.0f, " ms");
    toggle("delayEnabled", "Delay Enabled", false);
    add("delayFeedback", 0.0f, 0.90f, 0.25f);
    add("delayMix", 0.0f, 1.0f, 0.0f);
    toggle("delaySync", "Delay Sync", false);

    // Reverb
    add("reverbDecay", 0.1f, 0.98f, 0.55f);
    toggle("reverbEnabled", "Reverb Enabled", false);
    add("reverbSize", 0.0f, 1.0f, 0.55f);
    add("reverbTone", 0.0f, 1.0f, 0.65f);
    add("reverbMix", 0.0f, 1.0f, 0.0f);

    // Cabinet filtering and final output
    add("lowCut", 20.0f, 500.0f, 70.0f, " Hz");
    add("highCut", 3000.0f, 20000.0f, 12000.0f, " Hz");
    add("output", 0.0f, 10.0f, 7.0f);
    return p;
}

void MageAudioProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    spec = { sampleRate, static_cast<juce::uint32>(blockSize),
             static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels())) };

    for (int ch = 0; ch < 2; ++ch)
    {
        lowCut[ch].reset(); bass[ch].reset(); middle[ch].reset(); treble[ch].reset();
        presence[ch].reset(); depth[ch].reset(); highCut[ch].reset(); driveTone[ch].reset();
        gateEnvelope[ch] = 0.0f;
        octavePrevious[ch] = 0.0f;
        octavePolarity[ch] = false;
    }

    delay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 1.3));
    delay.prepare(spec);
    delay.reset();
    reverb.prepare(spec);
    reverb.reset();
    cabinetIR.prepare(spec);
    if (cabinetIRPath.isNotEmpty())
    {
        const juce::File irFile(cabinetIRPath);
        if (irFile.existsAsFile())
            loadCabinetIR(irFile);
    }
}

void MageAudioProcessor::releaseResources()
{
    delay.reset();
    reverb.reset();
    cabinetIR.reset();
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
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const auto value = [this](const char* id) { return state.getRawParameterValue(id)->load(); };
    const float gain = value("gain");
    const float master = value("master");
    const float output = value("output");
    const float bassDb = knobDb(value("bass"));
    const float midDb = knobDb(value("mid"));
    const float trebleDb = knobDb(value("treble"));
    const float presenceDb = knobDb(value("presence"));
    const float depthDb = knobDb(value("depth"));

    // Tone stack: broad low/high shelves plus musical mid and presence bands.
    for (int ch = 0; ch < channels; ++ch)
    {
        lowCut[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, value("lowCut"));
        bass[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(spec.sampleRate, 120.0, 0.707,
                                                                                  dbToGain(bassDb));
        middle[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(spec.sampleRate, 750.0, 0.8,
                                                                                      dbToGain(midDb));
        treble[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(spec.sampleRate, 4200.0, 0.707,
                                                                                     dbToGain(trebleDb));
        presence[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(spec.sampleRate, 3200.0, 0.9,
                                                                                        dbToGain(presenceDb));
        depth[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(spec.sampleRate, 85.0, 0.8,
                                                                                    dbToGain(depthDb));
        highCut[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate, value("highCut"));
        driveTone[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(spec.sampleRate,
                                                                                      900.0 + value("driveTone") * 1100.0);
    }

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit(0.0f, 1.0f, value("reverbSize") * 0.65f + value("reverbDecay") * 0.35f);
    rp.damping = 1.0f - value("reverbTone");
    rp.wetLevel = value("reverbMix");
    rp.dryLevel = 1.0f;
    rp.width = 1.0f;
    reverb.setParameters(rp);

    const float delayMs = value("delayTime");
    delay.setDelay(static_cast<float>(spec.sampleRate * delayMs / 1000.0));
    const float feedback = value("delayFeedback");
    const float delayMix = value("delayMix");
    const float octaveMix = value("octaveMix");
    const float driveAmount = value("drive");
    const float driveGain = 1.0f + driveAmount * 1.15f;
    const float driveOutput = dbToGain((value("driveLevel") - 5.0f) * 2.0f);
    const bool boost = value("driveBoost") > 0.5f;
    const bool driveEnabled = value("driveEnabled") > 0.5f;
    const bool octaveEnabled = value("octaveEnabled") > 0.5f;
    const bool delayEnabled = value("delayEnabled") > 0.5f;
    const bool reverbEnabled = value("reverbEnabled") > 0.5f;
    const bool octaveDown = value("octaveDown") > 0.5f;
    const bool gateEnabled = value("gateEnabled") > 0.5f;
    const bool gatePost = value("gatePost") > 0.5f;
    const float threshold = dbToGain(value("gate"));
    const float gateRelease = std::exp(-1.0f / (static_cast<float>(spec.sampleRate) *
                                                value("gateDecay") * 0.001f));
    const float ampDrive = 1.0f + gain * 1.1f;

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = samples[i];

            // Pre-amplifier gate, with a smoothed envelope to avoid clicks.
            if (gateEnabled && !gatePost)
            {
                const float magnitude = std::abs(x);
                gateEnvelope[ch] = magnitude > gateEnvelope[ch]
                    ? magnitude : gateRelease * gateEnvelope[ch] + (1.0f - gateRelease) * magnitude;
                const float gateGain = gateEnvelope[ch] >= threshold ? 1.0f
                    : juce::jlimit(0.0f, 1.0f, gateEnvelope[ch] / juce::jmax(0.00001f, threshold));
                x *= gateGain;
            }

            x = lowCut[ch].processSample(x);
            x = depth[ch].processSample(x);
            x = bass[ch].processSample(x);
            x = middle[ch].processSample(x);
            x = treble[ch].processSample(x);
            x = presence[ch].processSample(x);

            // Drive pedal: asymmetrical soft clipping with a user-controlled tone filter.
            {
                const float pedalDrive = driveEnabled ? driveGain * (boost ? 1.35f : 1.0f) : 1.0f;
                const float preDrive = x * ampDrive * pedalDrive;
                float driven = std::tanh(preDrive) * (1.0f + (driveEnabled ? driveAmount * 0.08f : 0.0f));
                if (driveEnabled)
                    driven = driveTone[ch].processSample(driven) * driveOutput;
                x = driven;
            }

            // Octave pedal: selectable sub-octave divider or rectified upper harmonic.
            if (octaveEnabled && octaveMix > 0.0001f)
            {
                float octave = 0.0f;
                if (octaveDown)
                {
                    if (octavePrevious[ch] <= 0.0f && x > 0.0f)
                        octavePolarity[ch] = !octavePolarity[ch];
                    octave = octavePolarity[ch] ? -std::abs(x) : std::abs(x);
                }
                else
                {
                    octave = std::abs(x) * 2.0f - 0.35f;
                }
                const float octaveTone = 0.5f + value("octaveTone") * 0.1f;
                const float octaveLevel = dbToGain((value("octaveLevel") - 5.0f) * 2.0f);
                x = x * (1.0f - octaveMix) + octave * octaveMix * octaveTone * octaveLevel;
            }
            octavePrevious[ch] = x;

            // Post-amplifier gate option.
            if (gateEnabled && gatePost)
            {
                const float magnitude = std::abs(x);
                gateEnvelope[ch] = magnitude > gateEnvelope[ch]
                    ? magnitude : gateRelease * gateEnvelope[ch] + (1.0f - gateRelease) * magnitude;
                const float gateGain = gateEnvelope[ch] >= threshold ? 1.0f
                    : juce::jlimit(0.0f, 1.0f, gateEnvelope[ch] / juce::jmax(0.00001f, threshold));
                x *= gateGain;
            }

            const float delayed = delay.popSample(ch);
            delay.pushSample(ch, x + delayed * feedback);
            if (delayEnabled)
                x = x * (1.0f - delayMix) + delayed * delayMix;

            samples[i] = x;
        }
    }

    // Cabinet high-frequency roll-off and ambience are after the pedal chain.
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            samples[i] = highCut[ch].processSample(samples[i]);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    if (cabinetIRLoaded.load())
        cabinetIR.process(context);
    if (reverbEnabled)
        reverb.process(context);

    const float masterGain = dbToGain((master - 5.0f) * 2.0f);
    const float outputGain = dbToGain((output - 7.0f) * 2.0f);
    buffer.applyGain(masterGain * outputGain);
}

void MageAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    tree.setProperty("cabinetIRPath", cabinetIRPath, nullptr);
    juce::MemoryOutputStream stream(destination, false);
    tree.writeToStream(stream);
}

void MageAudioProcessor::setStateInformation(const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(size));
    if (tree.isValid())
    {
        state.replaceState(tree);
        cabinetIRPath = tree.getProperty("cabinetIRPath").toString();
        if (cabinetIRPath.isNotEmpty())
        {
            const juce::File irFile(cabinetIRPath);
            if (irFile.existsAsFile())
                loadCabinetIR(irFile);
            else
                cabinetIRLoaded.store(false);
        }
    }
}

bool MageAudioProcessor::loadCabinetIR(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    const auto extension = file.getFileExtension().toLowerCase();
    if (extension != ".wav" && extension != ".aif" && extension != ".aiff")
        return false;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr || reader->numChannels < 1 || reader->numChannels > 2)
        return false;

    const auto stereoMode = reader->numChannels > 1
        ? juce::dsp::Convolution::Stereo::yes
        : juce::dsp::Convolution::Stereo::no;
    cabinetIR.loadImpulseResponse(file, stereoMode, juce::dsp::Convolution::Trim::yes, 0,
                                  juce::dsp::Convolution::Normalise::yes);
    cabinetIRPath = file.getFullPathName();
    cabinetIRName = file.getFileName();
    cabinetIRLoaded.store(true);
    return true;
}

void MageAudioProcessor::bypassCabinetIR()
{
    cabinetIR.reset();
    cabinetIRLoaded.store(false);
    cabinetIRPath.clear();
    cabinetIRName = "IR BYPASSED";
}

juce::AudioProcessorEditor* MageAudioProcessor::createEditor()
{
    return new MageAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MageAudioProcessor();
}

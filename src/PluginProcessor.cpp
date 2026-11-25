#include "PluginProcessor.h"
#include "PluginEditor.h"

using mage::dsp::dbToGain;

namespace
{
    // Amp-style 0..10 knob mapped to +/- range in dB around noon.
    float knobDb(float value, float range = 12.0f) { return (value - 5.0f) * (range / 5.0f); }
}

struct MageAudioProcessor::Params
{
    explicit Params(juce::AudioProcessorValueTreeState& s)
    {
        auto get = [&s](const char* id) { auto* v = s.getRawParameterValue(id); jassert(v != nullptr); return v; };
        gain = get("gain"); master = get("master"); bass = get("bass"); mid = get("mid");
        treble = get("treble"); presence = get("presence"); depth = get("depth");
        gate = get("gate"); gateDecay = get("gateDecay"); gateEnabled = get("gateEnabled"); gatePost = get("gatePost");
        octaveMix = get("octaveMix"); octaveTone = get("octaveTone"); octaveLevel = get("octaveLevel");
        octaveEnabled = get("octaveEnabled"); octaveDown = get("octaveDown");
        drive = get("drive"); driveEnabled = get("driveEnabled"); driveTone = get("driveTone");
        driveLevel = get("driveLevel"); driveBoost = get("driveBoost");
        delayTime = get("delayTime"); delayEnabled = get("delayEnabled"); delayFeedback = get("delayFeedback");
        delayMix = get("delayMix"); delaySync = get("delaySync"); delayDivision = get("delayDivision");
        reverbDecay = get("reverbDecay"); reverbEnabled = get("reverbEnabled"); reverbSize = get("reverbSize");
        reverbTone = get("reverbTone"); reverbMix = get("reverbMix");
        lowCut = get("lowCut"); highCut = get("highCut"); output = get("output");
        ampChannel = get("ampChannel");
        shimmerEnabled = get("shimmerEnabled"); shimmerMix = get("shimmerMix"); shimmerDecay = get("shimmerDecay");
        shimmerAmount = get("shimmerAmount"); shimmerTone = get("shimmerTone"); shimmerFifth = get("shimmerFifth");
        freezeEnabled = get("freezeEnabled"); freezeMix = get("freezeMix"); freezeFade = get("freezeFade");
        freezeTone = get("freezeTone");
    }
    std::atomic<float>* gain, *master, *bass, *mid, *treble, *presence, *depth,
        *gate, *gateDecay, *gateEnabled, *gatePost,
        *octaveMix, *octaveTone, *octaveLevel, *octaveEnabled, *octaveDown,
        *drive, *driveEnabled, *driveTone, *driveLevel, *driveBoost,
        *delayTime, *delayEnabled, *delayFeedback, *delayMix, *delaySync, *delayDivision,
        *reverbDecay, *reverbEnabled, *reverbSize, *reverbTone, *reverbMix,
        *lowCut, *highCut, *output,
        *ampChannel,
        *shimmerEnabled, *shimmerMix, *shimmerDecay, *shimmerAmount, *shimmerTone, *shimmerFifth,
        *freezeEnabled, *freezeMix, *freezeFade, *freezeTone;
};

const juce::StringArray& MageAudioProcessor::delayDivisionNames()
{
    static const juce::StringArray names { "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2" };
    return names;
}

const juce::StringArray& MageAudioProcessor::channelNames()
{
    static const juce::StringArray names { "Arcane", "Fae" };
    return names;
}

static double delayDivisionBeats(int index)
{
    static const double beats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
    return beats[juce::jlimit(0, 7, index)];
}

MageAudioProcessor::MageAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, &undoManager, "MAGE", createLayout()),
      presets(state, undoManager)
{
    p = std::make_unique<Params>(state);
}

juce::AudioProcessorValueTreeState::ParameterLayout MageAudioProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    using Attr = juce::AudioParameterFloatAttributes;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto knobText  = [](float v, int) { return juce::String(v, 1); };
    const auto dbText    = [](float v, int) { return juce::String(v, 0) + " dB"; };
    const auto msText    = [](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 2) + " s" : juce::String(juce::roundToInt(v)) + " ms"; };
    const auto hzText    = [](float v, int) { return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; };
    const auto pctText   = [](float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; };

    auto knob = [&](const char* id, const char* name, float initial)
    {
        layout.add(std::make_unique<Float>(juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), initial, Attr().withStringFromValueFunction(knobText)));
    };
    auto ranged = [&](const char* id, const char* name, float lo, float hi, float centre, float initial,
                      std::function<juce::String(float, int)> text, const char* label)
    {
        juce::NormalisableRange<float> range(lo, hi);
        if (centre > lo && centre < hi)
            range.setSkewForCentre(centre);
        layout.add(std::make_unique<Float>(juce::ParameterID { id, 1 }, name, range, initial,
            Attr().withStringFromValueFunction(std::move(text)).withLabel(label)));
    };
    auto toggle = [&](const char* id, const char* name, bool initial)
    {
        layout.add(std::make_unique<Bool>(juce::ParameterID { id, 1 }, name, initial));
    };

    // Amplifier
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "ampChannel", 1 },
        "Amp Channel", channelNames(), 0));
    knob("gain", "Gain", 5.8f);
    knob("bass", "Bass", 5.0f);
    knob("mid", "Middle", 5.0f);
    knob("treble", "Treble", 5.0f);
    knob("presence", "Presence", 5.0f);
    knob("depth", "Depth", 5.0f);
    knob("master", "Master", 5.0f);

    // Noise gate
    ranged("gate", "Gate Threshold", -90.0f, 0.0f, 0.0f, -55.0f, dbText, "dB");
    ranged("gateDecay", "Gate Release", 5.0f, 500.0f, 80.0f, 90.0f, msText, "ms");
    toggle("gateEnabled", "Gate Enabled", true);
    toggle("gatePost", "Gate Post Amp", false);

    // Octave
    ranged("octaveMix", "Octave Mix", 0.0f, 1.0f, 0.0f, 0.0f, pctText, "");
    knob("octaveTone", "Octave Tone", 5.0f);
    knob("octaveLevel", "Octave Level", 5.0f);
    toggle("octaveEnabled", "Octave Enabled", false);
    toggle("octaveDown", "Octave Down", false);

    // Drive
    knob("drive", "Drive", 0.0f);
    toggle("driveEnabled", "Drive Enabled", false);
    knob("driveTone", "Drive Tone", 5.0f);
    knob("driveLevel", "Drive Level", 5.0f);
    toggle("driveBoost", "Drive Boost", false);

    // Delay
    ranged("delayTime", "Delay Time", 40.0f, 1200.0f, 350.0f, 350.0f, msText, "ms");
    toggle("delayEnabled", "Delay Enabled", false);
    ranged("delayFeedback", "Delay Feedback", 0.0f, 0.90f, 0.0f, 0.25f, pctText, "");
    ranged("delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f, 0.0f, pctText, "");
    toggle("delaySync", "Delay Sync", false);
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "delayDivision", 1 },
        "Delay Division", delayDivisionNames(), 5));

    // Reverb
    ranged("reverbDecay", "Reverb Decay", 0.1f, 0.98f, 0.0f, 0.55f, pctText, "");
    toggle("reverbEnabled", "Reverb Enabled", false);
    ranged("reverbSize", "Reverb Size", 0.0f, 1.0f, 0.0f, 0.55f, pctText, "");
    ranged("reverbTone", "Reverb Tone", 0.0f, 1.0f, 0.0f, 0.65f, pctText, "");
    ranged("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.0f, 0.0f, pctText, "");

    // Shimmer (Fae channel, replaces the octave)
    toggle("shimmerEnabled", "Shimmer Enabled", false);
    ranged("shimmerMix", "Shimmer Mix", 0.0f, 1.0f, 0.0f, 0.4f, pctText, "");
    ranged("shimmerDecay", "Shimmer Decay", 0.0f, 1.0f, 0.0f, 0.65f, pctText, "");
    ranged("shimmerAmount", "Shimmer Amount", 0.0f, 1.0f, 0.0f, 0.55f, pctText, "");
    ranged("shimmerTone", "Shimmer Tone", 0.0f, 1.0f, 0.0f, 0.6f, pctText, "");
    toggle("shimmerFifth", "Shimmer Fifth", false);

    // Freeze (Fae channel, replaces the drive)
    toggle("freezeEnabled", "Freeze", false);
    ranged("freezeMix", "Freeze Mix", 0.0f, 1.0f, 0.0f, 0.7f, pctText, "");
    ranged("freezeFade", "Freeze Fade", 0.05f, 4.0f, 0.8f, 0.8f,
           [](float v, int) { return juce::String(v, v < 1.0f ? 2 : 1) + " s"; }, "s");
    ranged("freezeTone", "Freeze Tone", 0.0f, 1.0f, 0.0f, 0.7f, pctText, "");
    toggle("freezeMomentary", "Freeze Momentary", false);

    // Cabinet and output
    ranged("lowCut", "Low Cut", 20.0f, 500.0f, 90.0f, 70.0f, hzText, "Hz");
    ranged("highCut", "High Cut", 3000.0f, 20000.0f, 8000.0f, 12000.0f, hzText, "Hz");
    knob("output", "Output", 7.0f);
    return layout;
}

void MageAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    maxBlock = juce::jmax(1, samplesPerBlock);
    const double osRate = sampleRate * (1 << oversamplingOrder);

    mono.setSize(1, maxBlock, false, true, true);
    gateGains.setSize(1, maxBlock, false, true, true);

    oversampler.initProcessing(static_cast<size_t>(maxBlock));
    oversampler.reset();
    setLatencySamples(juce::roundToInt(oversampler.getLatencyInSamples()));

    gate.prepare(sampleRate);
    octave.prepare(sampleRate);
    drive.prepare(osRate);
    preamp.prepare(osRate);
    for (auto* b : { &bass, &middle, &treble, &presence, &depth })
        b->prepare(osRate);
    powerDC.prepare(osRate);
    cleanPreamp.prepare(osRate);
    cleanPresence.prepare(osRate);
    cleanDepth.prepare(osRate);
    cleanPowerDC.prepare(osRate);
    freeze.prepare(sampleRate);
    shimmer.prepare(sampleRate);
    shimmerWasEnabled = false;
    cabSim.prepare(sampleRate);
    for (auto* b : { &lowCut1, &lowCut2, &highCut1, &highCut2 })
        b->prepare(sampleRate);

    const juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(maxBlock), 1 };
    cabinetIR.prepare(monoSpec);
    if (cabinetIRPath.isNotEmpty())
    {
        const juce::File irFile(cabinetIRPath);
        if (irFile.existsAsFile())
            loadCabinetIR(irFile);
    }

    const juce::dsp::ProcessSpec stereoSpec { sampleRate, static_cast<juce::uint32>(maxBlock), 2 };
    delay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.6));
    delay.prepare(stereoSpec);
    delay.reset();
    for (auto& d : delayDamp) { d.setCutoff(sampleRate, 4500.0f); d.reset(); }
    reverb.prepare(stereoSpec);
    reverb.reset();

    // Smoothers
    for (auto* s : { &stage1Gain, &stage2Gain, &powerDrive, &cleanGain, &cleanPower, &cleanMakeup })
        s->reset(osRate, 0.04);
    channelMix.reset(osRate, 0.06);
    for (auto* s : { &outputGain, &octaveMixS, &delayMixS, &delayFeedbackS, &freezeMixS })
        s->reset(sampleRate, 0.03);
    shimmerMixS.reset(sampleRate, 0.08);
    delaySamplesS.reset(sampleRate, 0.25);

    stage1Gain.setCurrentAndTargetValue(1.0f);
    stage2Gain.setCurrentAndTargetValue(1.0f);
    powerDrive.setCurrentAndTargetValue(1.0f);
    cleanGain.setCurrentAndTargetValue(1.0f);
    cleanPower.setCurrentAndTargetValue(1.0f);
    cleanMakeup.setCurrentAndTargetValue(1.0f);
    channelMix.setCurrentAndTargetValue(p->ampChannel->load() > 0.5f ? 1.0f : 0.0f);
    shimmerMixS.setCurrentAndTargetValue(0.0f);
    freezeMixS.setCurrentAndTargetValue(p->freezeMix->load());
    outputGain.setCurrentAndTargetValue(dbToGain(knobDb(p->output->load(), 12.0f) - 10.0f));
    octaveMixS.setCurrentAndTargetValue(0.0f);
    delayMixS.setCurrentAndTargetValue(0.0f);
    delayFeedbackS.setCurrentAndTargetValue(p->delayFeedback->load());
    delaySamplesS.setCurrentAndTargetValue(static_cast<float>(sampleRate * p->delayTime->load() * 0.001));

    updateFilters(true);
}

void MageAudioProcessor::updateFilters(bool force)
{
    auto changed = [force](float& last, float now) { if (force || std::abs(last - now) > 1.0e-4f) { last = now; return true; } return false; };

    if (changed(lastBass, p->bass->load()))         bass.setLowShelf(110.0f, 0.7f, knobDb(lastBass));
    if (changed(lastMid, p->mid->load()))           middle.setPeak(700.0f, 0.7f, knobDb(lastMid));
    if (changed(lastTreble, p->treble->load()))     treble.setHighShelf(3200.0f, 0.7f, knobDb(lastTreble));
    if (changed(lastPresence, p->presence->load()))
    {
        presence.setHighShelf(3800.0f, 0.6f, knobDb(lastPresence, 9.0f));
        cleanPresence.setHighShelf(4500.0f, 0.6f, knobDb(lastPresence, 8.0f));
    }
    if (changed(lastDepth, p->depth->load()))
    {
        depth.setPeak(90.0f, 0.9f, knobDb(lastDepth, 9.0f));
        cleanDepth.setPeak(100.0f, 0.8f, knobDb(lastDepth, 7.0f));
    }
    {
        const bool a = changed(lastCleanBass, p->bass->load());
        const bool b = changed(lastCleanMid, p->mid->load());
        const bool c = changed(lastCleanTreble, p->treble->load());
        if (a || b || c)
            cleanPreamp.setTone(lastCleanBass, lastCleanMid, lastCleanTreble);
    }
    if (changed(lastLowCut, p->lowCut->load()))
    {
        lowCut1.setHighPass(lastLowCut, 0.54f);
        lowCut2.setHighPass(lastLowCut, 1.31f);   // 4th-order Butterworth pair
    }
    if (changed(lastHighCut, p->highCut->load()))
    {
        highCut1.setLowPass(lastHighCut, 0.54f);
        highCut2.setLowPass(lastHighCut, 1.31f);
    }
    if (changed(lastDriveTone, p->driveTone->load())) drive.setTone(lastDriveTone);
    if (changed(lastOctTone, p->octaveTone->load()))  octave.setTone(lastOctTone);
}

void MageAudioProcessor::releaseResources()
{
    delay.reset();
    reverb.reset();
    oversampler.reset();
}

bool MageAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    return outOk && inOk;
}

void MageAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numOut = getTotalNumOutputChannels();
    if (n == 0 || buffer.getNumChannels() == 0)
        return;

    if (n > maxBlock) // defensive: hosts should never exceed the prepared size
    {
        buffer.clear();
        return;
    }

    // ---- Guitar is mono: process input channel 1, fan out at the end. ----
    auto* m = mono.getWritePointer(0);
    juce::FloatVectorOperations::copy(m, buffer.getReadPointer(0), n);
    inputPeak.store(juce::jmax(inputPeak.load(), juce::FloatVectorOperations::findMaximum(m, n),
                               -juce::FloatVectorOperations::findMinimum(m, n)));

    const auto& P = *p;
    updateFilters(false);

    // The Fae channel swaps the octave and drive pedals for shimmer and freeze.
    const bool fae = P.ampChannel->load() > 0.5f;
    const bool gateOn = P.gateEnabled->load() > 0.5f;
    const bool gatePost = P.gatePost->load() > 0.5f;
    const bool octaveOn = !fae && P.octaveEnabled->load() > 0.5f;
    const bool octaveDown = P.octaveDown->load() > 0.5f;
    const bool driveOn = !fae && P.driveEnabled->load() > 0.5f;
    const bool shimmerOn = fae && P.shimmerEnabled->load() > 0.5f;
    const bool freezeOn = fae && P.freezeEnabled->load() > 0.5f;
    const bool boost = P.driveBoost->load() > 0.5f;
    const bool delayOn = P.delayEnabled->load() > 0.5f;
    const bool reverbOn = P.reverbEnabled->load() > 0.5f;

    // ---- Noise gate (detector on the dry input) ----
    auto* gg = gateGains.getWritePointer(0);
    if (gateOn)
    {
        gate.setParameters(P.gate->load(), P.gateDecay->load());
        for (int i = 0; i < n; ++i)
            gg[i] = gate.process(m[i]);
        if (!gatePost)
            juce::FloatVectorOperations::multiply(m, gg, n);
    }
    gateIsOpen.store(!gateOn || gate.isOpen());

    // ---- Octave (pre-amp) ----
    octaveMixS.setTargetValue(octaveOn ? P.octaveMix->load() : 0.0f);
    if (octaveOn || octaveMixS.isSmoothing() || octaveMixS.getCurrentValue() > 0.0f)
    {
        const float level = dbToGain(knobDb(P.octaveLevel->load(), 10.0f));
        for (int i = 0; i < n; ++i)
        {
            const float mix = octaveMixS.getNextValue();
            const float o = octave.process(m[i], octaveDown);
            m[i] = m[i] * (1.0f - mix) + o * mix * level;
        }
    }

    // ---- Oversampled section: drive pedal -> preamp -> tone stack -> power amp ----
    const float gainKnob = P.gain->load();
    stage1Gain.setTargetValue(dbToGain(4.0f + gainKnob * 2.6f));
    stage2Gain.setTargetValue(dbToGain(-4.0f + gainKnob * 2.2f));
    powerDrive.setTargetValue(dbToGain(-10.0f + P.master->load() * 1.6f));
    cleanGain.setTargetValue(dbToGain(-7.0f + gainKnob * 1.7f));
    cleanPower.setTargetValue(dbToGain(-5.0f + P.master->load() * 1.0f));
    // Level-match the clean channel: give back part of the gain/master boost so the
    // knobs mostly change character, not loudness.
    cleanMakeup.setTargetValue(dbToGain(4.5f - 0.6f * (gainKnob * 1.7f - 7.0f) - 0.5f * (P.master->load() - 5.0f)));
    channelMix.setTargetValue(fae ? 1.0f : 0.0f);
    const float driveAmt = P.drive->load() * 0.1f;
    const float driveLevel = dbToGain(knobDb(P.driveLevel->load(), 10.0f));

    float* chans[] = { m };
    juce::dsp::AudioBlock<float> monoBlock(chans, 1, static_cast<size_t>(n));
    auto up = oversampler.processSamplesUp(monoBlock);
    float* u = up.getChannelPointer(0);
    const int nu = static_cast<int>(up.getNumSamples());
    for (int i = 0; i < nu; ++i)
    {
        const float in = u[i];
        const float cm = channelMix.getNextValue();   // 0 = Arcane, 1 = Fae (crossfades on switch)
        float arcane = 0.0f, clean = 0.0f;
        if (cm < 1.0f)
        {
            float x = in;
            if (driveOn)
                x = drive.process(x, driveAmt, driveLevel, boost);
            x = preamp.process(x, stage1Gain.getNextValue(), stage2Gain.getNextValue());
            x = bass.process(x);
            x = middle.process(x);
            x = treble.process(x);
            // Power amp: master pushes the output stage into saturation; presence
            // and depth act like the feedback-loop shelving of a real power amp.
            x = mage::dsp::tubeClip(x * powerDrive.getNextValue() * 1.3f, 0.08f);
            x = powerDC.process(x);
            x = presence.process(x);
            x = depth.process(x);
            arcane = x * 0.7f;
        }
        if (cm > 0.0f)
        {
            // Fae clean: headroom-rich power stage that only blooms when the master is up.
            float x = cleanPreamp.process(in, cleanGain.getNextValue());
            x = mage::dsp::tubeClip(x * cleanPower.getNextValue(), 0.04f);
            x = cleanPowerDC.process(x);
            x = cleanPresence.process(x);
            x = cleanDepth.process(x);
            clean = x * 1.25f * cleanMakeup.getNextValue();
        }
        u[i] = arcane + (clean - arcane) * cm;
    }
    oversampler.processSamplesDown(monoBlock);

    if (gateOn && gatePost)
        juce::FloatVectorOperations::multiply(m, gg, n);

    // ---- Cabinet ----
    if (cabinetIRLoaded.load())
    {
        juce::dsp::ProcessContextReplacing<float> ctx(monoBlock);
        cabinetIR.process(ctx);
    }
    else
    {
        for (int i = 0; i < n; ++i)
            m[i] = cabSim.process(m[i]);
    }
    for (int i = 0; i < n; ++i)
        m[i] = highCut2.process(highCut1.process(lowCut2.process(lowCut1.process(m[i]))));

    // ---- Freeze (Fae): keeps recording while on the Fae channel so a stomp can capture the last moment ----
    if (fae || freeze.isActive())
    {
        freeze.setParameters(P.freezeFade->load(), P.freezeTone->load());
        freezeMixS.setTargetValue(P.freezeMix->load() * 1.4f);
        for (int i = 0; i < n; ++i)
        {
            const float frozen = freeze.process(m[i], freezeOn);
            m[i] += frozen * freezeMixS.getNextValue();
        }
    }

    // ---- Fan out to stereo ----
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        if (ch < numOut)
            buffer.copyFrom(ch, 0, mono, 0, 0, n);
        else
            buffer.clear(ch, 0, n);
    }
    const int outCh = juce::jmin(2, numOut, buffer.getNumChannels());

    // ---- Shimmer (Fae) ----
    if (shimmerOn && !shimmerWasEnabled)
        shimmer.reset();
    shimmerWasEnabled = shimmerOn;
    shimmerMixS.setTargetValue(shimmerOn ? P.shimmerMix->load() : 0.0f);
    if (outCh > 0 && (shimmerOn || shimmerMixS.isSmoothing() || shimmerMixS.getCurrentValue() > 0.0f))
    {
        shimmer.setParameters(P.shimmerDecay->load(), P.shimmerAmount->load(), P.shimmerTone->load(),
                              P.shimmerFifth->load() > 0.5f);
        float* L = buffer.getWritePointer(0);
        float* R = outCh > 1 ? buffer.getWritePointer(1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            const float in = R != nullptr ? 0.5f * (L[i] + R[i]) : L[i];
            float wl = 0.0f, wr = 0.0f;
            shimmer.process(in, wl, wr);
            const float mix = shimmerMixS.getNextValue();
            const float dry = 1.0f - 0.3f * mix;
            if (R != nullptr)
            {
                L[i] = L[i] * dry + wl * mix * 0.9f;
                R[i] = R[i] * dry + wr * mix * 0.9f;
            }
            else
            {
                L[i] = L[i] * dry + (wl + wr) * 0.5f * mix * 0.9f;
            }
        }
    }

    // ---- Delay ----
    if (delayOn && !delayWasEnabled)
        delay.reset();
    delayWasEnabled = delayOn;
    if (delayOn)
    {
        double targetSamples = sampleRate * P.delayTime->load() * 0.001;
        if (P.delaySync->load() > 0.5f)
            if (auto* head = getPlayHead())
                if (auto pos = head->getPosition())
                    if (auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 20.0)
                        targetSamples = sampleRate * 60.0 / *bpm * delayDivisionBeats(juce::roundToInt(P.delayDivision->load()));
        const float maxD = static_cast<float>(delay.getMaximumDelayInSamples() - 2);
        const float stereoOffset = static_cast<float>(sampleRate * 0.009);
        delaySamplesS.setTargetValue(juce::jlimit(1.0f, maxD - stereoOffset, static_cast<float>(targetSamples)));
        delayMixS.setTargetValue(P.delayMix->load());
        delayFeedbackS.setTargetValue(P.delayFeedback->load());

        for (int i = 0; i < n; ++i)
        {
            const float d = delaySamplesS.getNextValue();
            const float mix = delayMixS.getNextValue();
            const float fb = delayFeedbackS.getNextValue();
            for (int ch = 0; ch < outCh; ++ch)
            {
                float* s = buffer.getWritePointer(ch);
                const float wet = delayDamp[ch].process(delay.popSample(ch, d + (ch == 1 ? stereoOffset : 0.0f)));
                delay.pushSample(ch, s[i] + wet * fb);
                s[i] = s[i] * (1.0f - 0.4f * mix) + wet * mix;
            }
        }
    }
    else
    {
        delayMixS.setCurrentAndTargetValue(0.0f);
    }

    // ---- Reverb ----
    if (reverbOn && !reverbWasEnabled)
        reverb.reset();
    reverbWasEnabled = reverbOn;
    if (reverbOn && outCh > 0)
    {
        juce::dsp::Reverb::Parameters rp;
        const float mix = P.reverbMix->load();
        rp.roomSize = juce::jlimit(0.0f, 1.0f, 0.3f + 0.69f * (P.reverbSize->load() * 0.6f + P.reverbDecay->load() * 0.4f));
        rp.damping = 1.0f - P.reverbTone->load();
        rp.wetLevel = mix * 0.6f;
        rp.dryLevel = (1.0f - mix * 0.3f) * 0.5f;   // juce::Reverb doubles the dry path internally
        rp.width = 1.0f;
        reverb.setParameters(rp);
        juce::dsp::AudioBlock<float> outBlock(buffer.getArrayOfWritePointers(), static_cast<size_t>(outCh), static_cast<size_t>(n));
        juce::dsp::ProcessContextReplacing<float> ctx(outBlock);
        reverb.process(ctx);
    }

    // ---- Output level ----
    outputGain.setTargetValue(dbToGain(knobDb(P.output->load(), 12.0f) - 10.0f));
    if (outputGain.isSmoothing())
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = outputGain.getNextValue();
            for (int ch = 0; ch < outCh; ++ch)
                buffer.getWritePointer(ch)[i] *= g;
        }
    }
    else
    {
        for (int ch = 0; ch < outCh; ++ch)
            buffer.applyGain(ch, 0, n, outputGain.getTargetValue());
    }

    float peak = 0.0f;
    for (int ch = 0; ch < outCh; ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, n));
    outputPeak.store(juce::jmax(outputPeak.load(), peak));
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
    if (!tree.isValid() || !tree.hasType(state.state.getType()))
        return;

    state.replaceState(tree);
    undoManager.clearUndoHistory();
    cabinetIRPath = tree.getProperty("cabinetIRPath").toString();
    if (cabinetIRPath.isNotEmpty())
    {
        const juce::File irFile(cabinetIRPath);
        if (irFile.existsAsFile())
            loadCabinetIR(irFile);
        else
            cabinetIRLoaded.store(false);
    }
    else
    {
        cabinetIRLoaded.store(false);
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
    if (reader == nullptr || reader->numChannels < 1 || reader->lengthInSamples < 8)
        return false;

    // The cabinet runs on the mono amp signal, so only the first IR channel is used.
    // Convolution loads on a background thread and swaps in safely.
    cabinetIR.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0,
                                  juce::dsp::Convolution::Normalise::yes);
    cabinetIRPath = file.getFullPathName();
    cabinetIRName = file.getFileNameWithoutExtension();
    cabinetIRLoaded.store(true);
    return true;
}

void MageAudioProcessor::bypassCabinetIR()
{
    // Only flip the flag: the audio thread falls back to the built-in cab voicing.
    cabinetIRLoaded.store(false);
    cabinetIRPath.clear();
    cabinetIRName.clear();
}

juce::AudioProcessorEditor* MageAudioProcessor::createEditor()
{
    return new MageAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MageAudioProcessor();
}

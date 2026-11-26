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
        octaveEnabled = get("octaveEnabled");
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
        compEnabled = get("compEnabled"); compSustain = get("compSustain"); compLevel = get("compLevel");
        compParallel = get("compParallel");
        phaserEnabled = get("phaserEnabled"); phaserRate = get("phaserRate"); phaserDepth = get("phaserDepth");
        phaserFeedback = get("phaserFeedback"); phaserMix = get("phaserMix"); phaserStages = get("phaserStages");
        chorusEnabled = get("chorusEnabled"); chorusRate = get("chorusRate"); chorusDepth = get("chorusDepth");
        chorusTone = get("chorusTone"); chorusMix = get("chorusMix"); chorusVibrato = get("chorusVibrato");
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int n = 0; n < mage::numSlots; ++n)
                slot[ch][n] = get(mage::slotParamID(ch, n).toRawUTF8());
            ampPos[ch] = get(mage::ampPosParamID(ch).toRawUTF8());
        }
        enabled = { octaveEnabled, driveEnabled, shimmerEnabled, phaserEnabled, chorusEnabled,
                    delayEnabled, reverbEnabled };
    }
    std::atomic<float>* gain, *master, *bass, *mid, *treble, *presence, *depth,
        *gate, *gateDecay, *gateEnabled, *gatePost,
        *octaveMix, *octaveTone, *octaveLevel, *octaveEnabled,
        *drive, *driveEnabled, *driveTone, *driveLevel, *driveBoost,
        *delayTime, *delayEnabled, *delayFeedback, *delayMix, *delaySync, *delayDivision,
        *reverbDecay, *reverbEnabled, *reverbSize, *reverbTone, *reverbMix,
        *lowCut, *highCut, *output,
        *ampChannel,
        *shimmerEnabled, *shimmerMix, *shimmerDecay, *shimmerAmount, *shimmerTone, *shimmerFifth,
        *compEnabled, *compSustain, *compLevel, *compParallel,
        *phaserEnabled, *phaserRate, *phaserDepth, *phaserFeedback, *phaserMix, *phaserStages,
        *chorusEnabled, *chorusRate, *chorusDepth, *chorusTone, *chorusMix, *chorusVibrato;
    std::atomic<float>* slot[2][mage::numSlots] {};
    std::atomic<float>* ampPos[2] {};
    std::array<std::atomic<float>*, mage::numPedals> enabled {};   // indexed by mage::Pedal
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
    midiLearn.loadDefault();
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


    // Compressor (front of the chain)
    toggle("compEnabled", "Compressor Enabled", false);
    knob("compSustain", "Compressor Sustain", 5.0f);
    knob("compLevel", "Compressor Level", 5.0f);
    toggle("compParallel", "Compressor Parallel", false);

    const auto rateText = [](float v, int) { return juce::String(v, v < 1.0f ? 2 : 1) + " Hz"; };
    // Phaser
    toggle("phaserEnabled", "Phaser Enabled", false);
    ranged("phaserRate", "Phaser Rate", 0.05f, 8.0f, 0.8f, 0.6f, rateText, "Hz");
    ranged("phaserDepth", "Phaser Depth", 0.0f, 1.0f, 0.0f, 0.7f, pctText, "");
    ranged("phaserFeedback", "Phaser Feedback", 0.0f, 0.85f, 0.0f, 0.4f, pctText, "");
    ranged("phaserMix", "Phaser Mix", 0.0f, 1.0f, 0.0f, 0.5f, pctText, "");
    toggle("phaserStages", "Phaser 8-Stage", false);

    // Chorus / vibrato
    toggle("chorusEnabled", "Chorus Enabled", false);
    ranged("chorusRate", "Chorus Rate", 0.05f, 10.0f, 1.0f, 0.9f, rateText, "Hz");
    ranged("chorusDepth", "Chorus Depth", 0.0f, 1.0f, 0.0f, 0.5f, pctText, "");
    ranged("chorusTone", "Chorus Tone", 0.0f, 1.0f, 0.0f, 0.6f, pctText, "");
    ranged("chorusMix", "Chorus Mix", 0.0f, 1.0f, 0.0f, 0.5f, pctText, "");
    toggle("chorusVibrato", "Chorus Vibrato", false);

    // Pedalboards: four slots per channel plus the amp's place among them.
    for (int ch = 0; ch < 2; ++ch)
    {
        const juce::String chName = ch == 0 ? "Arcane" : "Fae";
        for (int s = 0; s < mage::numSlots; ++s)
            layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { mage::slotParamID(ch, s), 1 },
                chName + " Slot " + juce::String(s + 1), mage::pedalNames(), mage::defaultBoard[ch][s],
                juce::AudioParameterChoiceAttributes().withAutomatable(false)));
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { mage::ampPosParamID(ch), 1 },
            chName + " Amp Position", 0, mage::numSlots, mage::defaultAmpPos[ch],
            juce::AudioParameterIntAttributes().withAutomatable(false)));
    }

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
    shimmer.prepare(sampleRate);
    phaser.prepare(sampleRate);
    chorus.prepare(sampleRate);
    compressor.prepare(sampleRate);
    driveOversampler.initProcessing(static_cast<size_t>(maxBlock));
    driveOversampler.reset();
    scratch.setSize(3, maxBlock, false, true, true);
    for (auto& s : pedalOn) { s.reset(sampleRate, 0.03); s.setCurrentAndTargetValue(0.0f); }
    placement.fill(0);
    pedalWasOn.fill(false);
    compOnS.reset(sampleRate, 0.03);
    compOnS.setCurrentAndTargetValue(p->compEnabled->load() > 0.5f ? 1.0f : 0.0f);
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
    for (auto* s : { &stage1Gain, &stage2Gain, &stage3Gain, &powerDrive, &cleanGain, &cleanPower, &cleanMakeup })
        s->reset(osRate, 0.04);
    channelMix.reset(osRate, 0.06);
    for (auto* s : { &outputGain, &octaveMixS, &delayMixS, &delayFeedbackS })
        s->reset(sampleRate, 0.03);
    shimmerMixS.reset(sampleRate, 0.08);
    delaySamplesS.reset(sampleRate, 0.25);

    stage1Gain.setCurrentAndTargetValue(1.0f);
    stage2Gain.setCurrentAndTargetValue(1.0f);
    stage3Gain.setCurrentAndTargetValue(1.0f);
    powerDrive.setCurrentAndTargetValue(1.0f);
    cleanGain.setCurrentAndTargetValue(1.0f);
    cleanPower.setCurrentAndTargetValue(1.0f);
    cleanMakeup.setCurrentAndTargetValue(1.0f);
    channelMix.setCurrentAndTargetValue(p->ampChannel->load() > 0.5f ? 1.0f : 0.0f);
    shimmerMixS.setCurrentAndTargetValue(0.0f);
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

void MageAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    // MIDI-learned controls update their parameters first so they take effect this block.
    midiLearn.process(midi);
    midi.clear();
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

    const bool fae = P.ampChannel->load() > 0.5f;
    const int board = fae ? 1 : 0;
    const bool gateOn = P.gateEnabled->load() > 0.5f;
    const bool gatePost = P.gatePost->load() > 0.5f;

    // ---- Pedalboard: which pedals run before and after the amp, in board order ----
    int order[2][mage::numSlots] {};
    int count[2] { 0, 0 };
    {
        const int ampPos = juce::jlimit(0, mage::numSlots, juce::roundToInt(P.ampPos[board]->load()));
        std::array<int, mage::numPedals> now {};
        for (int s = 0; s < mage::numSlots; ++s)
        {
            const int type = juce::jlimit(0, mage::numPedals - 1, juce::roundToInt(P.slot[board][s]->load()));
            if (now[(size_t)type] != 0)
                continue;   // duplicate (only possible via host automation): first one wins
            const int section = s < ampPos ? 1 : 2;
            now[(size_t)type] = section;
            order[section - 1][count[section - 1]++] = type;
        }
        for (int t = 0; t < mage::numPedals; ++t)
        {
            if (now[(size_t)t] != placement[(size_t)t])
            {
                // Added, removed, or moved across the amp: start from a clean state.
                resetPedalState(t);
                pedalOn[(size_t)t].setCurrentAndTargetValue(0.0f);
                pedalWasOn[(size_t)t] = false;
            }
            placement[(size_t)t] = now[(size_t)t];
        }
    }

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

    // ---- Compressor (front of the chain, after the gate) ----
    const bool compOn = P.compEnabled->load() > 0.5f;
    compOnS.setTargetValue(compOn ? 1.0f : 0.0f);
    if (compOn || compOnS.isSmoothing())
    {
        compressor.setParameters(P.compSustain->load(), P.compLevel->load(), P.compParallel->load() > 0.5f);
        for (int i = 0; i < n; ++i)
        {
            const float y = compressor.process(m[i]);
            m[i] += (y - m[i]) * compOnS.getNextValue();
        }
    }

    // ---- Pedals before the amp (mono) ----
    for (int k = 0; k < count[0]; ++k)
        runPedal(order[0][k], m, nullptr, n);

    // ---- Oversampled amp: preamp -> tone stack -> power amp ----
    const float gainKnob = P.gain->load();
    // Up to 5 the gain knob feels as it always has; above that the extra gain comes in
    // fast for modern high-gain tones, with a third stage joining in.
    const float hot = juce::jmax(0.0f, gainKnob - 5.0f);
    stage1Gain.setTargetValue(dbToGain(4.0f + gainKnob * 2.6f + hot * 1.2f));
    stage2Gain.setTargetValue(dbToGain(-4.0f + gainKnob * 2.2f + hot * 1.0f));
    stage3Gain.setTargetValue(dbToGain(-14.0f + gainKnob * 1.8f));
    powerDrive.setTargetValue(dbToGain(-10.0f + P.master->load() * 1.6f));
    cleanGain.setTargetValue(dbToGain(-7.0f + gainKnob * 1.7f));
    cleanPower.setTargetValue(dbToGain(-5.0f + P.master->load() * 1.0f));
    // Level-match the clean channel: give back part of the gain/master boost so the
    // knobs mostly change character, not loudness.
    cleanMakeup.setTargetValue(dbToGain(4.5f - 0.6f * (gainKnob * 1.7f - 7.0f) - 0.5f * (P.master->load() - 5.0f)));
    channelMix.setTargetValue(fae ? 1.0f : 0.0f);

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
            float x = preamp.process(in, stage1Gain.getNextValue(), stage2Gain.getNextValue(), stage3Gain.getNextValue());
            x = bass.process(x);
            x = middle.process(x);
            x = treble.process(x);
            // Power amp: master pushes the output stage into saturation; presence
            // and depth act like the feedback-loop shelving of a real power amp.
            x = mage::dsp::tubeClip(x * powerDrive.getNextValue() * 1.3f, 0.08f);
            x = powerDC.process(x);
            x = presence.process(x);
            x = depth.process(x);
            arcane = x * 0.82f;   // level-matched to the looser voicing it replaced
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

    // ---- Fan out to stereo ----
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        if (ch < numOut)
            buffer.copyFrom(ch, 0, mono, 0, 0, n);
        else
            buffer.clear(ch, 0, n);
    }
    const int outCh = juce::jmin(2, numOut, buffer.getNumChannels());

    // ---- Pedals after the amp (stereo) ----
    if (outCh > 0)
        for (int k = 0; k < count[1]; ++k)
            runPedal(order[1][k], buffer.getWritePointer(0), outCh > 1 ? buffer.getWritePointer(1) : nullptr, n);

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

// ---------------------------------------------------------------------------
// Pedalboard
void MageAudioProcessor::resetPedalState(int type)
{
    switch (static_cast<mage::Pedal>(type))
    {
        case mage::Pedal::octave:  octave.reset(); octaveMixS.setCurrentAndTargetValue(p->octaveMix->load()); break;
        case mage::Pedal::drive:   drive.reset(); driveOversampler.reset(); break;
        case mage::Pedal::shimmer: shimmer.reset(); shimmerMixS.setCurrentAndTargetValue(p->shimmerMix->load()); break;
        case mage::Pedal::phaser:  phaser.reset(); break;
        case mage::Pedal::chorus:  chorus.reset(); break;
        case mage::Pedal::delay:
            delay.reset();
            for (auto& d : delayDamp) d.reset();
            delayMixS.setCurrentAndTargetValue(p->delayMix->load());
            delayFeedbackS.setCurrentAndTargetValue(p->delayFeedback->load());
            break;
        case mage::Pedal::reverb:  reverb.reset(); break;
    }
}

void MageAudioProcessor::runPedal(int type, float* L, float* R, int n)
{
    const auto pedal = static_cast<mage::Pedal>(type);
    const bool on = p->enabled[(size_t)type]->load() > 0.5f;
    auto& fade = pedalOn[(size_t)type];
    fade.setTargetValue(on ? 1.0f : 0.0f);
    if (!on && !fade.isSmoothing())
    {
        pedalWasOn[(size_t)type] = false;
        return;
    }
    if (on && !pedalWasOn[(size_t)type])
        resetPedalState(type);   // no stale tails from the last time it was on
    pedalWasOn[(size_t)type] = on;

    const bool crossfade = fade.isSmoothing() || fade.getCurrentValue() < 1.0f;
    float* dryL = scratch.getWritePointer(0);
    float* dryR = scratch.getWritePointer(1);
    if (crossfade)
    {
        juce::FloatVectorOperations::copy(dryL, L, n);
        if (R != nullptr)
            juce::FloatVectorOperations::copy(dryR, R, n);
    }

    processPedal(pedal, L, R, n);

    if (crossfade)
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = fade.getNextValue();
            L[i] = dryL[i] + (L[i] - dryL[i]) * g;
            if (R != nullptr)
                R[i] = dryR[i] + (R[i] - dryR[i]) * g;
        }
    }
}

void MageAudioProcessor::processPedal(mage::Pedal pedal, float* L, float* R, int n)
{
    const auto& P = *p;
    switch (pedal)
    {
        case mage::Pedal::octave:
        {
            octaveMixS.setTargetValue(P.octaveMix->load());
            const float level = dbToGain(knobDb(P.octaveLevel->load(), 10.0f));
            for (int i = 0; i < n; ++i)
            {
                const float mix = octaveMixS.getNextValue();
                const float in = R != nullptr ? 0.5f * (L[i] + R[i]) : L[i];
                const float o = octave.process(in) * mix * level;
                L[i] = L[i] * (1.0f - mix) + o;
                if (R != nullptr)
                    R[i] = R[i] * (1.0f - mix) + o;
            }
            break;
        }
        case mage::Pedal::drive:
        {
            const float amount = P.drive->load() * 0.1f;
            const float level = dbToGain(knobDb(P.driveLevel->load(), 10.0f));
            const bool boost = P.driveBoost->load() > 0.5f;
            float* x = L;
            if (R != nullptr)
            {
                x = scratch.getWritePointer(2);
                for (int i = 0; i < n; ++i)
                    x[i] = 0.5f * (L[i] + R[i]);
            }
            float* chans[] = { x };
            juce::dsp::AudioBlock<float> block(chans, 1, static_cast<size_t>(n));
            auto up = driveOversampler.processSamplesUp(block);
            float* u = up.getChannelPointer(0);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                u[i] = drive.process(u[i], amount, level, boost);
            driveOversampler.processSamplesDown(block);
            if (R != nullptr)
            {
                juce::FloatVectorOperations::copy(L, x, n);
                juce::FloatVectorOperations::copy(R, x, n);
            }
            break;
        }
        case mage::Pedal::shimmer:
        {
            shimmer.setParameters(P.shimmerDecay->load(), P.shimmerAmount->load(), P.shimmerTone->load(),
                                  P.shimmerFifth->load() > 0.5f);
            shimmerMixS.setTargetValue(P.shimmerMix->load());
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
            break;
        }
        case mage::Pedal::phaser:
            phaser.setParameters(P.phaserRate->load(), P.phaserDepth->load(), P.phaserFeedback->load(),
                                 P.phaserMix->load(), P.phaserStages->load() > 0.5f);
            for (int i = 0; i < n; ++i)
                phaser.process(L[i], R != nullptr ? R + i : nullptr);
            break;
        case mage::Pedal::chorus:
            chorus.setParameters(P.chorusRate->load(), P.chorusDepth->load(), P.chorusTone->load(),
                                 P.chorusMix->load(), P.chorusVibrato->load() > 0.5f);
            for (int i = 0; i < n; ++i)
                chorus.process(L[i], R != nullptr ? R + i : nullptr);
            break;
        case mage::Pedal::delay:
        {
            double targetSamples = sampleRate * P.delayTime->load() * 0.001;
            if (P.delaySync->load() > 0.5f)
                if (auto* head = getPlayHead())
                    if (auto pos = head->getPosition())
                        if (auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 20.0)
                            targetSamples = sampleRate * 60.0 / *bpm * delayDivisionBeats(juce::roundToInt(P.delayDivision->load()));
            const float maxD = static_cast<float>(delay.getMaximumDelayInSamples() - 2);
            const float stereoOffset = R != nullptr ? static_cast<float>(sampleRate * 0.009) : 0.0f;
            delaySamplesS.setTargetValue(juce::jlimit(1.0f, maxD - stereoOffset, static_cast<float>(targetSamples)));
            delayMixS.setTargetValue(P.delayMix->load());
            delayFeedbackS.setTargetValue(P.delayFeedback->load());
            float* chs[] = { L, R };
            const int numCh = R != nullptr ? 2 : 1;
            for (int i = 0; i < n; ++i)
            {
                const float d = delaySamplesS.getNextValue();
                const float mix = delayMixS.getNextValue();
                const float fb = delayFeedbackS.getNextValue();
                for (int ch = 0; ch < numCh; ++ch)
                {
                    float* s = chs[ch];
                    const float wet = delayDamp[ch].process(delay.popSample(ch, d + (ch == 1 ? stereoOffset : 0.0f)));
                    delay.pushSample(ch, s[i] + wet * fb);
                    s[i] = s[i] * (1.0f - 0.4f * mix) + wet * mix;
                }
            }
            break;
        }
        case mage::Pedal::reverb:
        {
            juce::dsp::Reverb::Parameters rp;
            const float mix = P.reverbMix->load();
            rp.roomSize = juce::jlimit(0.0f, 1.0f, 0.3f + 0.69f * (P.reverbSize->load() * 0.6f + P.reverbDecay->load() * 0.4f));
            rp.damping = 1.0f - P.reverbTone->load();
            rp.wetLevel = mix * 0.6f;
            rp.dryLevel = (1.0f - mix * 0.3f) * 0.5f;   // juce::Reverb doubles the dry path internally
            rp.width = 1.0f;
            reverb.setParameters(rp);
            float* chs[] = { L, R };
            juce::dsp::AudioBlock<float> block(chs, R != nullptr ? 2u : 1u, static_cast<size_t>(n));
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            reverb.process(ctx);
            break;
        }
    }
}

void MageAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    tree.setProperty("cabinetIRPath", cabinetIRPath, nullptr);
    tree.removeChild(tree.getChildWithName(mage::MidiLearn::treeType), nullptr);
    tree.appendChild(midiLearn.toValueTree(), nullptr);
    juce::MemoryOutputStream stream(destination, false);
    tree.writeToStream(stream);
}

void MageAudioProcessor::setStateInformation(const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(size));
    if (!tree.isValid() || !tree.hasType(state.state.getType()))
        return;

    // The MIDI map travels with the session but lives outside the parameter tree.
    // Older sessions without one keep the user's default map.
    if (auto map = tree.getChildWithName(mage::MidiLearn::treeType); map.isValid())
    {
        midiLearn.fromValueTree(map);
        tree.removeChild(map, nullptr);
    }
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

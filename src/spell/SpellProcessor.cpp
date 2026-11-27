#include "SpellProcessor.h"
#include "SpellEditor.h"

using namespace mage::spell;

namespace
{
    const float sliceBeats[] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f };
    const int defaultPattern[16] = { 1, 0, 0, 1, 0, 2, 0, 0, 1, 1, 0, 3, 0, 0, 2, 0 };

    juce::String percent(float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }
    juce::String semis(float v, int)   { const int s = juce::roundToInt(v); return (s > 0 ? "+" : "") + juce::String(s); }
    juce::String ms(float v, int)      { return juce::String(juce::roundToInt(v)) + " ms"; }
    juce::String cents(float v, int)   { return juce::String(juce::roundToInt(v)) + " ct"; }
    juce::String dbText(float v, int)  { return (v > 0.05f ? "+" : "") + juce::String(v, 1) + " dB"; }

    std::vector<mage::PresetManager::Factory> spellPresets()
    {
        // Glitch patterns: 0 off, 1 stutter, 2 reverse, 3 pitch jump.
        auto pattern = [](std::initializer_list<int> steps)
        {
            std::vector<std::pair<juce::String, float>> v;
            int i = 0;
            for (int s : steps)
                v.push_back({ SpellAudioProcessor::stepID(i++), (float)s });
            return v;
        };
        auto with = [](std::vector<std::pair<juce::String, float>> a, std::vector<std::pair<juce::String, float>> b)
        {
            a.insert(a.end(), b.begin(), b.end());
            return a;
        };
        return {
            { "Shattered Prism", {} },
            { "Tape Ghost", with(pattern({ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 }),
                { { "glitchMode", 1 }, { "glitchMix", 1.0f }, { "pitchOn", 0 }, { "swellOn", 1 }, { "swellMode", 1 },
                  { "swellBloom", 0.55f }, { "swellSize", 0.8f }, { "swellMix", 0.4f } }) },
            { "Octave Whammy", { { "glitchOn", 0 }, { "swellOn", 0 }, { "pitchMode", 1 }, { "pitchShift", 12 },
                                 { "pitchWhammy", 1.0f }, { "pitchMix", 1.0f }, { "pitchGlide", 60 }, { "pitchDetune", 0 } } },
            { "Ambient Swells", { { "glitchOn", 0 }, { "pitchOn", 1 }, { "pitchMode", 2 }, { "pitchDetune", 12 },
                                  { "pitchMix", 0.5f }, { "swellOn", 1 }, { "swellMode", 0 }, { "swellAttack", 700 },
                                  { "swellBloom", 0.85f }, { "swellSize", 0.9f }, { "swellMix", 1.0f } } },
            { "Scatter Storm", { { "glitchMode", 2 }, { "glitchDensity", 0.8f }, { "glitchJump", 7 }, { "glitchSlice", 0 },
                                 { "glitchMix", 0.9f }, { "pitchOn", 0 }, { "swellOn", 0 } } },
            { "Fifth Choir", { { "glitchOn", 0 }, { "pitchShift", 7 }, { "pitchDetune", 14 }, { "pitchMix", 0.6f },
                               { "swellOn", 1 }, { "swellAttack", 600 }, { "swellBloom", 0.7f }, { "swellMix", 0.8f } } },
        };
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SpellAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto toggle = [&](const char* id, const char* name, bool def)
    { layout.add(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def)); };
    auto choice = [&](const char* id, const char* name, StringArray options, int def, bool automatable = true)
    {
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, options, def,
                                                          AudioParameterChoiceAttributes().withAutomatable(automatable)));
    };
    auto ranged = [&](const char* id, const char* name, NormalisableRange<float> range, float def,
                      String (*text)(float, int))
    {
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, range, def,
                                                         AudioParameterFloatAttributes().withStringFromValueFunction(text)));
    };
    auto skewed = [](float lo, float hi, float centre) { NormalisableRange<float> r(lo, hi); r.setSkewForCentre(centre); return r; };

    toggle("glitchOn", "Glitch On", true);
    choice("glitchMode", "Glitch Mode", { "Slice", "Tape Stop", "Scatter" }, 0);
    choice("glitchSlice", "Glitch Slice", { "1/32", "1/16", "1/8", "1/4", "1/2" }, 1);
    ranged("glitchDensity", "Glitch Density", { 0.0f, 1.0f }, 0.62f, percent);
    ranged("glitchJump", "Glitch Jump", { -12.0f, 12.0f, 1.0f }, 12.0f, semis);
    ranged("glitchMix", "Glitch Mix", { 0.0f, 1.0f }, 0.5f, percent);
    for (int i = 0; i < 16; ++i)
        choice(stepID(i).toRawUTF8(), ("Glitch Step " + String(i + 1)).toRawUTF8(),
               { "Off", "Stutter", "Reverse", "Jump" }, defaultPattern[i], false);

    toggle("pitchOn", "Pitch On", true);
    choice("pitchMode", "Pitch Mode", { "Harmony", "Whammy", "Double" }, 0);
    ranged("pitchShift", "Pitch Shift", { -12.0f, 12.0f, 1.0f }, 12.0f, semis);
    ranged("pitchWhammy", "Pitch Bend", { 0.0f, 1.0f }, 1.0f, percent);
    ranged("pitchGlide", "Pitch Glide", skewed(5.0f, 1000.0f, 120.0f), 80.0f, ms);
    ranged("pitchDetune", "Pitch Detune", { 0.0f, 50.0f }, 9.0f, cents);
    ranged("pitchMix", "Pitch Mix", { 0.0f, 1.0f }, 0.45f, percent);

    toggle("swellOn", "Swell On", false);
    choice("swellMode", "Swell Mode", { "Swell", "Bloom" }, 0);
    ranged("swellAttack", "Swell Attack", skewed(20.0f, 2000.0f, 400.0f), 420.0f, ms);
    ranged("swellBloom", "Swell Bloom", { 0.0f, 1.0f }, 0.66f, percent);
    ranged("swellSize", "Swell Size", { 0.0f, 1.0f }, 0.7f, percent);
    ranged("swellMix", "Swell Mix", { 0.0f, 1.0f }, 0.55f, percent);

    ranged("morphX", "Morph X", { 0.0f, 1.0f }, 0.5f, percent);
    ranged("morphY", "Morph Y", { 0.0f, 1.0f }, 0.62f, percent);
    toggle("cast", "Cast", false);
    choice("castMode", "Cast Mode", { "Hold", "Latch", "Burst" }, 0);
    choice("order", "Spell Order", { "Glitch > Pitch > Swell", "Pitch > Swell > Glitch", "Swell > Glitch > Pitch",
                                     "Glitch > Swell > Pitch", "Pitch > Glitch > Swell", "Swell > Pitch > Glitch" }, 0);
    ranged("mix", "Mix", { 0.0f, 1.0f }, 1.0f, percent);
    ranged("output", "Output", { -24.0f, 12.0f }, 0.0f, dbText);
    return layout;
}

SpellAudioProcessor::SpellAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, &undoManager, "MAGESpell", createLayout()),
      presets(state, undoManager, spellPresets(), "Spell Presets"),
      midiLearn(*this, "SpellMidiMap.xml")
{
    midiLearn.loadDefault();
}

bool SpellAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;
    return in.size() <= out.size();
}

void SpellAudioProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    glitch.prepare(sampleRate);
    pitch.prepare(sampleRate);
    swell.prepare(sampleRate);
    const char* onIDs[3] = { "glitchOn", "pitchOn", "swellOn" };
    for (int i = 0; i < 3; ++i)
    {
        enabled[(size_t)i].reset(sampleRate, 0.03);
        enabled[(size_t)i].setCurrentAndTargetValue(value(onIDs[i]) > 0.5f ? 1.0f : 0.0f);
    }
    mixS.reset(sampleRate, 0.03);
    mixS.setCurrentAndTargetValue(value("mix"));
    outS.reset(sampleRate, 0.03);
    outS.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("output")));
    dry.setSize(2, juce::jmax(1, samplesPerBlock), false, true, true);
    scratch.setSize(2, juce::jmax(1, samplesPerBlock), false, true, true);
    burstLeft = 0.0;
}

void SpellAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midiLearn.process(midi);
    midi.clear();

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    if (numIn == 1 && numOut > 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, n);
    for (int ch = juce::jmax(numIn, numOut > 1 && numIn == 1 ? 2 : numIn); ch < numOut; ++ch)
        buffer.clear(ch, 0, n);
    const bool stereo = numOut > 1;
    if (dry.getNumSamples() < n)
    {
        dry.setSize(2, n, false, false, true);
        scratch.setSize(2, n, false, false, true);
    }

    // ---- Transport ----
    double bpm = 120.0, ppq = -1.0;
    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    hostBpm.store((float)bpm);
    glitch.setTransport(bpm, ppq);

    // ---- CAST ----
    const int castMode = (int)value("castMode");
    const bool castParam = value("cast") > 0.5f;
    bool castActive = castParam;
    if (castMode == 2)   // Burst: one bar of glitch from each press
    {
        if (castParam && !lastCast)
            burstLeft = 16.0 * sampleRate * 60.0 / bpm / 4.0;
        castActive = burstLeft > 0.0;
        burstLeft = juce::jmax(0.0, burstLeft - n);
    }
    lastCast = castParam;
    casting.store(castActive);

    // ---- Parameters ----
    const auto w = morphWeights(value("morphX"), value("morphY"));
    Glitch::Params gp;
    for (int i = 0; i < 16; ++i)
        gp.pattern[(size_t)i] = (int)value(stepID(i).toRawUTF8());
    gp.mode = (int)value("glitchMode");
    gp.sliceBeats = sliceBeats[juce::jlimit(0, 4, (int)value("glitchSlice"))];
    gp.density = value("glitchDensity");
    gp.jumpSemis = value("glitchJump");
    gp.mix = castActive ? juce::jmax(0.85f, value("glitchMix")) : value("glitchMix") * w[0];
    gp.cast = castActive;

    Pitch::Params pp;
    pp.mode = (int)value("pitchMode");
    pp.shiftSemis = value("pitchShift");
    pp.whammy = value("pitchWhammy");
    pp.glideMs = value("pitchGlide");
    pp.detuneCents = value("pitchDetune");
    pp.mix = value("pitchMix") * w[1];

    Swell::Params sp;
    sp.mode = (int)value("swellMode");
    sp.attackMs = value("swellAttack");
    sp.bloom = value("swellBloom");
    sp.size = value("swellSize");
    sp.mix = value("swellMix") * w[2];

    // While casting, Glitch runs even if its switch is off.
    const bool on[3] = { value("glitchOn") > 0.5f || castActive, value("pitchOn") > 0.5f, value("swellOn") > 0.5f };

    // ---- Chain ----
    for (int ch = 0; ch < (stereo ? 2 : 1); ++ch)
        dry.copyFrom(ch, 0, buffer, ch, 0, n);

    const int order = juce::jlimit(0, 5, (int)value("order"));
    for (int k = 0; k < 3; ++k)
    {
        const int id = orders[order][k];
        auto& en = enabled[(size_t)id];
        const bool wasSilent = en.getCurrentValue() <= 0.0f && !en.isSmoothing();
        en.setTargetValue(on[id] ? 1.0f : 0.0f);
        if (wasSilent && !on[id])
            continue;
        if (wasSilent)
        {
            if (id == 0) glitch.reset();
            if (id == 1) pitch.reset();
            if (id == 2) swell.reset();
        }

        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch)
            scratch.copyFrom(ch, 0, buffer, ch, 0, n);
        float* l = scratch.getWritePointer(0);
        float* r = stereo ? scratch.getWritePointer(1) : nullptr;
        if (id == 0) glitch.process(l, r, n, gp);
        if (id == 1) pitch.process(l, r, n, pp);
        if (id == 2) swell.process(l, r, n, sp);

        float* bl = buffer.getWritePointer(0);
        float* br = stereo ? buffer.getWritePointer(1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            const float e = en.getNextValue();
            bl[i] += (l[i] - bl[i]) * e;
            if (br != nullptr) br[i] += (r[i] - br[i]) * e;
        }
    }
    glitchStep.store(enabled[0].getCurrentValue() > 0.0f ? glitch.currentStep() : -1);

    // ---- Mix and output ----
    mixS.setTargetValue(value("mix"));
    outS.setTargetValue(juce::Decibels::decibelsToGain(value("output")));
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float m = mixS.getNextValue(), o = outS.getNextValue();
        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch)
        {
            float* d = buffer.getWritePointer(ch);
            const float x = dry.getSample(ch, i);
            d[i] = (x + (d[i] - x) * m) * o;
            peak = juce::jmax(peak, std::abs(d[i]));
        }
    }
    outputPeak.store(juce::jmax(peak, outputPeak.load() * 0.9f));
}

void SpellAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto tree = state.copyState();
    tree.removeChild(tree.getChildWithName(mage::MidiLearn::treeType), nullptr);
    tree.appendChild(midiLearn.toValueTree(), nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary(*xml, data);
}

void SpellAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (!tree.hasType(state.state.getType()))
            return;
        const auto map = tree.getChildWithName(mage::MidiLearn::treeType);
        if (map.isValid())
        {
            midiLearn.fromValueTree(map);
            tree.removeChild(map, nullptr);
        }
        state.replaceState(tree);
    }
}

juce::AudioProcessorEditor* SpellAudioProcessor::createEditor() { return new SpellEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpellAudioProcessor(); }

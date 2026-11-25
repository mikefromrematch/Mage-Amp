#pragma once
#include <JuceHeader.h>
#include <cmath>

// Small, real-time-safe building blocks for the MAGE signal chain.
// Everything here processes one mono sample at a time and never allocates
// after prepare() has been called.
namespace mage::dsp
{
using Coeffs = juce::dsp::IIR::Coefficients<float>;
using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;

inline float dbToGain(float db) noexcept { return std::pow(10.0f, db * 0.05f); }

// Biquad whose coefficients can be replaced from the audio thread without
// heap allocation (same-size assignment into the existing coefficient array).
class Biquad
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        filter.coefficients = new Coeffs(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        filter.reset();
    }
    void reset() { filter.reset(); }

    template <size_t N>
    void set(const std::array<float, N>& c) { *filter.coefficients = c; }

    void setLowPass(float hz, float q = 0.7071f)  { set(ArrayCoeffs::makeLowPass(sampleRate, clampHz(hz), q)); }
    void setHighPass(float hz, float q = 0.7071f) { set(ArrayCoeffs::makeHighPass(sampleRate, clampHz(hz), q)); }
    void setPeak(float hz, float q, float db)     { set(ArrayCoeffs::makePeakFilter(sampleRate, clampHz(hz), q, dbToGain(db))); }
    void setLowShelf(float hz, float q, float db) { set(ArrayCoeffs::makeLowShelf(sampleRate, clampHz(hz), q, dbToGain(db))); }
    void setHighShelf(float hz, float q, float db){ set(ArrayCoeffs::makeHighShelf(sampleRate, clampHz(hz), q, dbToGain(db))); }

    float process(float x) noexcept { return filter.processSample(x); }

private:
    float clampHz(float hz) const noexcept
    {
        return juce::jlimit(10.0f, static_cast<float>(sampleRate * 0.45), hz);
    }
    double sampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> filter;
};

// One-pole low-pass, used for envelope smoothing and gentle tone controls.
class OnePole
{
public:
    void setCutoff(double sampleRate, float hz) noexcept
    {
        a = std::exp(-juce::MathConstants<float>::twoPi * hz / static_cast<float>(sampleRate));
    }
    float process(float x) noexcept { z = x + a * (z - x); return z; }
    void reset() noexcept { z = 0.0f; }
private:
    float a = 0.0f, z = 0.0f;
};

// DC blocker (leaky differentiator).
class DCBlocker
{
public:
    void prepare(double sampleRate) noexcept { r = 1.0f - (juce::MathConstants<float>::twoPi * 12.0f / (float)sampleRate); reset(); }
    void reset() noexcept { x1 = y1 = 0.0f; }
    float process(float x) noexcept { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

// Asymmetric tube-style soft clipper. Bias shifts the operating point so the
// positive and negative halves clip differently, adding even harmonics.
inline float tubeClip(float x, float bias) noexcept
{
    return std::tanh(x + bias) - std::tanh(bias);
}

// ---------------------------------------------------------------------------
// Noise gate with hysteresis and hold. The detector always listens to the
// raw guitar input so the gate tracks your picking, not the distorted tail;
// the resulting gain can be applied before or after the amp.
class NoiseGate
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        envAttack  = std::exp(-1.0f / (float)(sampleRate * 0.0005));
        envRelease = std::exp(-1.0f / (float)(sampleRate * 0.030));
        gainAttack = std::exp(-1.0f / (float)(sampleRate * 0.0008));
        reset();
    }
    void reset() { env = 0.0f; gain = 0.0f; holdCounter = 0; open = false; }

    void setParameters(float thresholdDb, float releaseMs) noexcept
    {
        openThreshold  = dbToGain(thresholdDb);
        closeThreshold = dbToGain(thresholdDb - 6.0f);
        gainRelease = std::exp(-1.0f / (float)(sampleRate * juce::jmax(1.0f, releaseMs) * 0.001));
        holdSamples = (int)(sampleRate * 0.025);
    }

    // Returns the gate gain (0..1) for this detector sample.
    float process(float key) noexcept
    {
        const float mag = std::abs(key);
        const float coeff = mag > env ? envAttack : envRelease;
        env = mag + coeff * (env - mag);

        if (env >= openThreshold)       { open = true; holdCounter = holdSamples; }
        else if (env < closeThreshold)  { if (holdCounter > 0) --holdCounter; else open = false; }

        const float target = open ? 1.0f : 0.0f;
        const float g = target > gain ? gainAttack : gainRelease;
        gain = target + g * (gain - target);
        return gain;
    }
    bool isOpen() const noexcept { return open; }

private:
    double sampleRate = 44100.0;
    float env = 0.0f, gain = 0.0f;
    float envAttack = 0.0f, envRelease = 0.0f, gainAttack = 0.0f, gainRelease = 0.0f;
    float openThreshold = 0.001f, closeThreshold = 0.0005f;
    int holdSamples = 0, holdCounter = 0;
    bool open = false;
};

// ---------------------------------------------------------------------------
// Octave pedal, placed before the amp where tracking is clean.
//  - Down: analog-style flip-flop divider (toggles on each rising zero
//    crossing of a filtered copy) multiplied with the signal.
//  - Up: full-wave rectifier, DC removed.
class OctavePedal
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        trackLP.setCutoff(sampleRate, 500.0f);
        trackHP.prepare(sampleRate);
        dc.prepare(sampleRate);
        reset();
    }
    void reset() { trackLP.reset(); trackHP.reset(); dc.reset(); tone.reset(); prev = 0.0f; flip = 1.0f; armed = true; }

    void setTone(float knob0to10) noexcept
    {
        tone.setCutoff(sampleRate, 350.0f * std::pow(2.0f, knob0to10 * 0.45f)); // ~350 Hz .. ~7.6 kHz
    }

    float process(float x, bool down) noexcept
    {
        float oct;
        if (down)
        {
            const float t = trackHP.process(trackLP.process(x));
            // Small hysteresis so noise near zero doesn't double-trigger.
            if (armed && prev <= 0.0f && t > 0.0f) { flip = -flip; armed = false; }
            if (t < -0.002f) armed = true;
            prev = t;
            oct = x * flip;
        }
        else
        {
            oct = dc.process(std::abs(x) * 1.6f);
        }
        return tone.process(oct);
    }

private:
    double sampleRate = 44100.0;
    OnePole trackLP, tone;
    DCBlocker trackHP, dc;
    float prev = 0.0f, flip = 1.0f;
    bool armed = true;
};

// ---------------------------------------------------------------------------
// Overdrive pedal in the spirit of classic mid-hump drives: only the
// high-passed part of the signal is clipped and summed back with the dry
// signal, which tightens the low end in front of a high-gain amp.
class DrivePedal
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        clipHP.prepare(sampleRate);
        clipHP.setHighPass(650.0f, 0.6f);
        toneLP.prepare(sampleRate);
        setTone(5.0f);
    }
    void reset() { clipHP.reset(); toneLP.reset(); }

    void setTone(float knob0to10) { toneLP.setLowPass(700.0f * std::pow(2.0f, knob0to10 * 0.32f), 0.6f); }

    float process(float x, float drive, float level, bool boost) noexcept
    {
        const float pre = boost ? 2.0f : 1.0f;
        const float hp = clipHP.process(x * pre);
        const float gainAmt = 1.0f + drive * drive * 2.2f;          // 1x .. ~221x on the HP band
        const float clipped = std::tanh(hp * gainAmt) * 0.55f;
        const float y = x * pre * 0.9f + clipped;
        return toneLP.process(y) * level;
    }

private:
    double sampleRate = 44100.0;
    Biquad clipHP, toneLP;
};

// ---------------------------------------------------------------------------
// Three-stage high-gain preamp with interstage filtering. Runs oversampled.
class Preamp
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        for (auto* b : { &inputHP, &tightHP, &smooth1, &stage2HP, &smooth2, &brightPeak })
            b->prepare(sampleRate);
        inputHP.setHighPass(35.0f);
        brightPeak.setPeak(2800.0f, 0.6f, 3.0f);   // input "bright cap" character
        tightHP.setHighPass(140.0f, 0.6f);         // keeps palm mutes tight
        smooth1.setLowPass(11000.0f, 0.6f);
        stage2HP.setHighPass(60.0f);
        smooth2.setLowPass(7000.0f, 0.6f);
        dc.prepare(sampleRate);
    }
    void reset()
    {
        for (auto* b : { &inputHP, &tightHP, &smooth1, &stage2HP, &smooth2, &brightPeak })
            b->reset();
        dc.reset();
    }

    // g1/g2 are linear stage gains (smoothed by the caller).
    float process(float x, float g1, float g2) noexcept
    {
        x = inputHP.process(x);
        x = brightPeak.process(x);
        x = tubeClip(x * g1, 0.22f);
        x = tightHP.process(x);
        x = smooth1.process(x);
        x = tubeClip(x * g2, -0.18f);
        x = stage2HP.process(x);
        x = smooth2.process(x);
        x = tubeClip(x * 1.8f, 0.10f) * 0.8f;
        return dc.process(x);
    }

private:
    double sampleRate = 44100.0;
    Biquad inputHP, tightHP, smooth1, stage2HP, smooth2, brightPeak;
    DCBlocker dc;
};

// ---------------------------------------------------------------------------
// Fae clean channel: one gentle stage with lots of headroom, a glassy bright
// lift and its own scooped, chimey tone stack. Gain adds warmth and, near the
// top, soft breakup rather than saturation. Runs oversampled.
class CleanPreamp
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        for (auto* b : { &inputHP, &bright, &smooth, &stageHP, &bass, &mid, &treble, &sparkle })
            b->prepare(sampleRate);
        inputHP.setHighPass(30.0f);
        bright.setHighShelf(3200.0f, 0.7f, 3.5f);   // glassy top
        smooth.setLowPass(13000.0f, 0.6f);
        stageHP.setHighPass(45.0f);
        sparkle.setPeak(4800.0f, 0.9f, 2.5f);
        setTone(5.0f, 5.0f, 5.0f);
        dc.prepare(sampleRate);
    }
    void reset()
    {
        for (auto* b : { &inputHP, &bright, &smooth, &stageHP, &bass, &mid, &treble, &sparkle })
            b->reset();
        dc.reset();
    }
    // 0..10 knobs. The middle sits in a gentle scoop at noon, like classic clean amps.
    void setTone(float bassKnob, float midKnob, float trebleKnob)
    {
        bass.setLowShelf(130.0f, 0.7f, (bassKnob - 5.0f) * 2.2f);
        mid.setPeak(480.0f, 0.75f, -3.0f + (midKnob - 5.0f) * 1.8f);
        treble.setHighShelf(2400.0f, 0.7f, (trebleKnob - 5.0f) * 2.2f);
    }
    float process(float x, float g1) noexcept
    {
        x = inputHP.process(x);
        x = bright.process(x);
        x = tubeClip(x * g1, 0.12f);   // nearly linear until pushed
        x = smooth.process(x);
        x = stageHP.process(x);
        x = bass.process(x);
        x = mid.process(x);
        x = treble.process(x);
        x = sparkle.process(x);
        return dc.process(x);
    }

private:
    double sampleRate = 44100.0;
    Biquad inputHP, bright, smooth, stageHP, bass, mid, treble, sparkle;
    DCBlocker dc;
};

// ---------------------------------------------------------------------------
// Power-of-two circular buffer used by the pitch shifter, shimmer and freeze.
class RingBuffer
{
public:
    void prepare(int minLength)
    {
        int size = 1;
        while (size < minLength + 4)
            size <<= 1;
        data.assign(static_cast<size_t>(size), 0.0f);
        mask = size - 1;
        writePos = 0;
    }
    void reset() { std::fill(data.begin(), data.end(), 0.0f); writePos = 0; }
    void push(float x) noexcept { data[static_cast<size_t>(writePos)] = x; writePos = (writePos + 1) & mask; }
    // Sample written `delay` samples ago (1 = most recent), linearly interpolated.
    float read(float delay) const noexcept
    {
        const float pos = static_cast<float>(writePos) - delay;
        const float fl = std::floor(pos);
        const int i0 = static_cast<int>(fl);
        const float frac = pos - fl;
        const float a = data[static_cast<size_t>(i0 & mask)];
        const float b = data[static_cast<size_t>((i0 + 1) & mask)];
        return a + (b - a) * frac;
    }
    float readInt(int delay) const noexcept { return data[static_cast<size_t>((writePos - delay) & mask)]; }
    float at(int absoluteIndex) const noexcept { return data[static_cast<size_t>(absoluteIndex & mask)]; }
    int position() const noexcept { return writePos; }
    int capacity() const noexcept { return mask + 1; }

private:
    std::vector<float> data { 0.0f };
    int mask = 0, writePos = 0;
};

// ---------------------------------------------------------------------------
// Two-tap delay-line pitch shifter: two read heads sweep across a short
// window at the pitch ratio, crossfaded with complementary sin^2 windows.
class PitchShifter
{
public:
    void prepare(double sampleRate, float windowMs)
    {
        window = static_cast<float>(sampleRate * windowMs * 0.001);
        buffer.prepare(static_cast<int>(window) + 8);
        phase = 0.0f;
    }
    void reset() { buffer.reset(); phase = 0.0f; }
    void setRatio(float ratio) noexcept { increment = (ratio - 1.0f) / window; }

    float process(float x) noexcept
    {
        buffer.push(x);
        phase += increment;
        phase -= std::floor(phase);
        float pb = phase + 0.5f;
        pb -= std::floor(pb);
        const float da = 1.0f + window * (1.0f - phase);
        const float db = 1.0f + window * (1.0f - pb);
        const float wa = std::sin(juce::MathConstants<float>::pi * phase);
        const float wb = std::sin(juce::MathConstants<float>::pi * pb);
        return buffer.read(da) * wa * wa + buffer.read(db) * wb * wb;
    }

private:
    RingBuffer buffer;
    float window = 2048.0f, phase = 0.0f, increment = 0.0f;
};

// ---------------------------------------------------------------------------
// Shimmer: an 8-line feedback-delay-network reverb with a pitch shifter in its
// feedback path, so every pass round the tank climbs an octave (or a fifth)
// and the tail blooms into a choir-like halo. Mono in, stereo out.
class ShimmerReverb
{
public:
    static constexpr int numLines = 8;

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        const float scale = static_cast<float>(sampleRate / 44100.0);
        static constexpr int base[numLines] = { 1031, 1327, 1523, 1801, 2063, 2381, 2647, 2903 };
        for (int i = 0; i < numLines; ++i)
        {
            lengths[i] = juce::jmax(16, static_cast<int>(static_cast<float>(base[i]) * scale));
            lines[i].prepare(lengths[i] + 2);
        }
        static constexpr int ap[2] = { 142, 379 };
        for (int i = 0; i < 2; ++i)
        {
            apLen[i] = juce::jmax(4, static_cast<int>(static_cast<float>(ap[i]) * scale));
            allpass[i].prepare(apLen[i] + 2);
        }
        shifter.prepare(sampleRate, 70.0f);
        shifterHP.prepare(sampleRate);
        shifterHP.setHighPass(280.0f, 0.6f);
        setParameters(0.6f, 0.5f, 0.6f, false);
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        for (auto& a : allpass) a.reset();
        for (auto& d : damp) d.reset();
        shifter.reset();
        shifterHP.reset();
        shiftFeedback = 0.0f;
    }

    // All 0..1. `fifth` shifts up a perfect fifth instead of an octave.
    void setParameters(float decay, float shimmer, float tone, bool fifth) noexcept
    {
        const float d = juce::jlimit(0.0f, 1.0f, decay);
        feedback = 0.68f + 0.28f * d;
        // Fixed octave-loop gain: keeps the shimmer cascade decaying at every setting.
        loopGain = 3.4f;
        amount = 0.6f * juce::jlimit(0.0f, 1.0f, shimmer);
        const float cutoff = 1500.0f * std::pow(2.0f, juce::jlimit(0.0f, 1.0f, tone) * 3.1f);
        for (auto& lp : damp) lp.setCutoff(sampleRate, cutoff);
        shifter.setRatio(fifth ? 1.4983f : 2.0f);
    }

    void process(float in, float& outL, float& outR) noexcept
    {
        // The pitch-shifted tail is fed back into the tank input, so each pass
        // round the outer loop climbs another octave (or fifth).
        float x = in + shiftFeedback;
        for (int i = 0; i < 2; ++i)   // input diffusion
        {
            const float delayed = allpass[i].readInt(apLen[i]);
            const float v = x + 0.6f * delayed;
            allpass[i].push(v);
            x = delayed - 0.6f * v;
        }

        float outs[numLines];
        float sum = 0.0f;
        for (int i = 0; i < numLines; ++i)
        {
            outs[i] = lines[i].readInt(lengths[i]);
            sum += outs[i];
        }
        const float house = sum * (2.0f / numLines);
        for (int i = 0; i < numLines; ++i)
        {
            float v = outs[i] - house;                    // Householder mixing
            v = std::tanh(damp[i].process(v) * feedback); // damping + safety limiter
            lines[i].push(std::tanh(v + x * 0.35f));
        }
        const float l = (outs[0] - outs[2] + outs[4] - outs[6]) * 0.5f;
        const float r = (outs[1] - outs[3] + outs[5] - outs[7]) * 0.5f;
        // The tank's output is ~-20 dB below its input, so the octave loop needs
        // make-up gain to bloom; damping and the limiter keep it from running away.
        shiftFeedback = shifter.process(shifterHP.process((l + r) * 0.5f)) * amount * loopGain;
        outL = l * 2.2f;
        outR = r * 2.2f;
    }

private:
    double sampleRate = 44100.0;
    RingBuffer lines[numLines], allpass[2];
    int lengths[numLines] {}, apLen[2] {};
    OnePole damp[numLines];
    PitchShifter shifter;
    Biquad shifterHP;
    float feedback = 0.8f, amount = 0.4f, shiftFeedback = 0.0f, loopGain = 5.0f;
};

// ---------------------------------------------------------------------------
// Freeze: records continuously; when engaged it captures the last moment of
// playing and sustains it as a seamless drone made of overlapping, randomly
// placed grains read from the captured audio. Fades in and out smoothly, and
// only starts recording again once it has fully faded away.
class FreezePedal
{
public:
    static constexpr int numGrains = 4;

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        buffer.prepare(static_cast<int>(sampleRate * 1.2));
        captureLength = static_cast<int>(sampleRate * 0.6);
        grainLength = static_cast<int>(sampleRate * 0.20);
        toneLP.prepare(sampleRate);
        dc.prepare(sampleRate);
        setParameters(0.8f, 0.7f);
        reset();
    }

    void reset()
    {
        buffer.reset();
        toneLP.reset();
        dc.reset();
        frozen = false;
        level = 0.0f;
    }

    void setParameters(float fadeSeconds, float tone) noexcept
    {
        fadeStep = 1.0f / static_cast<float>(sampleRate * juce::jmax(0.02f, fadeSeconds));
        toneLP.setLowPass(900.0f * std::pow(2.0f, juce::jlimit(0.0f, 1.0f, tone) * 3.8f), 0.6f);
    }

    bool isActive() const noexcept { return frozen; }

    // Returns the frozen layer (to be mixed in by the caller).
    float process(float x, bool engaged) noexcept
    {
        if (!frozen)
        {
            buffer.push(x);
            if (!engaged)
                return 0.0f;
            capture();
        }

        level = engaged ? juce::jmin(1.0f, level + fadeStep) : juce::jmax(0.0f, level - fadeStep);
        if (!engaged && level <= 0.0f)
        {
            frozen = false;
            return 0.0f;
        }

        float y = 0.0f;
        for (auto& g : grains)
        {
            const float t = static_cast<float>(g.age) / static_cast<float>(grainLength);
            const float w = std::sin(juce::MathConstants<float>::pi * t);
            y += buffer.at(captureStart + g.start + g.age) * w * w;
            if (++g.age >= grainLength)
                restartGrain(g);
        }
        y = dc.process(toneLP.process(y * 0.62f));
        return y * level;
    }

private:
    struct Grain { int start = 0, age = 0; };

    void capture() noexcept
    {
        frozen = true;
        level = 0.0f;
        captureStart = buffer.position() - captureLength;
        for (int i = 0; i < numGrains; ++i)
        {
            restartGrain(grains[i]);
            grains[i].age = grainLength * i / numGrains;   // staggered so the windows overlap evenly
        }
    }
    void restartGrain(Grain& g) noexcept
    {
        seed = seed * 1664525u + 1013904223u;
        const int span = captureLength - grainLength;
        g.start = static_cast<int>((seed >> 8) % static_cast<juce::uint32>(juce::jmax(1, span)));
        g.age = 0;
    }

    double sampleRate = 44100.0;
    RingBuffer buffer;
    Biquad toneLP;
    DCBlocker dc;
    Grain grains[numGrains];
    int captureLength = 1, grainLength = 1, captureStart = 0;
    juce::uint32 seed = 0x5eed1234u;
    float level = 0.0f, fadeStep = 0.001f;
    bool frozen = false;
};

// ---------------------------------------------------------------------------
// Fixed speaker-cabinet voicing used when no impulse response is loaded:
// low resonance bump, a slight low-mid scoop, upper-mid "cone" peak and a
// steep top-end roll-off.
class CabSim
{
public:
    void prepare(double sampleRate)
    {
        for (auto* b : { &resonance, &scoop, &cone, &rolloff1, &rolloff2 })
            b->prepare(sampleRate);
        resonance.setPeak(105.0f, 1.3f, 4.0f);
        scoop.setPeak(420.0f, 0.9f, -3.5f);
        cone.setPeak(2300.0f, 1.4f, 3.0f);
        rolloff1.setLowPass(5600.0f, 0.65f);
        rolloff2.setLowPass(5600.0f, 0.9f);
    }
    void reset() { for (auto* b : { &resonance, &scoop, &cone, &rolloff1, &rolloff2 }) b->reset(); }
    float process(float x) noexcept
    {
        return rolloff2.process(rolloff1.process(cone.process(scoop.process(resonance.process(x)))));
    }
private:
    Biquad resonance, scoop, cone, rolloff1, rolloff2;
};
} // namespace mage::dsp

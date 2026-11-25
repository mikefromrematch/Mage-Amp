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

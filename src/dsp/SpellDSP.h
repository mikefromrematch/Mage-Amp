#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

// The three spells of MAGE Spell (Glitch, Pitch, Swell). Each engine is stereo
// (pass right == nullptr for mono) and allocation-free once prepared, so the same
// classes drive both the Spell plugin and the Spell pedal on the MAGE board.
namespace mage::spell
{
// Small fast PRNG for glitch decisions on the audio thread.
struct Random
{
    juce::uint32 s = 0x9e3779b9u;
    float next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float)(s & 0xffffff) / 16777216.0f; }
};

// Circular buffer with fractional (linear) reads relative to an absolute position.
class Tape
{
public:
    void prepare(int size) { data.assign((size_t)size, 0.0f); mask = size; write = 0; }
    void clear() { std::fill(data.begin(), data.end(), 0.0f); }
    void push(float x) noexcept { data[(size_t)write] = x; if (++write >= mask) write = 0; }
    int position() const noexcept { return write; }
    int size() const noexcept { return mask; }
    // Read at absolute (wrapped) sample position p.
    float read(double p) const noexcept
    {
        const double n = (double)mask;
        p = std::fmod(p, n);
        if (p < 0.0) p += n;
        const int i0 = (int)p;
        const int i1 = i0 + 1 >= mask ? 0 : i0 + 1;
        const float f = (float)(p - (double)i0);
        return data[(size_t)i0] + (data[(size_t)i1] - data[(size_t)i0]) * f;
    }
private:
    std::vector<float> data;
    int mask = 1, write = 0;
};

// ---------------------------------------------------------------------------
// GLITCH: a 16-step, tempo-synced slicer. On each sixteenth the step pattern
// says whether to stutter (loop the slice just played), reverse it, or replay it
// pitch-jumped. Consecutive steps of the same kind keep one loop running, so a
// long slice can span several steps. Tape Stop mode turns every firing step into
// a slow-down; Scatter mode ignores the pattern and rolls dice per step.
class Glitch
{
public:
    enum Step { off = 0, stutter = 1, reverse = 2, jump = 3 };
    enum Mode { slice = 0, tapeStop = 1, scatter = 2 };

    struct Params
    {
        std::array<int, 16> pattern {};
        int mode = slice;
        float sliceBeats = 0.25f;   // slice length in quarter notes (1/16 = 0.25)
        float density = 1.0f;       // chance an active step fires (Scatter: chance any step fires)
        float jumpSemis = 12.0f;
        float mix = 0.5f;
        bool cast = false;          // every step fires while casting
    };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& t : tapes) t.prepare((int)(sr * 6.0));
        reset();
    }
    void reset()
    {
        for (auto& t : tapes) t.clear();
        active = off; activeGain = 0.0f; stepIndex = -1; posInStep = 0.0; eventPhase = 0.0;
        pendingBoundary = true;   // step 1 starts on the first sample
    }

    // Host transport: called at block start. ppq < 0 means free-running.
    void setTransport(double bpm, double ppq) noexcept
    {
        tempo = juce::jlimit(30.0, 300.0, bpm > 0.0 ? bpm : 120.0);
        stepLen = sr * 60.0 / tempo / 4.0;
        if (ppq >= 0.0)
        {
            const double steps = ppq * 4.0;
            const int idx = (int)std::floor(steps);
            const double frac = steps - std::floor(steps);
            if (std::abs((frac * stepLen) - posInStep) > 64.0 || ((idx % 16) + 16) % 16 != stepIndex)
            {
                // Jumped or drifted: snap to the host grid (next boundary handled in process()).
                stepIndex = ((idx % 16) + 16) % 16 - 1;
                posInStep = frac * stepLen;
                pendingBoundary = posInStep < 1.0;
            }
        }
    }

    int currentStep() const noexcept { return stepIndex; }
    bool isFiring() const noexcept { return active != off; }

    void process(float* left, float* right, int n, const Params& p) noexcept
    {
        const double sliceLen = juce::jlimit(64.0, (double)tapes[0].size() * 0.45, sr * 60.0 / tempo * (double)p.sliceBeats);
        const double fadeLen = sr * 0.003;
        const float rampStep = (float)(1.0 / (sr * 0.004));
        for (int i = 0; i < n; ++i)
        {
            if (posInStep >= stepLen || pendingBoundary)
            {
                if (!pendingBoundary) posInStep -= stepLen;
                pendingBoundary = false;
                stepIndex = (stepIndex + 1) % 16;
                onStep(p, sliceLen);
            }

            const float inL = left[i];
            const float inR = right != nullptr ? right[i] : inL;
            tapes[0].push(inL);
            tapes[1].push(inR);

            float wetL = inL, wetR = inR;
            const int readType = active != off ? active : (activeGain > 0.0f ? fadeType : off);
            if (readType != off)
            {
                double readPos = 0.0;
                float seam = 1.0f;
                if (p.mode == tapeStop)
                {
                    // Playback speed falls from 1 to 0 over the event.
                    const double t = juce::jlimit(0.0, 1.0, eventPhase / juce::jmax(1.0, eventLen));
                    const double rate = (1.0 - t) * (1.0 - t);
                    tapeRead += rate;
                    readPos = tapeRead;
                }
                else
                {
                    double ph = 0.0;
                    if (readType == jump)
                    {
                        ph = std::fmod(eventPhase * jumpRate, sliceLen);
                        readPos = loopStart + ph;
                    }
                    else if (readType == reverse)
                    {
                        ph = std::fmod(eventPhase, sliceLen);
                        readPos = loopStart + sliceLen - 1.0 - ph;
                    }
                    else
                    {
                        ph = std::fmod(eventPhase, sliceLen);
                        readPos = loopStart + ph;
                    }
                    seam = (float)juce::jmin(1.0, ph / fadeLen, (sliceLen - ph) / fadeLen);
                }
                wetL = tapes[0].read(readPos) * seam;
                wetR = tapes[1].read(readPos) * seam;
                eventPhase += 1.0;
            }

            const float target = active != off ? 1.0f : 0.0f;
            activeGain += juce::jlimit(-rampStep, rampStep, target - activeGain);
            if (activeGain <= 0.0f && active == off) { posInStep += 1.0; continue; }
            const float m = p.mix * activeGain;
            left[i] = inL + (wetL - inL) * m;
            if (right != nullptr) right[i] = inR + (wetR - inR) * m;
            posInStep += 1.0;
        }
    }

private:
    void onStep(const Params& p, double sliceLen) noexcept
    {
        int type = p.pattern[(size_t)stepIndex];
        if (p.mode == scatter)
            type = rng.next() < p.density * 0.7f ? 1 + (int)(rng.next() * 2.999f) : off;
        else if (p.cast)
            type = type == off ? stutter : type;
        else if (type != off && rng.next() >= p.density)
            type = off;

        if (p.mode == tapeStop && type != off)
            type = stutter;   // any firing step becomes a tape stop

        if (type == active && type != off)
            return;   // same kind on consecutive steps: keep the loop running

        if (type != off)
        {
            const double now = (double)tapes[0].position();
            loopStart = now - sliceLen;
            tapeRead = now - 1.0;
            eventPhase = 0.0;
            eventLen = stepLen * 2.0;
            jumpRate = std::pow(2.0, (double)p.jumpSemis / 12.0);
            fadeType = type;
        }
        active = type;
    }

    double sr = 44100.0, tempo = 120.0, stepLen = 5512.5, posInStep = 0.0;
    int stepIndex = -1;
    bool pendingBoundary = false;
    std::array<Tape, 2> tapes;
    int active = off, fadeType = off;   // fadeType keeps the last loop playing while it fades out
    float activeGain = 0.0f;
    double loopStart = 0.0, tapeRead = 0.0, eventPhase = 0.0, eventLen = 1.0, jumpRate = 2.0;
    Random rng;
};

// ---------------------------------------------------------------------------
// One granular pitch-shift voice: two read taps sweep through a short delay line
// with crossfaded Hann windows (the classic delay-line shifter). Low latency and
// guitar friendly; chords smear a little at big intervals, which suits the effect.
class ShiftVoice
{
public:
    void prepare(double sampleRate, double windowMs = 46.0)
    {
        sr = sampleRate;
        window = sr * windowMs * 0.001;
        tape.prepare((int)(sr * 0.5));
        reset();
    }
    void reset() { tape.clear(); phase = 0.0; }
    void setExtraDelayMs(double ms) noexcept { extra = sr * ms * 0.001; }

    float process(float x, double ratio) noexcept
    {
        tape.push(x);
        phase += (1.0 - ratio) / window;
        phase -= std::floor(phase);
        const double now = (double)tape.position() - 1.0 - 2.0 - extra;
        const double p2 = phase + 0.5 - std::floor(phase + 0.5);
        const float w1 = (float)std::sin(juce::MathConstants<double>::pi * phase);
        const float w2 = (float)std::sin(juce::MathConstants<double>::pi * p2);
        return tape.read(now - phase * window) * w1 * w1 + tape.read(now - p2 * window) * w2 * w2;
    }

private:
    double sr = 44100.0, window = 2000.0, phase = 0.0, extra = 0.0;
    Tape tape;
};

// PITCH: Harmony (dry + shifted voice, optional detuned twin), Whammy (an expression
// sweep from unison up/down to the interval, fully wet), Double (two slightly
// detuned, slightly delayed voices for an instant double-track).
class Pitch
{
public:
    enum Mode { harmony = 0, whammy = 1, doubler = 2 };
    struct Params
    {
        int mode = harmony;
        float shiftSemis = 12.0f;
        float whammy = 1.0f;      // bend: 0 = unison, 1 = full interval
        float glideMs = 80.0f;
        float detuneCents = 9.0f;
        float mix = 0.45f;
    };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& v : voices) v.prepare(sr);
        voices[2].setExtraDelayMs(11.0);
        voices[3].setExtraDelayMs(17.0);
        reset();
    }
    void reset()
    {
        for (auto& v : voices) v.reset();
        semis = -1000.0;
    }

    void process(float* left, float* right, int n, const Params& p) noexcept
    {
        // Bend (the whammy amount) sweeps from unison to the interval in every mode.
        const double target = p.mode == doubler ? 0.0 : (double)(p.shiftSemis * p.whammy);
        if (semis < -999.0) semis = target;
        const double glide = std::exp(-1.0 / (sr * juce::jmax(1.0, (double)p.glideMs) * 0.001));
        const double cents = (double)p.detuneCents / 100.0;
        for (int i = 0; i < n; ++i)
        {
            semis = target + (semis - target) * glide;
            const float inL = left[i];
            const float inR = right != nullptr ? right[i] : inL;
            float wetL, wetR;
            if (p.mode == doubler)
            {
                const double up = std::pow(2.0, (cents + 0.02) / 12.0), down = std::pow(2.0, -(cents + 0.02) / 12.0);
                const float a = voices[2].process(inL, up), b = voices[3].process(inR, down);
                wetL = a * 0.9f + b * 0.25f;
                wetR = b * 0.9f + a * 0.25f;
                left[i] = inL + wetL * p.mix * 0.8f;
                if (right != nullptr) right[i] = inR + wetR * p.mix * 0.8f;
                continue;
            }
            const double ratio = std::pow(2.0, semis / 12.0);
            wetL = voices[0].process(inL, ratio);
            wetR = right != nullptr ? voices[1].process(inR, ratio) : wetL;
            if (p.detuneCents > 0.5f)
            {
                // A detuned twin on the shifted voice, panned to the other side.
                const double twin = std::pow(2.0, (semis - cents) / 12.0);
                const float t = voices[2].process(0.5f * (inL + inR), twin);
                wetL = wetL * 0.8f + t * 0.45f;
                wetR = wetR * 0.8f + t * 0.25f;
            }
            if (p.mode == whammy)
            {
                left[i] = inL + (wetL - inL) * p.mix;
                if (right != nullptr) right[i] = inR + (wetR - inR) * p.mix;
            }
            else
            {
                left[i] = inL + wetL * p.mix;
                if (right != nullptr) right[i] = inR + wetR * p.mix;
            }
        }
    }

private:
    double sr = 44100.0, semis = 0.0;
    std::array<ShiftVoice, 4> voices;
};

// ---------------------------------------------------------------------------
// SWELL: detects each pick attack and fades the note in (a volume-knob swell),
// feeding a reverb. Bloom mode leaves the dry note alone and swells the reverb
// up behind it instead, like a reverse reverb that follows your playing.
class Swell
{
public:
    enum Mode { swell = 0, bloom = 1 };
    struct Params
    {
        int mode = swell;
        float attackMs = 420.0f;
        float bloom = 0.66f;   // reverb amount
        float size = 0.7f;
        float mix = 0.55f;     // depth of the swell
    };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reverb.setSampleRate(sr);
        reset();
    }
    void reset()
    {
        reverb.reset();
        fast = floor = peak = hpLow = 0.0f; gain = 1.0f; refractory = 0; dipping = false;
    }
    float currentGain() const noexcept { return gain; }

    void process(float* left, float* right, int n, const Params& p) noexcept
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.55f + 0.44f * p.size;
        rp.damping = 0.35f;
        rp.width = 1.0f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        reverb.setParameters(rp);

        // Pick detector: a fast envelope of the high-passed signal (pick attacks are
        // bright) against a floor that drops instantly between notes and rises slowly,
        // gated by the recent peak so beating strings in a ringing chord don't retrigger.
        const float fastA = std::exp(-1.0f / (float)(sr * 0.0005)), fastR = std::exp(-1.0f / (float)(sr * 0.020));
        const float floorUp = std::exp(-1.0f / (float)(sr * 0.025)), peakFall = std::exp(-1.0f / (float)(sr * 0.8));
        const float hpA = std::exp(-juce::MathConstants<float>::twoPi * 600.0f / (float)sr);
        const float rise = (float)(1.0 / (sr * juce::jmax(5.0f, p.attackMs) * 0.001));
        const float fall = (float)(1.0 / (sr * 0.004));
        const int hold = (int)(sr * 0.035);

        for (int i = 0; i < n; ++i)
        {
            const float inL = left[i];
            const float inR = right != nullptr ? right[i] : inL;
            const float mono = 0.5f * (inL + inR);
            hpLow = mono + (hpLow - mono) * hpA;
            const float level = std::abs(mono - hpLow);
            fast = level > fast ? level + (fast - level) * fastA : level + (fast - level) * fastR;
            peak = fast > peak ? fast : peak * peakFall;
            if (refractory > 0)
            {
                --refractory;
                floor = juce::jmax(floor, fast);
            }
            else
            {
                floor = fast < floor ? fast : fast + (floor - fast) * floorUp;
                if (fast > 0.002f && fast > floor * 1.5f + 0.001f && fast > 0.25f * peak)
                {
                    dipping = true;   // new pick: duck quickly, then swell back up
                    refractory = hold;
                    floor = fast;
                }
            }
            if (dipping)
            {
                gain -= fall;
                if (gain <= 0.0f) { gain = 0.0f; dipping = false; }
            }
            else
            {
                gain = juce::jmin(1.0f, gain + rise);
            }
            const float curve = gain * gain * (3.0f - 2.0f * gain);   // smoothstep

            float dryG, revInL, revInR, revG;
            if (p.mode == bloom)
            {
                dryG = 1.0f - 0.35f * p.mix;
                revInL = inL; revInR = inR;
                revG = p.bloom * (0.25f + 1.6f * p.mix * curve);
            }
            else
            {
                dryG = 1.0f - p.mix + p.mix * curve;
                revInL = inL * dryG; revInR = inR * dryG;
                revG = p.bloom * 0.8f;
            }
            float l = revInL, r = revInR;
            reverb.processStereo(&l, &r, 1);
            if (right != nullptr)
            {
                left[i] = inL * dryG + l * revG * 0.5f;
                right[i] = inR * dryG + r * revG * 0.5f;
            }
            else
            {
                left[i] = inL * dryG + 0.5f * (l + r) * revG * 0.5f;
            }
        }
    }

private:
    double sr = 44100.0;
    juce::Reverb reverb;
    float fast = 0.0f, floor = 0.0f, peak = 0.0f, hpLow = 0.0f, gain = 1.0f;
    int refractory = 0;
    bool dipping = false;
};

// Barycentric weights for the morph triangle: Glitch at the top, Pitch bottom-right,
// Swell bottom-left (x, y in 0..1, y down). Normalised so the strongest spell is 1,
// which makes the centre leave every spell at full strength.
inline std::array<float, 3> morphWeights(float x, float y) noexcept
{
    const float ax = 0.5f, ay = 0.09f, bx = 0.91f, by = 0.88f, cx = 0.09f, cy = 0.88f;
    const float det = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
    float wa = ((by - cy) * (x - cx) + (cx - bx) * (y - cy)) / det;
    float wb = ((cy - ay) * (x - cx) + (ax - cx) * (y - cy)) / det;
    wa = juce::jmax(0.0f, wa); wb = juce::jmax(0.0f, wb);
    float wc = juce::jmax(0.0f, 1.0f - wa - wb);
    const float m = juce::jmax(wa, wb, wc, 1.0e-6f);
    return { wa / m, wb / m, wc / m };
}
} // namespace mage::spell

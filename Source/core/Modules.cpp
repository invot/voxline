#include "Modules.h"

#include <algorithm>
#include <cmath>

namespace vx
{

namespace
{
    inline float crushSample (float x, int bits)
    {
        const int b = std::max (2, std::min (16, bits));
        const float levels = (float) ((1 << (b - 1)) - 1);
        return std::round (std::max (-1.0f, std::min (1.0f, x)) * levels) / levels;
    }

    inline float shapeSample (float x, int mode, float driveNorm)
    {
        switch (mode)
        {
            case SatTube:
            {
                const float bias = 0.18f + 0.22f * driveNorm;
                return std::tanh (x + bias) - std::tanh (bias);
            }
            case SatTape:
            {
                const float y = std::tanh (x);
                return y + 0.10f * y * y * y;
            }
            case SatFuzz:
            {
                return std::tanh (1.7f * x + 0.30f) - 0.291f;
            }
            default:
            {
                return std::tanh (x * 0.9f);
            }
        }
    }
}

//==============================================================================
void EqModule::prepare (double sampleRate)
{
    sr = std::max (8000.0, sampleRate);
    reset();
}

void EqModule::reset()
{
    hpf.reset(); lp.reset(); low.reset(); body.reset(); pres.reset();
    air.reset(); tiltLow.reset(); tiltHigh.reset();
}

void EqModule::setParams (const EqParams& p, float toneMacro)
{
    on = p.on;

    hpf.set (makeHighPass (sr, p.hpf, 0.70710678));
    lp.set (makeLowPass (sr, p.lp, 0.70710678));
    low.set (makeLowShelf (sr, p.lowFreq, p.lowGain, 0.9));
    body.set (makePeaking (sr, p.bodyFreq, p.bodyQ, p.bodyGain));
    pres.set (makePeaking (sr, p.presFreq, p.presQ, p.presGain));
    air.set (makeHighShelf (sr, p.airFreq, p.airGain, 0.9));

    const float tilt = (toneMacro - 0.5f) * 8.0f;
    tiltLow.set (makeLowShelf (sr, 700.0, -tilt, 0.8));
    tiltHigh.set (makeHighShelf (sr, 2600.0, tilt, 0.8));
}

void EqModule::process (float* l, float* r, int n)
{
    if (! on)
        return;

    for (int i = 0; i < n; ++i)
    {
        float a = l[i], b = r[i];
        hpf.process (a, b);
        lp.process (a, b);
        low.process (a, b);
        body.process (a, b);
        pres.process (a, b);
        air.process (a, b);
        tiltLow.process (a, b);
        tiltHigh.process (a, b);
        l[i] = a;
        r[i] = b;
    }
}

double EqModule::responseDb (double freq) const
{
    return hpf.magnitudeDb (freq, sr)
         + lp.magnitudeDb (freq, sr)
         + low.magnitudeDb (freq, sr)
         + body.magnitudeDb (freq, sr)
         + pres.magnitudeDb (freq, sr)
         + air.magnitudeDb (freq, sr)
         + tiltLow.magnitudeDb (freq, sr)
         + tiltHigh.magnitudeDb (freq, sr);
}

//==============================================================================
void DeEsserModule::prepare (double sampleRate)
{
    sr = std::max (8000.0, sampleRate);
    env.set (sr, 1.0, 45.0);
    attackCoef  = (float) std::exp (-1.0 / (sr * 0.001));
    releaseCoef = (float) std::exp (-1.0 / (sr * 0.050));
    reset();
}

void DeEsserModule::reset()
{
    hpf.reset();
    lpf.reset();
    env.reset();
    gain = 1.0f;
}

void DeEsserModule::process (float* l, float* r, int n, const DeEssParams& p)
{
    if (! p.on)
        return;

    hpf.set (makeHighPass (sr, p.freq, 0.70710678));
    lpf.set (makeLowPass (sr, p.freq, 0.70710678));

    for (int i = 0; i < n; ++i)
    {
        // The detector listens to a proper high-pass. The audio split is
        // complementary (high = input - low), so with no reduction the output
        // is the input exactly; summing a Butterworth LP and HP at the same
        // corner would notch the crossover frequency out.
        const float detL = hpf.processL (l[i]);
        const float detR = hpf.processR (r[i]);
        const float lowL  = lpf.processL (l[i]);
        const float lowR  = lpf.processR (r[i]);
        const float highL = l[i] - lowL;
        const float highR = r[i] - lowR;

        const float e = env.process (std::max (std::abs (detL), std::abs (detR)));
        const float over = gainToDb (e) - p.thresholdDb;
        const float grDb = over > 0.0f ? std::min (p.amountDb, over * 0.8f) : 0.0f;
        const float target = dbToGain (-grDb);
        const float c = target < gain ? attackCoef : releaseCoef;
        gain = target + c * (gain - target);

        l[i] = lowL + highL * gain;
        r[i] = lowR + highR * gain;
    }
}

//==============================================================================
void CompressorModule::prepare (double sampleRate)
{
    sr = std::max (8000.0, sampleRate);
    scHp.set (makeHighPass (sr, 120.0, 0.70710678));
    reset();
}

void CompressorModule::reset()
{
    env.reset();
    scHp.reset();
    gain = 1.0f;
    lastGr = 0.0f;
}

void CompressorModule::process (float* l, float* r, int n, const CompParams& p, float pressMacro)
{
    if (! p.on)
        return;

    env.set (sr, p.attackMs, p.releaseMs);

    const float threshold = p.thresholdDb - (pressMacro - 0.5f) * 10.0f;
    const float ratio = std::max (1.0f, p.ratio);
    const float slope = 1.0f - 1.0f / ratio;
    const float knee = 6.0f;
    const float mix = clamp01 (p.mix);

    lastGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float detL = scHp.processL (l[i]);
        const float detR = scHp.processR (r[i]);
        const float e = env.process (std::max (std::abs (detL), std::abs (detR)));

        const float over = gainToDb (e) - threshold;
        float grDb = 0.0f;

        if (over > knee * 0.5f)
            grDb = over * slope;
        else if (over > -knee * 0.5f)
        {
            const float x = over + knee * 0.5f;
            grDb = slope * x * x / (2.0f * knee);
        }

        grDb = std::min (40.0f, std::max (0.0f, grDb));
        const float g = dbToGain (-grDb + p.makeupDb);

        const float dryL = l[i], dryR = r[i];
        l[i] = dryL + (dryL * g - dryL) * mix;
        r[i] = dryR + (dryR * g - dryR) * mix;

        lastGr = std::max (lastGr, grDb);
    }
}

//==============================================================================
void SaturatorModule::prepare (double sampleRate)
{
    sr = std::max (8000.0, sampleRate);
    dcL.setLowPass (sr, 22.0);
    dcR.setLowPass (sr, 22.0);
    reset();
}

void SaturatorModule::reset()
{
    toneLow.reset();
    toneHigh.reset();
    dcL.reset();
    dcR.reset();
    heldL = heldR = 0.0f;
    holdCounter = 0;
    gain = 1.0f;
}

void SaturatorModule::process (float* l, float* r, int n, const SatParams& p, float gritMacro)
{
    on = p.on;
    if (! on)
        return;

    mode = p.mode;
    crushBits = p.bits;

    const float driveNorm = std::max (0.0f, std::min (1.0f, (p.drive * (0.55f + gritMacro * 0.9f)) * 0.01f));
    gain = dbToGain (2.0f + driveNorm * 26.0f);
    outGain = 1.0f / (0.55f + 0.45f * std::max (1.0f, gain));

    const float tilt = p.tone * 5.0f;
    toneLow.set (makeLowShelf (sr, 320.0, -tilt, 0.8));
    toneHigh.set (makeHighShelf (sr, 4200.0, tilt, 0.8));

    hold = 1 + (int) (driveNorm * 7.0f);
    if (holdCounter > hold)
        holdCounter = 0;

    const float mix = clamp01 (p.mix);

    for (int i = 0; i < n; ++i)
    {
        const float dryL = l[i], dryR = r[i];
        float wetL, wetR;

        if (mode == SatCrush)
        {
            if (holdCounter <= 0)
            {
                heldL = crushSample (dryL * gain, crushBits);
                heldR = crushSample (dryR * gain, crushBits);
                holdCounter = hold;
            }

            --holdCounter;
            wetL = heldL * outGain * 1.4f;
            wetR = heldR * outGain * 1.4f;
        }
        else
        {
            wetL = shapeSample (dryL * gain, mode, driveNorm) * outGain;
            wetR = shapeSample (dryR * gain, mode, driveNorm) * outGain;
        }

        wetL = dcL.highPass (wetL);
        wetR = dcR.highPass (wetR);

        toneLow.process (wetL, wetR);
        toneHigh.process (wetL, wetR);

        l[i] = dryL + (wetL - dryL) * mix;
        r[i] = dryR + (wetR - dryR) * mix;
    }
}

//==============================================================================
void MotionModule::prepare (double sampleRate, int)
{
    sr = std::max (8000.0, sampleRate);
    delayL.prepare ((int) (sr * 0.12) + 8);
    delayR.prepare ((int) (sr * 0.12) + 8);
    shiftL.prepare (sr, 30.0f, 14.0f);
    shiftR.prepare (sr, 30.0f, 14.0f);
    lfo.reset (0.0f);
    reset();
}

void MotionModule::reset()
{
    delayL.reset();
    delayR.reset();
    shiftL.reset();
    shiftR.reset();
    lfo.reset (0.0f);
}

void MotionModule::process (float* l, float* r, int n, const MotionParams& p)
{
    mode = p.mode;

    if (! p.on)
    {
        on = false;
        return;
    }

    if (! on)
    {
        reset();   // don't replay whatever was in the lines when it was switched off
        on = true;
    }

    const float depth = clamp01 (p.depth);
    const float mix = clamp01 (p.mix);
    const float rate = std::max (0.01f, p.rate);

    const float rPhase = mode == MotionChorus ? 0.25f
                       : mode == MotionFlanger ? 0.33f
                       : 0.5f;

    for (int i = 0; i < n; ++i)
    {
        const float phase = lfo.phase;
        float wetL = 0.0f, wetR = 0.0f;

        if (mode == MotionMicroshift)
        {
            wetL = shiftL.process (l[i], 9.0f);
            wetR = shiftR.process (r[i], -9.0f);
        }
        else if (mode == MotionDoubler)
        {
            const float baseMs = 18.0f + 10.0f * clamp01 (p.width);
            const float wobble = 0.25f * Lfo::sine (phase);
            const float dL = (baseMs + wobble) * 0.001f * (float) sr;
            const float dR = (baseMs + 7.0f + 0.25f * Lfo::sine (phase + rPhase)) * 0.001f * (float) sr;

            wetL = delayL.read (dL);
            wetR = delayR.read (dR);
            delayL.push (l[i]);
            delayR.push (r[i]);
        }
        else
        {
            const bool chorus = mode == MotionChorus;
            const float baseMs = chorus ? 13.0f : 1.8f;
            const float depthMs = (chorus ? 8.0f : 3.8f) * depth;

            const float modL = 0.5f + 0.5f * Lfo::sine (phase);
            const float modR = 0.5f + 0.5f * Lfo::sine (phase + rPhase);
            const float dL = (baseMs + depthMs * modL) * 0.001f * (float) sr;
            const float dR = (baseMs + depthMs * modR) * 0.001f * (float) sr;

            wetL = delayL.read (dL);
            wetR = delayR.read (dR);

            const float fb = std::max (-0.9f, std::min (0.9f, p.feedback));
            delayL.push (l[i] + wetL * fb);
            delayR.push (r[i] + wetR * fb);
        }

        lfo.tick (rate, sr);

        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * (0.4f + 1.2f * clamp01 (p.width));
        wetL = mid + side;
        wetR = mid - side;

        l[i] = l[i] * (1.0f - mix) + wetL * mix;
        r[i] = r[i] * (1.0f - mix) + wetR * mix;
    }
}

//==============================================================================
void DelayModule::prepare (double sampleRate, int)
{
    sr = std::max (8000.0, sampleRate);
    delayL.prepare ((int) (sr * maxDelaySeconds) + 8);
    delayR.prepare ((int) (sr * maxDelaySeconds) + 8);
    timeSamples.setTime (sr, 120.0f);
    timeSamples.snap (300.0f * 0.001f * (float) sr);
    throwSmooth.setTime (sr, 40.0f);
    throwSmooth.snap (0.0f);
    lpL.setLowPass (sr, 5000.0);
    lpR.setLowPass (sr, 5000.0);
    hpL.setLowPass (sr, 120.0);
    hpR.setLowPass (sr, 120.0);
    timePrimed = false;
    reset();
}

void DelayModule::reset()
{
    delayL.reset();
    delayR.reset();
    lpL.reset();
    lpR.reset();
    hpL.reset();
    hpR.reset();
    throwSmooth.snap (0.0f);
    timePrimed = false;
    on = false;
}

void DelayModule::process (float* l, float* r, int n, const DelayParams& p, const float* duck, float throwAmount)
{
    // THROW works whether or not the delay module is switched on: with the
    // module off the base send is zero and only the throw opens it.
    const float throwTarget = clamp01 (throwAmount);
    const bool active = p.on || throwTarget > 0.0f || throwSmooth.value() > 1.0e-4f;

    if (! active)
    {
        on = false;
        return;
    }

    if (! on)
    {
        // Coming back on: start from silence, not from a tail frozen when the
        // module was last switched off.
        delayL.reset();
        delayR.reset();
        lpL.reset(); lpR.reset(); hpL.reset(); hpR.reset();
        timePrimed = false;
        on = true;
    }

    float delayMs = p.timeMs;

    if (p.sync != SyncFree)
        delayMs = (60000.0f / std::max (minBpm, p.bpm)) * syncBeats (p.sync);

    timeSamples.target = std::max (1.0f, std::min ((float) (sr * maxDelaySeconds), delayMs * 0.001f * (float) sr));

    if (! timePrimed)
    {
        timeSamples.snap (timeSamples.target);
        timePrimed = true;
    }

    // Fast in, slow out, so a thrown phrase rings out after the button lifts.
    throwSmooth.target = throwTarget;
    throwSmooth.setTime (sr, throwTarget > throwSmooth.value() ? 30.0f : 700.0f);
    feedback = std::max (0.0f, std::min (0.92f, p.feedback));
    mix = p.on ? clamp01 (p.mix) : 0.0f;
    duckAmount = clamp01 (p.duck);
    pingPong = p.pingPong;

    lpL.setLowPass (sr, p.tone);
    lpR.setLowPass (sr, p.tone);
    hpL.setLowPass (sr, 120.0);
    hpR.setLowPass (sr, 120.0);

    for (int i = 0; i < n; ++i)
    {
        const float d = timeSamples.next();
        const float wetL = delayL.read (d);
        const float wetR = delayR.read (d);

        float fbl = hpL.highPass (lpL.lowPass (wetL));
        float fbr = hpR.highPass (lpR.lowPass (wetR));

        const float t = throwSmooth.next();
        const float fb = std::min (0.94f, feedback + t * 0.25f);

        const float dryL = l[i], dryR = r[i];

        if (pingPong)
        {
            delayL.push (0.5f * (dryL + dryR) + fbr * fb);
            delayR.push (fbl * fb);
        }
        else
        {
            delayL.push (dryL + fbl * fb);
            delayR.push (dryR + fbr * fb);
        }

        const float wetMix = std::min (1.4f, mix + t * 0.55f);
        const float g = wetMix * (1.0f - duckAmount * (duck != nullptr ? clamp01 (duck[i]) : 0.0f));

        l[i] = dryL + wetL * g;
        r[i] = dryR + wetR * g;
    }
}

//==============================================================================
void LimiterModule::prepare (double sampleRate, int maxBlock)
{
    sr = std::max (8000.0, sampleRate);
    lookahead = std::max (8, (int) (sr * 0.0015));

    audioL.prepare (lookahead + maxBlock + 8, 0.0f);
    audioR.prepare (lookahead + maxBlock + 8, 0.0f);

    // lookahead + 1 live entries at most, plus the empty slot a ring needs.
    minValue.assign ((size_t) (lookahead + 2), 1.0f);
    minIndex.assign ((size_t) (lookahead + 2), 0);

    releaseCoef = (float) std::exp (-1.0 / (sr * 0.080));
    gain = 1.0f;
    lastGr = 0.0f;
    reset();
}

void LimiterModule::reset()
{
    audioL.reset (0.0f);
    audioR.reset (0.0f);
    std::fill (minValue.begin(), minValue.end(), 1.0f);
    std::fill (minIndex.begin(), minIndex.end(), 0);
    minHead = minTail = 0;
    sampleCounter = 0;
    gain = 1.0f;
    lastGr = 0.0f;
}

void LimiterModule::process (float* l, float* r, int n, bool on, float ceilingDb, float releaseMs)
{
    ceiling = dbToGain (std::min (-0.05f, ceilingDb));
    releaseCoef = (float) std::exp (-1.0 / (sr * std::max (5.0f, releaseMs) * 0.001));
    lastGr = 0.0f;

    // The attack ramps down across the look-ahead window, so the gain reaches
    // the target by the time the peak leaves the delay line instead of
    // stepping on the peak sample itself.
    const float attackCoef = (float) std::exp (-3.0 / (double) std::max (1, lookahead));
    const int window = (int) minValue.size();

    for (int i = 0; i < n; ++i)
    {
        const float inL = l[i], inR = r[i];
        audioL.push (inL);
        audioR.push (inR);

        float target = 1.0f;

        if (on)
        {
            const float peak = std::max (std::abs (inL), std::abs (inR));
            if (peak > ceiling)
                target = ceiling / peak;
        }

        // Sliding minimum of the target over the look-ahead window (a
        // monotonic ring queue, O(1) amortised per sample).
        const long long now = sampleCounter++;

        // Expire first: the window is [now - lookahead, now], which is the
        // input span that covers the delayed sample leaving this call.
        while (minHead != minTail && minIndex[(size_t) minHead] < now - lookahead)
            minHead = (minHead + 1) % window;

        while (minHead != minTail)
        {
            const int last = (minTail - 1 + window) % window;
            if (minValue[(size_t) last] > target)
                minTail = last;
            else
                break;
        }

        minValue[(size_t) minTail] = target;
        minIndex[(size_t) minTail] = now;
        minTail = (minTail + 1) % window;

        const float held = minHead != minTail ? minValue[(size_t) minHead] : 1.0f;

        if (held < gain)
            gain = held + attackCoef * (gain - held);
        else
            gain = releaseCoef * gain + (1.0f - releaseCoef) * held;

        const float applied = gain;
        // push() already advanced the cursor; retain the full lookahead delay.
        float outL = audioL.read ((float) lookahead + 1.0f) * applied;
        float outR = audioR.read ((float) lookahead + 1.0f) * applied;

        if (on)
        {
            outL = std::max (-ceiling, std::min (ceiling, outL));
            outR = std::max (-ceiling, std::min (ceiling, outR));
            lastGr = std::max (lastGr, gainToDb (1.0f / std::max (0.0001f, applied)));
        }

        l[i] = outL;
        r[i] = outR;
    }
}

} // namespace vx

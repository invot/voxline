#pragma once

#include "Biquad.h"
#include "DspUtil.h"
#include "Params.h"

#include <vector>

namespace vx
{

//==============================================================================
class EqModule
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const EqParams&, float toneMacro);
    void process (float* l, float* r, int n);
    double responseDb (double freq) const;

private:
    double sr = 48000.0;
    StereoBiquad hpf, lp, low, body, pres, air, tiltLow, tiltHigh;
    bool on = true;
};

//==============================================================================
class DeEsserModule
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (float* l, float* r, int n, const DeEssParams&);

private:
    double sr = 48000.0;
    StereoBiquad hpf, lpf;
    EnvelopeFollower env;
    float gain = 1.0f;
    float attackCoef = 0.0f, releaseCoef = 0.0f;
};

//==============================================================================
class CompressorModule
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (float* l, float* r, int n, const CompParams&, float pressMacro);

    float lastGainReductionDb() const noexcept { return lastGr; }

private:
    double sr = 48000.0;
    EnvelopeFollower env;
    StereoBiquad scHp;
    float lastGr = 0.0f;
    float gain = 1.0f;
};

//==============================================================================
class SaturatorModule
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (float* l, float* r, int n, const SatParams&, float gritMacro);

private:
    double sr = 48000.0;
    StereoBiquad toneLow, toneHigh;
    OnePole dcL, dcR;
    float gain = 1.0f;
    float outGain = 1.0f;
    int mode = SatTube;
    int crushBits = 8;
    int hold = 1, holdCounter = 0;
    float heldL = 0.0f, heldR = 0.0f;
    bool on = true;
};

//==============================================================================
class MotionModule
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, const MotionParams&);

private:
    double sr = 48000.0;
    DelayLine delayL, delayR;
    DetuneHead shiftL, shiftR;
    Lfo lfo;
    float phaseR = 0.25f;
    float lfoRB = 0.0f;
    int mode = MotionChorus;
    bool on = false;
};

//==============================================================================
class DelayModule
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, const DelayParams&, const float* duck, float throwAmount);

private:
    double sr = 48000.0;
    DelayLine delayL, delayR;
    OnePole lpL, lpR, hpL, hpR;
    Smoothed timeSamples;
    Smoothed throwSmooth;
    float feedback = 0.25f;
    float mix = 0.12f;
    float duckAmount = 0.35f;
    bool pingPong = true;
    bool on = false;
    bool timePrimed = false;
};

//==============================================================================
class LimiterModule
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, bool on, float ceilingDb, float releaseMs);

    int lookaheadSamples() const noexcept { return lookahead; }
    float lastGainReductionDb() const noexcept { return lastGr; }

private:
    double sr = 48000.0;
    int lookahead = 72;
    DelayLine audioL, audioR, gainLine;
    float releaseCoef = 0.0f;
    float gain = 1.0f;
    float lastGr = 0.0f;
    float ceiling = 0.99f;
};

//==============================================================================
struct WidthModule
{
    static void apply (float& l, float& r, float width) noexcept
    {
        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r) * width;
        l = mid + side;
        r = mid - side;
    }
};

} // namespace vx

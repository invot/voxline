#pragma once

#include "Biquad.h"
#include "DspUtil.h"
#include "Params.h"

namespace vx
{

/** An eight-line feedback delay network with input diffusion, damping,
    pre-delay, return EQ and an optional octave-up shimmer path. */
class ReverbModule
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, const VerbParams&, const float* duck, float spaceMacro);

private:
    static constexpr int kLines = 8;

    void updateSize (float size);

    double sr = 48000.0;
    DelayLine lines[kLines];
    OnePole dampers[kLines];
    float baseLen[kLines] = { 1553.0f, 1613.0f, 1693.0f, 1777.0f,
                              1861.0f, 1949.0f, 2039.0f, 2131.0f };
    float lineDelay[kLines] {};
    float lineFeedback[kLines] {};
    float modPhase[kLines] {};
    float modRate[kLines] {};

    DelayLine preL, preR;
    Smoothed preSamples;
    Allpass apL1, apL2, apR1, apR2;

    StereoBiquad outHp, outLp;
    DetuneHead shimL, shimR;

    float lastSize = -1.0f;
    float shimmerTailL = 0.0f, shimmerTailR = 0.0f;
    float shimmerAmount = 0.0f;
    bool on = false;
};

} // namespace vx

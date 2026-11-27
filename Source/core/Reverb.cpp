#include "Reverb.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace vx
{

namespace
{
    inline void hadamard8 (float* v)
    {
        for (int step = 1; step < 8; step *= 2)
        {
            for (int i = 0; i < 8; i += step * 2)
            {
                for (int j = 0; j < step; ++j)
                {
                    const float a = v[i + j];
                    const float b = v[i + j + step];
                    v[i + j] = a + b;
                    v[i + j + step] = a - b;
                }
            }
        }

        const float n = 0.3535533905932738f;   // 1 / sqrt(8)
        for (int i = 0; i < 8; ++i)
            v[i] *= n;
    }
}

void ReverbModule::prepare (double sampleRate, int)
{
    sr = std::max (8000.0, sampleRate);

    for (int i = 0; i < kLines; ++i)
    {
        const int maxLen = (int) (baseLen[i] * (sr / 48000.0) * 1.5) + 24;
        lines[i].prepare (maxLen);
        dampers[i].setLowPass (sr, 6000.0);
        modPhase[i] = (float) i / 8.0f;
        modRate[i] = 0.061f + 0.023f * (float) i;
    }

    preL.prepare ((int) (sr * 0.16) + 8);
    preR.prepare ((int) (sr * 0.16) + 8);
    preSamples.setTime (sr, 40.0f);
    preSamples.snap (0.0f);

    apL1.prepare ((int) (sr * 0.0047) + 2, 0.55f);
    apL2.prepare ((int) (sr * 0.0079) + 2, 0.62f);
    apR1.prepare ((int) (sr * 0.0053) + 2, 0.57f);
    apR2.prepare ((int) (sr * 0.0091) + 2, 0.60f);

    shimL.prepare (sr, 90.0f, 8.0f);
    shimR.prepare (sr, 90.0f, 8.0f);

    lastSize = -1.0f;
    shimmerTailL = shimmerTailR = 0.0f;
    reset();
}

void ReverbModule::reset()
{
    for (auto& line : lines)
        line.reset();

    for (auto& damper : dampers)
        damper.reset();

    preL.reset();
    preR.reset();
    apL1.reset();
    apL2.reset();
    apR1.reset();
    apR2.reset();
    outHp.reset();
    outLp.reset();
    shimL.reset();
    shimR.reset();

    shimmerTailL = shimmerTailR = 0.0f;
}

void ReverbModule::updateSize (float size)
{
    const float s = clamp01 (size);
    const float scale = 0.55f + 0.9f * s;
    const float rt60 = 0.45f + 7.0f * std::pow (s, 2.2f);

    for (int i = 0; i < kLines; ++i)
    {
        const float len = baseLen[i] * (float) (sr / 48000.0) * scale;
        lineDelay[i] = len;
        lineFeedback[i] = std::min (0.9995f, (float) std::pow (10.0, -3.0 * (double) len / (double) (rt60 * (float) sr)));
    }
}

void ReverbModule::process (float* l, float* r, int n, const VerbParams& p, const float* duck, float spaceMacro)
{
    on = p.on;
    if (! on)
        return;

    if (std::abs (p.size - lastSize) > 0.0005f)
    {
        updateSize (p.size);
        lastSize = p.size;
    }

    const float damping = clamp01 (p.damp);
    const float dampCut = 700.0f + (1.0f - damping) * (1.0f - damping) * 15000.0f;
    for (auto& damper : dampers)
        damper.setLowPass (sr, dampCut);

    outHp.set (makeHighPass (sr, p.hp, 0.70710678));
    outLp.set (makeLowPass (sr, p.lp, 0.70710678));

    preSamples.target = std::max (0.0f, std::min ((float) (sr * 0.15), p.preMs * 0.001f * (float) sr));
    shimmerAmount = clamp01 (p.shimmer);

    const float wetGain = clamp01 (p.mix) * (0.35f + 1.3f * clamp01 (spaceMacro));

    if (wetGain <= 0.00001f)
        return;

    for (int i = 0; i < n; ++i)
    {
        preL.push (l[i]);
        preR.push (r[i]);

        const float pre = preSamples.next();
        float inL = preL.read (pre);
        float inR = preR.read (pre);

        inL = apL2.process (apL1.process (inL));
        inR = apR2.process (apR1.process (inR));

        if (shimmerAmount > 0.0001f)
        {
            const float shimmerIn = 0.5f * (shimmerTailL + shimmerTailR) * shimmerAmount * 0.28f;
            const float sl = shimL.process (shimmerIn, 1196.0f);
            const float sr2 = shimR.process (shimmerIn, 1204.0f);
            inL += std::tanh (sl);
            inR += std::tanh (sr2);
        }

        float outs[8];

        for (int k = 0; k < 8; ++k)
        {
            const float mod = 2.5f * Lfo::sine (modPhase[k]);
            modPhase[k] += modRate[k] / (float) sr;
            if (modPhase[k] >= 1.0f)
                modPhase[k] -= 1.0f;

            outs[k] = lines[k].read (lineDelay[k] + mod);
            if (! std::isfinite (outs[k]))
                outs[k] = 0.0f;
        }

        float fb[8];

        for (int k = 0; k < 8; ++k)
            fb[k] = dampers[k].lowPass (outs[k]);

        hadamard8 (fb);

        for (int k = 0; k < 8; ++k)
        {
            const float input = (k < 4 ? inL : inR) * (k % 4 == 0 ? 1.0f : 0.62f);
            float next = fb[k] * lineFeedback[k] + input * 0.45f;

            if (! std::isfinite (next) || std::abs (next) > 6.0f)
                next = 0.0f;

            lines[k].push (next);
        }

        float wetL = (outs[0] + outs[2] + outs[4] + outs[6]) * 0.25f;
        float wetR = (outs[1] + outs[3] + outs[5] + outs[7]) * 0.25f;

        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * (0.3f + 1.2f * clamp01 (p.width));
        wetL = mid + side;
        wetR = mid - side;

        outHp.process (wetL, wetR);
        outLp.process (wetL, wetR);

        shimmerTailL = wetL;
        shimmerTailR = wetR;

        const float duckGain = 1.0f - clamp01 (p.duck) * (duck != nullptr ? clamp01 (duck[i]) : 0.0f);
        l[i] += wetL * wetGain * duckGain;
        r[i] += wetR * wetGain * duckGain;
    }
}

} // namespace vx

#include "Chain.h"

#include <algorithm>
#include <cmath>

namespace vx
{

void VocalChain::prepare (double sampleRate, int maxBlock)
{
    sr = std::max (8000.0, sampleRate);
    maxBlockSize = std::max (16, maxBlock);

    tune.prepare (sr, maxBlockSize);
    eq.prepare (sr);
    deEss.prepare (sr);
    comp1.prepare (sr);
    comp2.prepare (sr);
    sat.prepare (sr);
    motion.prepare (sr, maxBlockSize);
    delay.prepare (sr, maxBlockSize);
    reverb.prepare (sr, maxBlockSize);
    limiter.prepare (sr, maxBlockSize);

    latency = tune.latencySamples() + limiter.lookaheadSamples();

    bypassL.prepare (latency + maxBlockSize + 8);
    bypassR.prepare (latency + maxBlockSize + 8);

    duckFollower.set (sr, 8.0f, 120.0f);
    duckScratch.assign ((size_t) maxBlockSize, 0.0f);

    meterIn = meterOut = 0.0f;
    reset();
}

void VocalChain::reset()
{
    tune.reset();
    eq.reset();
    deEss.reset();
    comp1.reset();
    comp2.reset();
    sat.reset();
    motion.reset();
    delay.reset();
    reverb.reset();
    limiter.reset();

    bypassL.reset();
    bypassR.reset();
    duckFollower.reset();
    std::fill (duckScratch.begin(), duckScratch.end(), 0.0f);

    meterIn = meterOut = 0.0f;
    inMeter.store (0.0f);
    outMeter.store (0.0f);
    comp1Meter.store (0.0f);
    comp2Meter.store (0.0f);
    limiterMeter.store (0.0f);
    centsMeter.store (0.0f);
}

void VocalChain::process (float* l, float* r, int n, const ChainParams& p)
{
    if (n <= 0)
        return;

    if (n > (int) duckScratch.size())
    {
        duckScratch.resize ((size_t) n);
        bypassL.prepare (latency + n + 8);
        bypassR.prepare (latency + n + 8);
    }

    float peakIn = 0.0f;

    for (int i = 0; i < n; ++i)
        peakIn = std::max (peakIn, std::max (std::abs (l[i]), std::abs (r[i])));

    meterIn = std::max (peakIn, meterIn * 0.86f);

    if (p.bypass)
    {
        float peakOut = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float inL = l[i], inR = r[i];

            l[i] = bypassL.read ((float) latency);
            r[i] = bypassR.read ((float) latency);

            bypassL.push (inL);
            bypassR.push (inR);

            peakOut = std::max (peakOut, std::max (std::abs (l[i]), std::abs (r[i])));
        }

        meterOut = std::max (peakOut, meterOut * 0.86f);
        inMeter.store (meterIn);
        outMeter.store (meterOut);
        comp1Meter.store (0.0f);
        comp2Meter.store (0.0f);
        limiterMeter.store (0.0f);
        centsMeter.store (0.0f);
        return;
    }

    tune.process (l, r, n, p.tune);

    eq.setParams (p.eq, p.tone);
    eq.process (l, r, n);

    deEss.process (l, r, n, p.deess);

    comp1.process (l, r, n, p.comp1, p.press);
    comp2.process (l, r, n, p.comp2, p.press);

    sat.process (l, r, n, p.sat, p.grit);
    motion.process (l, r, n, p.motion);

    for (int i = 0; i < n; ++i)
    {
        const float e = duckFollower.process (std::max (std::abs (l[i]), std::abs (r[i])));
        duckScratch[(size_t) i] = std::min (1.0f, e / 0.045f);
    }

    DelayParams dp = p.delay;
    const float spaceScale = 0.35f + 1.3f * clamp01 (p.space);
    dp.mix = clamp01 (dp.mix * spaceScale);
    dp.bpm = p.bpm;
    delay.process (l, r, n, dp, duckScratch.data(), p.throwAmount);

    reverb.process (l, r, n, p.verb, duckScratch.data(), p.space);

    for (int i = 0; i < n; ++i)
    {
        WidthModule::apply (l[i], r[i], p.width);
        const float g = dbToGain (p.levelDb);
        l[i] *= g;
        r[i] *= g;
    }

    limiter.process (l, r, n, p.limiterOn, p.ceilingDb, p.limiterReleaseMs);

    float peakOut = 0.0f;

    for (int i = 0; i < n; ++i)
        peakOut = std::max (peakOut, std::max (std::abs (l[i]), std::abs (r[i])));

    meterOut = std::max (peakOut, meterOut * 0.86f);

    inMeter.store (meterIn);
    outMeter.store (meterOut);
    comp1Meter.store (comp1.lastGainReductionDb());
    comp2Meter.store (comp2.lastGainReductionDb());
    limiterMeter.store (limiter.lastGainReductionDb());
    centsMeter.store (tune.shiftCents());
}

} // namespace vx

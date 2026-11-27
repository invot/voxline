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
    dryL.assign ((size_t) maxBlockSize, 0.0f);
    dryR.assign ((size_t) maxBlockSize, 0.0f);

    bypassMix.setTime (sr, 3.0f);   // one-pole time constant: ~25 ms to settle
    levelGain.setTime (sr, 20.0f);

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
    smoothersPrimed = false;

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
        // Hosts should never exceed the prepared block size; survive it anyway.
        duckScratch.resize ((size_t) n);
        dryL.resize ((size_t) n);
        dryR.resize ((size_t) n);
        bypassL.prepare (latency + n + 8);
        bypassR.prepare (latency + n + 8);
    }

    float peakIn = 0.0f;

    // The dry path is fed on every block, bypassed or not, so engaging bypass
    // plays the input from exactly `latency` samples ago instead of whatever
    // was left in the line the last time bypass was used.
    for (int i = 0; i < n; ++i)
    {
        peakIn = std::max (peakIn, std::max (std::abs (l[i]), std::abs (r[i])));

        dryL[(size_t) i] = bypassL.read ((float) latency);
        dryR[(size_t) i] = bypassR.read ((float) latency);
        bypassL.push (l[i]);
        bypassR.push (r[i]);
    }

    meterIn = std::max (peakIn, meterIn * 0.86f);

    if (! smoothersPrimed)
    {
        bypassMix.snap (p.bypass ? 1.0f : 0.0f);
        levelGain.snap (dbToGain (p.levelDb));
        smoothersPrimed = true;
    }

    bypassMix.target = p.bypass ? 1.0f : 0.0f;
    levelGain.target = dbToGain (p.levelDb);

    // The chain keeps running under bypass so every stage stays warm and the
    // switch back is a crossfade, not a jump into stale state.
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
        const float g = levelGain.next();
        l[i] *= g;
        r[i] *= g;
    }

    limiter.process (l, r, n, p.limiterOn, p.ceilingDb, p.limiterReleaseMs);

    float peakOut = 0.0f;
    const bool settled = std::abs (bypassMix.value() - bypassMix.target) < 1.0e-4f;

    if (! (settled && bypassMix.target < 0.5f))
    {
        for (int i = 0; i < n; ++i)
        {
            const float b = bypassMix.next();
            l[i] += (dryL[(size_t) i] - l[i]) * b;
            r[i] += (dryR[(size_t) i] - r[i]) * b;
        }

        // Land exactly on 0 or 1 so a settled bypass is bit-exact dry.
        if (std::abs (bypassMix.value() - bypassMix.target) < 1.0e-4f)
            bypassMix.snap (bypassMix.target);
    }

    for (int i = 0; i < n; ++i)
        peakOut = std::max (peakOut, std::max (std::abs (l[i]), std::abs (r[i])));

    meterOut = std::max (peakOut, meterOut * 0.86f);

    const bool bypassed = p.bypass && settled;

    inMeter.store (meterIn);
    outMeter.store (meterOut);
    comp1Meter.store (bypassed ? 0.0f : comp1.lastGainReductionDb());
    comp2Meter.store (bypassed ? 0.0f : comp2.lastGainReductionDb());
    limiterMeter.store (bypassed ? 0.0f : limiter.lastGainReductionDb());
    centsMeter.store (bypassed ? 0.0f : tune.shiftCents());
}

} // namespace vx

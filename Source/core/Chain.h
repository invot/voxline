#pragma once

#include "Modules.h"
#include "Reverb.h"
#include "Tune.h"

#include <atomic>
#include <vector>

namespace vx
{

/** Tune -> EQ -> de-ess -> serial compressors -> drive -> motion -> time ->
    width -> limiter. The tune stage is always running so the reported latency
    never changes when it is switched off. */
class VocalChain
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, const ChainParams&);

    int latencySamples() const noexcept { return latency; }

    float inputMeter() const noexcept { return inMeter.load (std::memory_order_relaxed); }
    float outputMeter() const noexcept { return outMeter.load (std::memory_order_relaxed); }
    float comp1Gr() const noexcept { return comp1Meter.load (std::memory_order_relaxed); }
    float comp2Gr() const noexcept { return comp2Meter.load (std::memory_order_relaxed); }
    float limiterGr() const noexcept { return limiterMeter.load (std::memory_order_relaxed); }
    float tuneCents() const noexcept { return centsMeter.load (std::memory_order_relaxed); }

private:
    TuneModule tune;
    EqModule eq;
    DeEsserModule deEss;
    CompressorModule comp1, comp2;
    SaturatorModule sat;
    MotionModule motion;
    DelayModule delay;
    ReverbModule reverb;
    LimiterModule limiter;

    DelayLine bypassL, bypassR;
    EnvelopeFollower duckFollower;
    std::vector<float> duckScratch;

    double sr = 48000.0;
    int maxBlockSize = 512;
    int latency = 0;
    float meterIn = 0.0f, meterOut = 0.0f;

    std::atomic<float> inMeter { 0.0f };
    std::atomic<float> outMeter { 0.0f };
    std::atomic<float> comp1Meter { 0.0f };
    std::atomic<float> comp2Meter { 0.0f };
    std::atomic<float> limiterMeter { 0.0f };
    std::atomic<float> centsMeter { 0.0f };
};

} // namespace vx

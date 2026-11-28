#include "Tune.h"

#include "signalsmith-stretch.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

namespace vx
{

struct TuneModule::Impl
{
    using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

    Stretch stretch;

    std::vector<float> outL, outR;          // one block of stretcher output
    std::vector<float> stretchBufL, stretchBufR;
    int stretchCount = 1; // one zero sample of cubic interpolation history
    double resamplePos = 1.0;
    double carry = 0.0;

    std::vector<float> fifoL, fifoR;
    int fifoCapacity = 0;
    int fifoRead = 0, fifoWrite = 0, fifoCount = 0;

    int prime = 256;

    inline void fifoPush (float l, float r) noexcept
    {
        if (fifoCount >= fifoCapacity)
            return;

        fifoL[(size_t) fifoWrite] = l;
        fifoR[(size_t) fifoWrite] = r;
        if (++fifoWrite >= fifoCapacity)
            fifoWrite = 0;

        ++fifoCount;
    }

    inline void fifoPop (float& l, float& r) noexcept
    {
        if (fifoCount <= 0)
        {
            l = r = 0.0f;
            return;
        }

        l = fifoL[(size_t) fifoRead];
        r = fifoR[(size_t) fifoRead];

        if (++fifoRead >= fifoCapacity)
            fifoRead = 0;

        --fifoCount;
    }

    inline void pushSilence (int count) noexcept
    {
        for (int i = 0; i < count; ++i)
            fifoPush (0.0f, 0.0f);
    }
};

TuneModule::TuneModule() = default;
TuneModule::~TuneModule() = default;

//==============================================================================
void YinDetector::prepare (double sampleRate)
{
    sr = std::max (8000.0, sampleRate);
    decim = std::max (1, (int) std::lround (sr / 24000.0));
    internalSr = sr / (double) decim;

    antiAlias.c = makeLowPass (sr, std::min (1800.0, internalSr * 0.42), 0.70710678);

    frame = 1024;
    // The hop is counted in decimated samples, so derive it from the internal
    // rate: ~21 ms (half a frame) at every host rate. Deriving it from the host
    // rate made it longer than the whole frame at 96 kHz and above.
    hop = std::max (64, std::min (frame / 2, (int) (internalSr * 0.0213)));

    ring.assign ((size_t) frame, 0.0f);
    ordered.assign ((size_t) frame, 0.0f);

    const int tauMax = std::min (frame / 2 - 1, (int) (internalSr / 65.0));
    diff.assign ((size_t) std::max (8, tauMax + 2), 0.0f);
    cmnd.assign ((size_t) std::max (8, tauMax + 2), 0.0f);

    reset();
}

void YinDetector::reset()
{
    write = 0;
    filled = 0;
    sinceHop = 0;
    decimCounter = 0;
    lastHz = 0.0f;
    lastConf = 0.0f;
    antiAlias.reset();
    std::fill (ring.begin(), ring.end(), 0.0f);
}

void YinDetector::push (float x)
{
    x = antiAlias.process (x);

    if (++decimCounter >= decim)
    {
        decimCounter = 0;
        ring[(size_t) write] = x;
        if (++write >= frame)
            write = 0;

        if (filled < frame)
            ++filled;

        ++sinceHop;
    }
}

void YinDetector::analyse()
{
    if (filled < frame)
        return;

    for (int i = 0; i < frame; ++i)
    {
        int idx = write + i;
        if (idx >= frame)
            idx -= frame;
        ordered[(size_t) i] = ring[(size_t) idx];
    }

    const int tauMin = std::max (2, (int) (internalSr / 1200.0));
    const int tauMax = std::min (frame / 2 - 1, (int) (internalSr / 65.0));
    const int w = frame - tauMax;

    if (tauMax <= tauMin + 2 || w < 64)
        return;

    double running = 0.0;
    cmnd[0] = 1.0f;

    for (int tau = 1; tau <= tauMax; ++tau)
    {
        double sum = 0.0;

        for (int j = 0; j < w; ++j)
        {
            const double d = (double) ordered[(size_t) j] - (double) ordered[(size_t) (j + tau)];
            sum += d * d;
        }

        diff[(size_t) tau] = (float) sum;
        running += sum;
        cmnd[(size_t) tau] = (float) (running > 1.0e-12 ? (sum * (double) tau / running) : 0.0);
    }

    int best = -1;

    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (cmnd[(size_t) tau] < 0.18f)
        {
            while (tau + 1 <= tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau])
                ++tau;

            best = tau;
            break;
        }
    }

    if (best < 0)
    {
        float lowest = 1.0f;

        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < lowest)
            {
                lowest = cmnd[(size_t) tau];
                best = tau;
            }
        }
    }

    if (best < 1)
        return;

    float shift = 0.0f;

    if (best > tauMin && best < tauMax)
    {
        const float a = cmnd[(size_t) (best - 1)];
        const float b = cmnd[(size_t) best];
        const float c = cmnd[(size_t) (best + 1)];
        const float denom = a - 2.0f * b + c;

        if (std::abs (denom) > 1.0e-9f)
            shift = std::max (-1.0f, std::min (1.0f, 0.5f * (a - c) / denom));
    }

    const float tau = (float) best + shift;
    lastHz = (float) (internalSr / std::max (1.0f, tau));
    lastConf = std::max (0.0f, std::min (1.0f, 1.0f - cmnd[(size_t) best]));
    sinceHop = 0;
}

//==============================================================================
namespace
{
    inline float cubicAt (const std::vector<float>& x, int count, double pos) noexcept
    {
        const int i = (int) std::floor (pos);
        const double t = pos - (double) i;

        const float y0 = x[(size_t) std::max (0, i - 1)];
        const float y1 = x[(size_t) std::min (count - 1, i)];
        const float y2 = x[(size_t) std::min (count - 1, i + 1)];
        const float y3 = x[(size_t) std::min (count - 1, i + 2)];

        const double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
        const double a1 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        const double a2 = -0.5 * y0 + 0.5 * y2;
        const double a3 = y1;

        return (float) (((a0 * t + a1) * t + a2) * t + a3);
    }
}

void TuneModule::prepare (double sampleRate, int maxBlock)
{
    sr = std::max (8000.0, sampleRate);

    if (! impl)
        impl = std::make_unique<Impl>();

    impl->stretch.configure (2, 2048, 512);
    impl->stretch.reset();
    impl->stretch.setTransposeSemitones (0.0f);
    impl->stretch.setFormantSemitones (0.0f, false);

    const int stretchLatency = impl->stretch.inputLatency() + impl->stretch.outputLatency();

    const int blockCap = std::max (4096, maxBlock * 6 + 4096);
    impl->outL.assign ((size_t) blockCap, 0.0f);
    impl->outR.assign ((size_t) blockCap, 0.0f);
    impl->stretchBufL.assign ((size_t) blockCap, 0.0f);
    impl->stretchBufR.assign ((size_t) blockCap, 0.0f);
    impl->stretchCount = 1;
    impl->resamplePos = 1.0;
    impl->carry = 0.0;

    impl->fifoCapacity = std::max (4096, maxBlock * 6 + 4096);
    impl->fifoL.assign ((size_t) impl->fifoCapacity, 0.0f);
    impl->fifoR.assign ((size_t) impl->fifoCapacity, 0.0f);
    impl->fifoRead = impl->fifoWrite = impl->fifoCount = 0;
    impl->prime = std::min (512, impl->fifoCapacity / 4);
    impl->pushSilence (impl->prime);

    octL.prepare (sr, 26.0f, 3.0f);
    octR.prepare (sr, 26.0f, 3.0f);

    yin.prepare (sr);

    // Interpolation starts with one history sample, so the first generated
    // sample is preserved. The primed FIFO supplies the interpolation lookahead.
    latency = stretchLatency + impl->prime;

    smoothedSemis = 0.0f;
    lastHz = 0.0f;
    octaveActive = false;
}

void TuneModule::reset()
{
    if (! impl)
        return;

    impl->stretch.reset();
    impl->stretchCount = 1;
    impl->stretchBufL[0] = impl->stretchBufR[0] = 0.0f;
    impl->resamplePos = 1.0;
    impl->carry = 0.0;

    impl->fifoRead = impl->fifoWrite = impl->fifoCount = 0;
    impl->pushSilence (impl->prime);

    octL.reset();
    octR.reset();
    yin.reset();

    smoothedSemis = 0.0f;
    lastHz = 0.0f;
    octaveActive = false;
}

float TuneModule::targetForPitch (const TuneParams& p) const
{
    if (! p.on || p.amount <= 0.0001f || lastHz < 55.0f)
        return 0.0f;

    const float midi = 69.0f + 12.0f * std::log2 (lastHz / 440.0f);
    const int nearest = (int) std::lround (midi);
    const int target = nearestScaleNote (p.scale, p.key, nearest);
    const float semis = ((float) target - midi) * clamp01 (p.amount);

    return std::max (-1.9f, std::min (1.9f, semis));
}

void TuneModule::process (float* l, float* r, int n, const TuneParams& p)
{
    if (n <= 0 || ! impl)
        return;

    if (n > (int) impl->outL.size())
    {
        impl->outL.resize ((size_t) n);
        impl->outR.resize ((size_t) n);
        impl->stretchBufL.resize ((size_t) (n * 6 + 4096));
        impl->stretchBufR.resize ((size_t) (n * 6 + 4096));
    }

    double sum = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const float mono = 0.5f * (l[i] + r[i]);
        yin.push (mono);
        sum += (double) mono * (double) mono;
    }

    const float rms = (float) std::sqrt (sum / std::max (1, n));

    if (yin.ready())
    {
        yin.analyse();
        lastHz = yin.frequency();
    }

    float target = 0.0f;

    if (rms > 0.0015f && yin.confidence() > 0.55f
        && lastHz >= 55.0f && lastHz <= 1200.0f)
        target = targetForPitch (p);

    const float speed = std::max (1.0f, p.speedMs);
    const float coef = (float) std::exp (-(double) n / (sr * (double) speed * 0.001));
    smoothedSemis = target + coef * (smoothedSemis - target);
    smoothedSemis = std::max (-2.2f, std::min (2.2f, smoothedSemis));

    const double ratio = std::pow (2.0, (double) smoothedSemis / 12.0);

    // Generate ratio * n samples, then play the result back at `ratio`. The
    // resampler makes the pitch exact; the stretcher only has to handle a
    // near-unity time ratio, where it is clean.
    double desired = (double) n * ratio + impl->carry;
    int m = (int) std::floor (desired);
    impl->carry = desired - (double) m;

    // Fractional output counts carry across calls, including calls small
    // enough to produce no stretched samples. Do not force a minimum block.
    m = std::min (m, (int) impl->outL.size());

    impl->stretch.setTransposeSemitones (0.0f);
    // Switching the module off drops the formant shift and sub-octave too; the
    // stretcher itself keeps running so the latency never changes.
    impl->stretch.setFormantSemitones (-smoothedSemis + (p.on ? p.formant : 0.0f), false);
    impl->stretch.setFormantBase (lastHz > 55.0f ? lastHz / (float) sr : 0.0f);

    float* in[2] = { l, r };
    float* out[2] = { impl->outL.data(), impl->outR.data() };
    impl->stretch.process (in, n, out, m);

    // Append the newly stretched samples to the interpolation buffer.
    if (impl->stretchCount + m > (int) impl->stretchBufL.size())
    {
        const int keep = std::max (0, (int) impl->stretchCount - (int) std::floor (impl->resamplePos) + 1);
        if (keep > 0)
        {
            std::memmove (impl->stretchBufL.data(), impl->stretchBufL.data() + (impl->stretchCount - keep),
                          sizeof (float) * (size_t) keep);
            std::memmove (impl->stretchBufR.data(), impl->stretchBufR.data() + (impl->stretchCount - keep),
                          sizeof (float) * (size_t) keep);
        }
        impl->stretchCount = keep;
        impl->resamplePos = 1.0;

        if (impl->stretchCount + m > (int) impl->stretchBufL.size())
        {
            impl->stretchBufL.resize ((size_t) (impl->stretchCount + m + 64));
            impl->stretchBufR.resize ((size_t) (impl->stretchCount + m + 64));
        }
    }

    std::memcpy (impl->stretchBufL.data() + impl->stretchCount, impl->outL.data(),
                 sizeof (float) * (size_t) m);
    std::memcpy (impl->stretchBufR.data() + impl->stretchCount, impl->outR.data(),
                 sizeof (float) * (size_t) m);
    impl->stretchCount += m;

    while (impl->resamplePos + 2.0 < (double) impl->stretchCount)
    {
        const float sl = cubicAt (impl->stretchBufL, impl->stretchCount, impl->resamplePos);
        const float sr2 = cubicAt (impl->stretchBufR, impl->stretchCount, impl->resamplePos);
        impl->fifoPush (sl, sr2);
        impl->resamplePos += ratio;
    }

    // Drop what has been consumed, keeping one sample of history.
    const int drop = (int) std::floor (impl->resamplePos) - 1;

    if (drop > 0)
    {
        const int keep = impl->stretchCount - drop;
        if (keep > 0)
        {
            std::memmove (impl->stretchBufL.data(), impl->stretchBufL.data() + drop,
                          sizeof (float) * (size_t) keep);
            std::memmove (impl->stretchBufR.data(), impl->stretchBufR.data() + drop,
                          sizeof (float) * (size_t) keep);
        }
        impl->stretchCount = std::max (0, keep);
        impl->resamplePos -= (double) drop;
    }

    // The host expects exactly n samples.
    for (int i = 0; i < n; ++i)
        impl->fifoPop (l[i], r[i]);

    const float octMix = p.on ? clamp01 (p.octaveMix) : 0.0f;

    if (octMix > 0.0005f)
    {
        if (! octaveActive)
        {
            octL.reset();
            octR.reset();
            octaveActive = true;
        }

        for (int i = 0; i < n; ++i)
        {
            l[i] += octL.process (l[i], -1200.0f) * octMix * 0.65f;
            r[i] += octR.process (r[i], -1200.0f) * octMix * 0.65f;
        }
    }
    else
    {
        octaveActive = false;
    }
}

} // namespace vx

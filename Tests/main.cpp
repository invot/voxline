// Headless tests for the DSP core. No JUCE: the whole chain can be rendered
// from a console binary, which is how these checks and the tuning test run.

#include "core/Chain.h"
#include "core/Modules.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
    int failures = 0;

    void check (bool ok, const char* what)
    {
        std::printf ("%s  %s\n", ok ? "[ok]  " : "[FAIL]", what);

        if (! ok)
            ++failures;
    }

    constexpr double kSr = 48000.0;

    double rms (const std::vector<float>& x, int start, int end)
    {
        double sum = 0.0;
        int n = 0;

        for (int i = std::max (0, start); i < std::min ((int) x.size(), end); ++i)
        {
            sum += (double) x[(size_t) i] * (double) x[(size_t) i];
            ++n;
        }

        return std::sqrt (sum / std::max (1, n));
    }

    double peak (const std::vector<float>& x, int start = 0, int end = -1)
    {
        if (end < 0)
            end = (int) x.size();

        double p = 0.0;

        for (int i = std::max (0, start); i < std::min ((int) x.size(), end); ++i)
            p = std::max (p, (double) std::abs (x[(size_t) i]));

        return p;
    }

    bool allFinite (const std::vector<float>& x)
    {
        for (float v : x)
            if (! std::isfinite (v))
                return false;

        return true;
    }

    double autocorrelationHz (const std::vector<float>& x, int start, int end)
    {
        const int lo = (int) (kSr / 420.0);
        const int hi = (int) (kSr / 90.0);
        const int a = std::max (0, start);
        const int b = std::min ((int) x.size(), end);

        auto corr = [&] (int lag)
        {
            double sum = 0.0;
            for (int i = a; i < b - lag; ++i)
                sum += (double) x[(size_t) i] * (double) x[(size_t) (i + lag)];
            return sum;
        };

        double best = -1.0;
        int bestLag = lo;

        for (int lag = lo; lag <= hi; ++lag)
        {
            const double c = corr (lag);
            if (c > best)
            {
                best = c;
                bestLag = lag;
            }
        }

        if (bestLag <= lo || bestLag >= hi)
            return kSr / (double) bestLag;

        const double s0 = corr (bestLag - 1);
        const double s1 = corr (bestLag);
        const double s2 = corr (bestLag + 1);
        const double denom = s0 - 2.0 * s1 + s2;
        const double shift = std::abs (denom) > 1.0e-9 ? 0.5 * (s0 - s2) / denom : 0.0;

        return kSr / ((double) bestLag + std::max (-1.0, std::min (1.0, shift)));
    }

    double goertzel (const std::vector<float>& x, double hz, int start, int end)
    {
        const int n = std::max (1, end - start);
        const double w = 2.0 * 3.14159265358979323846 * hz / kSr;
        const double coeff = 2.0 * std::cos (w);
        double s0 = 0.0, s1 = 0.0, s2 = 0.0;

        for (int i = start; i < end; ++i)
        {
            s0 = (double) x[(size_t) i] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }

        const double real = s1 - s2 * std::cos (w);
        const double imag = s2 * std::sin (w);
        return 2.0 * std::sqrt (real * real + imag * imag) / (double) n;
    }

    void testBiquads()
    {
        vx::Biquad lp;
        lp.c = vx::makeLowPass (kSr, 1000.0, 0.70710678);
        const double low = lp.magnitudeDb (100.0, kSr);
        const double high = lp.magnitudeDb (10000.0, kSr);

        check (std::abs (low) < 0.2, "low-pass passes 100 Hz flat");
        check (high < -35.0, "low-pass rejects 10 kHz");

        vx::Biquad hp;
        hp.c = vx::makeHighPass (kSr, 100.0, 0.70710678);
        check (hp.magnitudeDb (30.0, kSr) < -14.0, "high-pass rejects 30 Hz");
        check (std::abs (hp.magnitudeDb (1000.0, kSr)) < 0.2, "high-pass passes 1 kHz");
    }

    void testCompressor()
    {
        vx::CompressorModule comp;
        comp.prepare (kSr);

        vx::CompParams p;
        p.thresholdDb = -20.0f;
        p.ratio = 4.0f;
        p.attackMs = 1.0f;
        p.releaseMs = 50.0f;
        p.mix = 1.0f;
        p.makeupDb = 0.0f;

        std::vector<float> l (48000, 0.0f), r (48000, 0.0f);
        double phase = 0.0;

        for (int i = 0; i < (int) l.size(); ++i)
        {
            phase += 2.0 * 3.14159265358979323846 * 1000.0 / kSr;
            l[(size_t) i] = 0.5f * (float) std::sin (phase);
            r[(size_t) i] = l[(size_t) i];
        }

        const double inRms = rms (l, 24000, 48000);

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            comp.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p, 0.5f);

        const double outRms = rms (l, 24000, 48000);

        check (outRms < inRms * 0.62, "compressor reduces a loud sine");
        check (comp.lastGainReductionDb() > 5.0f, "compressor reports real gain reduction");
    }

    void testDeEsser()
    {
        vx::DeEsserModule deess;
        deess.prepare (kSr);

        vx::DeEssParams p;
        p.freq = 7000.0f;
        p.thresholdDb = -40.0f;
        p.amountDb = 12.0f;

        std::vector<float> l (48000, 0.0f), r (48000, 0.0f);
        double a = 0.0, b = 0.0;

        for (int i = 0; i < (int) l.size(); ++i)
        {
            a += 2.0 * 3.14159265358979323846 * 1000.0 / kSr;
            b += 2.0 * 3.14159265358979323846 * 7200.0 / kSr;
            l[(size_t) i] = 0.12f * (float) std::sin (a) + 0.16f * (float) std::sin (b);
            r[(size_t) i] = l[(size_t) i];
        }

        const double sibBefore = goertzel (l, 7200.0, 24000, 46000);

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            deess.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p);

        const double sibAfter = goertzel (l, 7200.0, 24000, 46000);

        check (sibAfter < sibBefore * 0.72, "de-esser takes 7.2 kHz down");
        check (goertzel (l, 1000.0, 24000, 46000) > 0.06, "de-esser keeps the body");
    }

    void testLimiter()
    {
        vx::LimiterModule lim;
        lim.prepare (kSr, 512);

        std::vector<float> l (96000, 0.0f), r (96000, 0.0f);
        double phase = 0.0;

        for (int i = 0; i < (int) l.size(); ++i)
        {
            phase += 2.0 * 3.14159265358979323846 * 220.0 / kSr;
            const float s = 0.9f * (float) std::sin (phase);
            l[(size_t) i] = s;
            r[(size_t) i] = s;
        }

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            lim.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos),
                         true, -6.0f, 80.0f);

        const int look = lim.lookaheadSamples();
        const double outPeak = peak (l, look + 4800, (int) l.size());
        check (outPeak <= 0.505, "limiter holds the -6 dB ceiling");
        check (lim.lastGainReductionDb() > 2.0f, "limiter reports gain reduction");
        check (allFinite (l), "limiter output is finite");
    }

    void testDelay()
    {
        vx::DelayModule d;
        d.prepare (kSr, 512);

        vx::DelayParams p;
        p.on = true;
        p.timeMs = 100.0f;
        p.feedback = 0.0f;
        p.tone = 12000.0f;
        p.mix = 1.0f;
        p.duck = 0.0f;
        p.pingPong = false;

        std::vector<float> l (24000, 0.0f), r (24000, 0.0f);
        l[0] = 0.8f;
        r[0] = 0.8f;
        std::vector<float> duck ((size_t) 512, 0.0f);

        for (int pos = 0; pos < (int) l.size(); pos += 512)
        {
            duck.assign ((size_t) 512, 0.0f);
            d.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p, duck.data(), 0.0f);
        }

        int echo = 0;
        double echoPeak = 0.0;

        for (int i = 1000; i < (int) l.size(); ++i)
            if (std::abs (l[(size_t) i]) > echoPeak)
            {
                echoPeak = std::abs (l[(size_t) i]);
                echo = i;
            }

        check (std::abs (echo - 4800) <= 3, "delay repeats at 100 ms");
        check (echoPeak > 0.4, "delay repeat is audible");
    }

    void testReverb()
    {
        vx::ReverbModule rev;
        rev.prepare (kSr, 512);

        vx::VerbParams p;
        p.on = true;
        p.size = 0.7f;
        p.preMs = 20.0f;
        p.damp = 0.4f;
        p.width = 1.0f;
        p.mix = 0.6f;
        p.duck = 0.0f;
        p.hp = 250.0f;
        p.lp = 9000.0f;

        std::vector<float> l (96000, 0.0f), r (96000, 0.0f);
        double phase = 0.0;

        for (int i = 0; i < 12000; ++i)
        {
            phase += 2.0 * 3.14159265358979323846 * 330.0 / kSr;
            l[(size_t) i] = 0.25f * (float) std::sin (phase);
            r[(size_t) i] = l[(size_t) i];
        }

        std::vector<float> duck ((size_t) 512, 0.0f);

        for (int pos = 0; pos < (int) l.size(); pos += 512)
        {
            duck.assign ((size_t) 512, 0.0f);
            rev.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p, duck.data(), 0.5f);
        }

        check (allFinite (l), "reverb output is finite");
        check (rms (l, 24000, 48000) > 0.0008, "reverb tail rings after the source stops");
        check (peak (l, 24000, 48000) < 1.0, "reverb tail stays bounded");
    }

    vx::ChainParams neutralParams()
    {
        vx::ChainParams p;
        p.tune.on = true;
        p.tune.amount = 0.0f;
        p.tune.octaveMix = 0.0f;
        p.eq.on = false;
        p.deess.on = false;
        p.comp1.on = false;
        p.comp2.on = false;
        p.sat.on = false;
        p.motion.on = false;
        p.delay.on = false;
        p.verb.on = false;
        p.limiterOn = false;
        p.width = 1.0f;
        p.levelDb = 0.0f;
        return p;
    }

    void testTune()
    {
        const double wrongHz = 220.0 * std::pow (2.0, 0.3 / 12.0);   // A3, 30 cents sharp

        for (int corrected = 0; corrected < 2; ++corrected)
        {
            vx::VocalChain chain;
            chain.prepare (kSr, 512);

            vx::ChainParams p = neutralParams();
            p.tune.on = true;
            p.tune.amount = corrected ? 1.0f : 0.0f;
            p.tune.speedMs = 2.0f;
            p.tune.scale = vx::ScaleChromatic;
            p.tune.key = 0;

            const int total = (int) (kSr * 3.0);
            std::vector<float> l ((size_t) total, 0.0f), r ((size_t) total, 0.0f);

            double phase = 0.0;

            for (int i = 0; i < total; ++i)
            {
                phase += 2.0 * 3.14159265358979323846 * wrongHz / kSr;
                const float s = 0.28f * (float) std::sin (phase);
                l[(size_t) i] = s;
                r[(size_t) i] = s;
            }

            for (int pos = 0; pos < total; pos += 512)
                chain.process (l.data() + pos, r.data() + pos, std::min (512, total - pos), p);

            const double hz = autocorrelationHz (l, (int) (kSr * 1.0), total);
            const double target = 220.0;
            const double errorCents = 1200.0 * std::log2 (hz / target);

            std::printf ("       tune amount %.0f%%: %.2f Hz (%+.1f cents from A3)\n",
                         corrected ? 100.0 : 0.0, hz, errorCents);

            if (corrected)
                check (std::abs (errorCents) < 12.0, "tune pulls a 30-cent-sharp A3 onto A3");
            else
                check (std::abs (errorCents - 30.0) < 12.0, "tune at 0% leaves the pitch alone");
        }
    }

    void testChainLatencyAndStability()
    {
        {
            vx::VocalChain chain;
            chain.prepare (kSr, 512);
            vx::ChainParams p = neutralParams();
            p.bypass = true;

            std::vector<float> l (12000, 0.0f), r (12000, 0.0f);
            l[100] = 0.7f;
            r[100] = 0.7f;

            for (int pos = 0; pos < (int) l.size(); pos += 512)
                chain.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p);

            const int expected = 100 + chain.latencySamples();
            int found = -1;

            for (int i = 0; i < (int) l.size(); ++i)
                if (std::abs (l[(size_t) i]) > 0.3f)
                {
                    found = i;
                    break;
                }

            std::printf ("       bypass impulse at %d, reported latency %d\n", found, chain.latencySamples());
            check (found == expected, "bypass keeps the exact reported latency");
        }

        {
            vx::VocalChain chain;
            chain.prepare (kSr, 512);
            vx::ChainParams p = neutralParams();
            p.tune.amount = 0.0f;

            std::vector<float> l (96000, 0.0f), r (96000, 0.0f);
            unsigned seed = 7;
            auto rnd = [&]()
            {
                seed = seed * 1664525u + 1013904223u;
                return (float) ((seed >> 9) & 0x7fffff) / (float) 0x7fffff;
            };

            for (int i = 0; i < 192; ++i)
            {
                const float w = 0.5f - 0.5f * (float) std::cos (6.283185307 * i / 192.0);
                l[(size_t) (5000 + i)] = 0.5f * w * (rnd() * 2.0f - 1.0f);
                r[(size_t) (5000 + i)] = l[(size_t) (5000 + i)];
            }

            for (int pos = 0; pos < (int) l.size(); pos += 512)
                chain.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p);

            int found = -1;
            float highest = 0.0f;

            for (int i = 3000; i < (int) l.size(); ++i)
                if (std::abs (l[(size_t) i]) > highest)
                {
                    highest = std::abs (l[(size_t) i]);
                    found = i;
                }

            std::printf ("       processed burst peak at %d, reported latency %d\n",
                         found - 5000, chain.latencySamples());
            check (std::abs (found - 5000 - chain.latencySamples()) <= 64,
                   "processed path matches the reported latency");
        }

        {
            vx::VocalChain chain;
            chain.prepare (kSr, 512);
            vx::ChainParams p;
            p.tune.on = true;
            p.tune.amount = 0.8f;
            p.eq.on = true;
            p.deess.on = true;
            p.comp1.on = true;
            p.comp1.thresholdDb = -24.0f;
            p.comp2.on = true;
            p.comp2.ratio = 8.0f;
            p.comp2.mix = 0.4f;
            p.sat.on = true;
            p.sat.mix = 0.5f;
            p.motion.on = true;
            p.motion.mode = vx::MotionMicroshift;
            p.motion.mix = 0.3f;
            p.delay.on = true;
            p.delay.mix = 0.2f;
            p.verb.on = true;
            p.verb.mix = 0.3f;
            p.verb.shimmer = 0.4f;

            std::vector<float> l (96000, 0.0f), r (96000, 0.0f);
            double phase = 0.0;

            for (int i = 0; i < (int) l.size(); ++i)
            {
                phase += 2.0 * 3.14159265358979323846 * 196.0 / kSr;
                const float s = 0.4f * (float) std::sin (phase);
                l[(size_t) i] = s;
                r[(size_t) i] = s;
            }

            for (int pos = 0; pos < (int) l.size(); pos += 512)
                chain.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p);

            check (allFinite (l), "full chain stays finite with every stage on");
            check (peak (l) <= 1.0001, "full chain stays inside full scale");
            check (chain.latencySamples() >= 2700 && chain.latencySamples() <= 2800,
                   "reported latency includes the tuner and limiter lookahead");
        }
    }

    // ---- regressions for fixed bugs -----------------------------------------

    void testDeEsserIsTransparentWhenIdle()
    {
        // A Butterworth LP + HP split summed back together notches the corner.
        vx::DeEsserModule deess;
        deess.prepare (kSr);

        vx::DeEssParams p;
        p.freq = 6800.0f;
        p.thresholdDb = 0.0f;     // never triggers
        p.amountDb = 12.0f;

        std::vector<float> l (48000, 0.0f), r (48000, 0.0f);
        double a = 0.0;

        for (int i = 0; i < (int) l.size(); ++i)
        {
            a += 2.0 * 3.14159265358979323846 * 6800.0 / kSr;
            l[(size_t) i] = r[(size_t) i] = 0.1f * (float) std::sin (a);
        }

        const double before = goertzel (l, 6800.0, 24000, 46000);

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            deess.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos), p);

        const double after = goertzel (l, 6800.0, 24000, 46000);
        std::printf ("       idle de-esser at its own corner: %.2f dB\n", 20.0 * std::log10 (after / before));
        check (std::abs (20.0 * std::log10 (after / before)) < 0.1, "idle de-esser is flat at its crossover");
    }

    void testLimiterLooksAhead()
    {
        vx::LimiterModule lim;
        lim.prepare (kSr, 512);
        const int look = lim.lookaheadSamples();

        std::vector<float> in (8192, 0.2f), l, r;
        in[4000] = 1.0f;          // one spike well over a -6 dB ceiling
        l = in;
        r = in;

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            lim.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos),
                         true, -6.0f, 80.0f);

        // With real look-ahead the gain is already coming down before the spike
        // leaves the delay line, so the samples just ahead of it are quieter.
        const float before = l[(size_t) (4000 + look - 4)];
        check (before < 0.2f * 0.9f, "limiter gain ramps down ahead of the peak");
        check (std::abs (l[(size_t) (4000 + look)]) <= 0.5012f, "limited spike sits at the ceiling");
        check (std::abs (l[(size_t) (1000 + look)] - 0.2f) < 1.0e-4f, "limiter leaves quiet audio alone");
    }

    void testBypassEngagesCleanly()
    {
        vx::VocalChain chain;
        chain.prepare (kSr, 512);
        vx::ChainParams p = neutralParams();
        p.sat.on = true;
        p.sat.mix = 1.0f;

        const int total = 48000;
        std::vector<float> in ((size_t) total), l, r;
        unsigned seed = 11;

        for (auto& v : in)
        {
            seed = seed * 1664525u + 1013904223u;
            v = 0.3f * ((float) ((seed >> 9) & 0x7fffff) / (float) 0x7fffff * 2.0f - 1.0f);
        }

        l = in;
        r = in;
        const int switchAt = 24064;   // a block boundary

        for (int pos = 0; pos < total; pos += 512)
        {
            p.bypass = pos >= switchAt;
            chain.process (l.data() + pos, r.data() + pos, std::min (512, total - pos), p);
        }

        // Once the crossfade has settled, bypass is the input delayed by the
        // reported latency, even inside the first `latency` samples after the
        // switch (where the old bypass line still held stale audio).
        const int lat = chain.latencySamples();
        double worst = 0.0;

        for (int i = switchAt + 2000; i < switchAt + lat; ++i)
            worst = std::max (worst, (double) std::abs (l[(size_t) i] - in[(size_t) (i - lat)]));

        check (worst < 1.0e-6, "engaging bypass plays the latency-matched input at once");
    }

    void testThrowWithDelayOff()
    {
        vx::DelayModule delay;
        delay.prepare (kSr, 512);

        vx::DelayParams p;
        p.on = false;
        p.timeMs = 100.0f;
        p.feedback = 0.0f;
        p.duck = 0.0f;

        std::vector<float> l (24000, 0.0f), r (24000, 0.0f);
        l[1000] = r[1000] = 0.8f;

        for (int pos = 0; pos < (int) l.size(); pos += 512)
            delay.process (l.data() + pos, r.data() + pos, std::min (512, (int) l.size() - pos),
                           p, nullptr, 1.0f);

        check (peak (l, 1000 + 4800 - 4, 1000 + 4800 + 4) > 0.1, "THROW opens the delay even when it is off");
    }
}

int main()
{
    std::printf ("voxline DSP tests\n\n");

    testBiquads();
    testCompressor();
    testDeEsser();
    testLimiter();
    testDelay();
    testReverb();
    testTune();
    testChainLatencyAndStability();
    testDeEsserIsTransparentWhenIdle();
    testLimiterLooksAhead();
    testBypassEngagesCleanly();
    testThrowWithDelayOff();

    std::printf ("\n%s\n", failures == 0 ? "all DSP checks passed" : "DSP CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}

#pragma once

// Small real-time primitives: envelopes, smoothing, interpolated delay lines,
// LFOs, allpass diffusion and a granular detune/pitch head.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace vx
{

struct EnvelopeFollower
{
    float env = 0.0f;
    float attack = 0.0f;   // one-pole coefficient
    float release = 0.0f;

    void set (double sr, float attackMs, float releaseMs) noexcept
    {
        const double fs = std::max (1.0, sr);
        attack  = (float) std::exp (-1.0 / (fs * std::max (0.01f, attackMs) * 0.001));
        release = (float) std::exp (-1.0 / (fs * std::max (0.01f, releaseMs) * 0.001));
    }

    float process (float x) noexcept
    {
        const float a = std::abs (x);
        const float c = a > env ? attack : release;
        env = a + c * (env - a);

        if (env < 1.0e-20f)
            env = 0.0f;

        return env;
    }

    void reset() noexcept { env = 0.0f; }
};

struct OnePole
{
    float z = 0.0f;
    float a = 0.0f;   // lowpass coefficient

    void setLowPass (double sr, double hz) noexcept
    {
        const double fs = std::max (1.0, sr);
        a = (float) std::exp (-2.0 * 3.14159265358979323846 * std::max (1.0, hz) / fs);
    }

    inline float lowPass (float x) noexcept
    {
        z = x + a * (z - x);
        if (std::abs (z) < 1.0e-20f) z = 0.0f;
        return z;
    }

    inline float highPass (float x) noexcept { return x - lowPass (x); }
    void reset() noexcept { z = 0.0f; }
};

struct Smoothed
{
    float current = 0.0f;
    float target = 0.0f;
    float coef = 0.0f;

    void setTime (double sr, float ms) noexcept
    {
        const double fs = std::max (1.0, sr);
        coef = (float) std::exp (-1.0 / (fs * std::max (0.05f, ms) * 0.001));
    }

    void snap (float v) noexcept { current = target = v; }

    inline float next() noexcept
    {
        current = target + coef * (current - target);
        return current;
    }

    inline float value() const noexcept { return current; }
};

struct DelayLine
{
    std::vector<float> buf;
    int size = 0;
    int write = 0;

    void prepare (int maxDelaySamples, float initial = 0.0f)
    {
        size = std::max (8, maxDelaySamples + 8);
        buf.assign ((size_t) size, initial);
        write = 0;
    }

    void reset (float initial = 0.0f)
    {
        std::fill (buf.begin(), buf.end(), initial);
        write = 0;
    }

    inline void push (float x) noexcept
    {
        buf[(size_t) write] = x;
        if (++write >= size)
            write = 0;
    }

    /** Delay in samples, read with linear interpolation. Call before push for
        feedback paths (delay 1 = the previous sample). */
    inline float read (float delay) const noexcept
    {
        float rp = (float) write - delay;

        while (rp < 0.0f)
            rp += (float) size;

        while (rp >= (float) size)
            rp -= (float) size;

        const int i0 = (int) rp;
        const float f = rp - (float) i0;
        int i1 = i0 + 1;
        if (i1 >= size) i1 = 0;

        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }
};

struct Lfo
{
    float phase = 0.0f;

    void reset (float p = 0.0f) noexcept { phase = p; }

    inline float tick (float rateHz, double sr) noexcept
    {
        phase += (float) (rateHz / std::max (1.0, sr));
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f)  phase += 1.0f;
        return phase;
    }

    static inline float sine (float phase) noexcept
    {
        return std::sin (2.0f * 3.14159265358979323846f * phase);
    }
};

/** Schroeder allpass for input diffusion in the reverb. */
struct Allpass
{
    std::vector<float> buf;
    int size = 0, write = 0;
    float g = 0.5f;

    void prepare (int delaySamples, float gain)
    {
        size = std::max (4, delaySamples + 2);
        buf.assign ((size_t) size, 0.0f);
        write = 0;
        g = gain;
    }

    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); write = 0; }

    inline float process (float x) noexcept
    {
        const float delayed = buf[(size_t) write];
        const float y = delayed - g * x;
        buf[(size_t) write] = x + g * y;
        if (++write >= size) write = 0;
        return y;
    }
};

/** Granular pitch head: plays the input back at 2^(cents/1200) with two
    windowed read heads. Used for the microshift halo and the reverb shimmer;
    small shifts stay smooth and an octave up is deliberately soft and washy. */
class DetuneHead
{
public:
    void prepare (double sampleRate, float grainMs, float baseDelayMs)
    {
        sr = std::max (8000.0, sampleRate);
        grain = (float) (sr * grainMs * 0.001);
        base = (float) (sr * baseDelayMs * 0.001);
        const int maxDelay = (int) (base + grain + 8.0f);
        buf.assign ((size_t) std::max (16, maxDelay), 0.0f);
        size = (int) buf.size();
        write = 0;
        behindA = base + grain * 0.5f;
        behindB = base + grain;
        primed = true;
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        write = 0;
        behindA = base + grain * 0.5f;
        behindB = base + grain;
    }

    inline float process (float x, float cents) noexcept
    {
        if (size <= 0 || grain < 4.0f)
            return x;

        buf[(size_t) write] = x;

        const float inc = std::pow (2.0f, cents / 1200.0f);
        const float step = 1.0f - inc;

        behindA += step;
        behindB += step;

        wrap (behindA);
        wrap (behindB);

        const float dA = (behindA - base) / grain;
        const float dB = (behindB - base) / grain;
        const float wA = 0.5f - 0.5f * std::cos (2.0f * 3.14159265358979323846f * clampF (dA));
        const float wB = 0.5f - 0.5f * std::cos (2.0f * 3.14159265358979323846f * clampF (dB));
        const float sum = wA + wB;

        const float yA = readAt (behindA);
        const float yB = readAt (behindB);
        const float y = (yA * wA + yB * wB) / std::max (0.001f, sum);

        if (++write >= size)
            write = 0;

        if (y > 8.0f || y < -8.0f)
            return 0.0f;

        return y;
    }

private:
    static inline float clampF (float x) noexcept { return std::max (0.0f, std::min (1.0f, x)); }

    inline void wrap (float& behind) const noexcept
    {
        while (behind < base)
            behind += grain;

        while (behind >= base + grain)
            behind -= grain;
    }

    inline float readAt (float behind) const noexcept
    {
        float rp = (float) write - behind;

        while (rp < 0.0f)
            rp += (float) size;

        while (rp >= (float) size)
            rp -= (float) size;

        const int i0 = (int) rp;
        const float f = rp - (float) i0;
        int i1 = i0 + 1;
        if (i1 >= size) i1 = 0;

        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }

    std::vector<float> buf;
    int size = 0, write = 0;
    float grain = 0.0f, base = 0.0f;
    float behindA = 0.0f, behindB = 0.0f;
    double sr = 48000.0;
    bool primed = false;
};

} // namespace vx

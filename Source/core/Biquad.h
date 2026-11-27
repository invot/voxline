#pragma once

// RBJ-cookbook biquads with a per-channel state pair, plus the magnitude
// response the editor uses to draw the EQ curve.

#include <cmath>
#include <complex>

namespace vx
{

struct BiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

    static BiquadCoeffs normalise (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        if (std::abs (a0) < 1.0e-12)
            a0 = 1.0;

        BiquadCoeffs c;
        c.b0 = (float) (b0 / a0);
        c.b1 = (float) (b1 / a0);
        c.b2 = (float) (b2 / a0);
        c.a1 = (float) (a1 / a0);
        c.a2 = (float) (a2 / a0);
        return c;
    }
};

inline float safeFreq (double sr, double f) noexcept
{
    return (float) std::max (10.0, std::min (f, sr * 0.46));
}

inline BiquadCoeffs makeLowPass (double sr, double f, double q = 0.70710678)
{
    const double w = 2.0 * 3.14159265358979323846 * safeFreq (sr, f) / sr;
    const double cw = std::cos (w), sw = std::sin (w);
    const double alpha = sw / (2.0 * std::max (0.05, q));
    const double a0 = 1.0 + alpha;
    return BiquadCoeffs::normalise ((1.0 - cw) * 0.5, 1.0 - cw, (1.0 - cw) * 0.5,
                                    a0, -2.0 * cw, 1.0 - alpha);
}

inline BiquadCoeffs makeHighPass (double sr, double f, double q = 0.70710678)
{
    const double w = 2.0 * 3.14159265358979323846 * safeFreq (sr, f) / sr;
    const double cw = std::cos (w), sw = std::sin (w);
    const double alpha = sw / (2.0 * std::max (0.05, q));
    const double a0 = 1.0 + alpha;
    return BiquadCoeffs::normalise ((1.0 + cw) * 0.5, -(1.0 + cw), (1.0 + cw) * 0.5,
                                    a0, -2.0 * cw, 1.0 - alpha);
}

inline BiquadCoeffs makePeaking (double sr, double f, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w = 2.0 * 3.14159265358979323846 * safeFreq (sr, f) / sr;
    const double cw = std::cos (w), sw = std::sin (w);
    const double alpha = sw / (2.0 * std::max (0.05, q));
    const double a0 = 1.0 + alpha / A;
    return BiquadCoeffs::normalise (1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
                                    a0, -2.0 * cw, 1.0 - alpha / A);
}

inline BiquadCoeffs makeLowShelf (double sr, double f, double gainDb, double slope = 1.0)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w = 2.0 * 3.14159265358979323846 * safeFreq (sr, f) / sr;
    const double cw = std::cos (w), sw = std::sin (w);
    const double alpha = sw * 0.5 * std::sqrt ((A + 1.0 / A) * (1.0 / std::max (0.1, slope) - 1.0) + 2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;

    const double a0 = (A + 1.0) + (A - 1.0) * cw + beta;
    return BiquadCoeffs::normalise (
        A * ((A + 1.0) - (A - 1.0) * cw + beta),
        2.0 * A * ((A - 1.0) - (A + 1.0) * cw),
        A * ((A + 1.0) - (A - 1.0) * cw - beta),
        a0,
        -2.0 * ((A - 1.0) + (A + 1.0) * cw),
        (A + 1.0) + (A - 1.0) * cw - beta);
}

inline BiquadCoeffs makeHighShelf (double sr, double f, double gainDb, double slope = 1.0)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w = 2.0 * 3.14159265358979323846 * safeFreq (sr, f) / sr;
    const double cw = std::cos (w), sw = std::sin (w);
    const double alpha = sw * 0.5 * std::sqrt ((A + 1.0 / A) * (1.0 / std::max (0.1, slope) - 1.0) + 2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;

    const double a0 = (A + 1.0) - (A - 1.0) * cw + beta;
    return BiquadCoeffs::normalise (
        A * ((A + 1.0) + (A - 1.0) * cw + beta),
        -2.0 * A * ((A - 1.0) + (A + 1.0) * cw),
        A * ((A + 1.0) + (A - 1.0) * cw - beta),
        a0,
        2.0 * ((A - 1.0) - (A + 1.0) * cw),
        (A + 1.0) - (A - 1.0) * cw - beta);
}

struct Biquad
{
    BiquadCoeffs c;
    float z1 = 0.0f, z2 = 0.0f;

    inline float process (float x) noexcept
    {
        const float y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;

        if (std::abs (z1) < 1.0e-20f) z1 = 0.0f;
        if (std::abs (z2) < 1.0e-20f) z2 = 0.0f;

        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

    double magnitudeDb (double freq, double sr) const noexcept
    {
        const double w = 2.0 * 3.14159265358979323846 * freq / sr;
        const std::complex<double> z1c (std::cos (-w), std::sin (-w));
        const std::complex<double> z2c = z1c * z1c;
        const std::complex<double> num = (double) c.b0 + (double) c.b1 * z1c + (double) c.b2 * z2c;
        const std::complex<double> den = 1.0 + (double) c.a1 * z1c + (double) c.a2 * z2c;
        const double mag = std::abs (num) / std::max (1.0e-12, std::abs (den));
        return 20.0 * std::log10 (std::max (1.0e-6, mag));
    }
};

struct StereoBiquad
{
    Biquad l, r;

    void set (const BiquadCoeffs& c) noexcept { l.c = c; r.c = c; }
    void reset() noexcept { l.reset(); r.reset(); }
    inline float processL (float x) noexcept { return l.process (x); }
    inline float processR (float x) noexcept { return r.process (x); }
    void process (float& a, float& b) noexcept { a = l.process (a); b = r.process (b); }
    double magnitudeDb (double f, double sr) const noexcept { return l.magnitudeDb (f, sr); }
};

} // namespace vx

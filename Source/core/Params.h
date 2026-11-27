#pragma once

// Plain parameter structs shared by the DSP core, the host parameter layer and
// the UI response graphs. No JUCE here so the chain can be tested from a
// console binary.

#include <algorithm>
#include <cmath>

namespace vx
{

enum SatMode
{
    SatTube = 0,
    SatTape,
    SatFuzz,
    SatCrush
};

enum MotionMode
{
    MotionChorus = 0,
    MotionFlanger,
    MotionDoubler,
    MotionMicroshift
};

enum DelaySync
{
    SyncFree = 0,
    Sync1_32,
    Sync1_16,
    Sync8T,
    Sync16D,
    Sync8,
    Sync8D,
    Sync4,
    Sync4D,
    Sync2
};

enum Scale
{
    ScaleChromatic = 0,
    ScaleMajor,
    ScaleMinor,
    ScaleHarmonicMinor,
    ScaleMajorPent,
    ScaleMinorPent,
    ScaleDorian,
    ScaleMixolydian
};

struct TuneParams
{
    bool on = true;
    float amount = 0.35f;     // 0..1: how much of the way to the target note
    float speedMs = 35.0f;    // retune glide
    int key = 0;              // 0 = C
    int scale = ScaleChromatic;
    float formant = 0.0f;     // semitones
    float octaveMix = 0.0f;   // 0..1: -12 st layer tucked underneath
};

struct EqParams
{
    bool on = true;
    float hpf = 85.0f;
    float lp = 20000.0f;
    float lowFreq = 140.0f;
    float lowGain = 0.0f;
    float bodyFreq = 300.0f;
    float bodyGain = 0.0f;
    float bodyQ = 0.8f;
    float presFreq = 3200.0f;
    float presGain = 0.0f;
    float presQ = 0.9f;
    float airFreq = 11000.0f;
    float airGain = 0.0f;
};

struct DeEssParams
{
    bool on = true;
    float freq = 6800.0f;
    float thresholdDb = -30.0f;
    float amountDb = 6.0f;
};

struct CompParams
{
    bool on = true;
    float thresholdDb = -22.0f;
    float ratio = 4.0f;
    float attackMs = 3.0f;
    float releaseMs = 80.0f;
    float mix = 1.0f;
    float makeupDb = 0.0f;
};

struct SatParams
{
    bool on = true;
    int mode = SatTube;
    float drive = 20.0f;      // 0..100
    float tone = 0.0f;        // -1 dark .. +1 bright
    float mix = 0.12f;        // parallel blend
    int bits = 8;
};

struct MotionParams
{
    bool on = false;
    int mode = MotionChorus;
    float rate = 0.6f;
    float depth = 0.5f;
    float feedback = 0.0f;
    float width = 0.7f;
    float mix = 0.15f;
};

struct DelayParams
{
    bool on = false;
    float timeMs = 300.0f;
    int sync = SyncFree;
    float feedback = 0.25f;
    float tone = 5000.0f;
    float mix = 0.12f;
    bool pingPong = true;
    float duck = 0.35f;
    float bpm = 120.0f;
};

struct VerbParams
{
    bool on = true;
    float size = 0.35f;
    float preMs = 20.0f;
    float damp = 0.5f;
    float width = 0.85f;
    float mix = 0.12f;
    float duck = 0.4f;
    float hp = 300.0f;
    float lp = 9000.0f;
    float shimmer = 0.0f;
};

struct ChainParams
{
    bool bypass = false;

    // The six shaping macros plus output trim.
    float press = 0.5f;       // 0..1
    float tone = 0.5f;        // 0..1
    float grit = 0.15f;       // 0..1
    float width = 1.0f;       // 0..2
    float space = 0.35f;      // 0..1
    float levelDb = 0.0f;

    TuneParams tune;
    EqParams eq;
    DeEssParams deess;
    CompParams comp1;
    CompParams comp2;
    SatParams sat;
    MotionParams motion;
    DelayParams delay;
    VerbParams verb;

    bool limiterOn = true;
    float ceilingDb = -0.3f;
    float limiterReleaseMs = 80.0f;

    float bpm = 120.0f;
    float throwAmount = 0.0f;   // momentary delay throw, not automated

    double sampleRate = 48000.0;
};

inline float clamp01 (float x) noexcept { return std::max (0.0f, std::min (1.0f, x)); }
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-7f)); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

/** Semitone scale membership, shared by the tuner and the UI. */
inline bool scaleHasNote (int scale, int key, int note) noexcept
{
    static const int major[]       = { 0, 2, 4, 5, 7, 9, 11 };
    static const int minor[]       = { 0, 2, 3, 5, 7, 8, 10 };
    static const int harmMinor[]   = { 0, 2, 3, 5, 7, 8, 11 };
    static const int majorPent[]   = { 0, 2, 4, 7, 9 };
    static const int minorPent[]   = { 0, 3, 5, 7, 10 };
    static const int dorian[]      = { 0, 2, 3, 5, 7, 9, 10 };
    static const int mixolydian[]  = { 0, 2, 4, 5, 7, 9, 10 };

    if (scale == ScaleChromatic)
        return true;

    const int* table = major;
    int count = 7;

    switch (scale)
    {
        case ScaleMinor:         table = minor;      break;
        case ScaleHarmonicMinor: table = harmMinor;  break;
        case ScaleMajorPent:     table = majorPent;  count = 5; break;
        case ScaleMinorPent:     table = minorPent;  count = 5; break;
        case ScaleDorian:        table = dorian;     break;
        case ScaleMixolydian:    table = mixolydian; break;
        default: break;
    }

    int pc = (note - key) % 12;
    if (pc < 0)
        pc += 12;

    for (int i = 0; i < count; ++i)
        if (table[i] == pc)
            return true;

    return false;
}

/** Nearest note allowed by the key/scale, searched within a whole tone. */
inline int nearestScaleNote (int scale, int key, int midiNote) noexcept
{
    if (scaleHasNote (scale, key, midiNote))
        return midiNote;

    for (int d = 1; d <= 2; ++d)
    {
        if (scaleHasNote (scale, key, midiNote - d)) return midiNote - d;
        if (scaleHasNote (scale, key, midiNote + d)) return midiNote + d;
    }

    return midiNote;
}

/** Delay sync choice to a length in beats (a quarter note is one beat). */
inline float syncBeats (int choice) noexcept
{
    switch (choice)
    {
        case Sync1_32: return 0.125f;
        case Sync1_16: return 0.25f;
        case Sync8T:   return 1.0f / 3.0f;
        case Sync16D:  return 0.375f;
        case Sync8:    return 0.5f;
        case Sync8D:   return 0.75f;
        case Sync4:    return 1.0f;
        case Sync4D:   return 1.5f;
        case Sync2:    return 2.0f;
        default:       return 0.0f;
    }
}

} // namespace vx

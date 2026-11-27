#pragma once

#include "Biquad.h"
#include "DspUtil.h"
#include "Params.h"

#include <memory>
#include <vector>

namespace vx
{

/** YIN pitch tracker, decimated to roughly 24 kHz. Monophonic, which is what a
    lead vocal is. */
class YinDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    void push (float x);
    bool ready() const noexcept { return filled >= frame && sinceHop >= hop; }
    void analyse();

    float frequency() const noexcept { return lastHz; }
    float confidence() const noexcept { return lastConf; }

private:
    double sr = 48000.0;
    double internalSr = 24000.0;
    int decim = 2;
    int decimCounter = 0;
    int frame = 1024;
    int hop = 512;
    int sinceHop = 0;
    int filled = 0;
    int write = 0;

    Biquad antiAlias;
    std::vector<float> ring;
    std::vector<float> ordered;
    std::vector<float> diff;
    std::vector<float> cmnd;
    float lastHz = 0.0f;
    float lastConf = 0.0f;
};

/** Pitch correction with the Signalsmith Stretch library (MIT).

    The library is used as a time-stretcher, not as a pitch-shifter: its peak
    map is tuned for semitone-sized moves and quantises cent-sized ones. A
    cubic resampler after the stretcher makes the ratio exact, so a 30-cent
    correction lands on the note, not 10 cents short. The shifter latency is
    fixed and reported to the host. */
class TuneModule
{
public:
    TuneModule();
    ~TuneModule();

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (float* l, float* r, int n, const TuneParams&);

    int latencySamples() const noexcept { return latency; }
    float shiftCents() const noexcept { return smoothedSemis * 100.0f; }
    float detectedHz() const noexcept { return lastHz; }

private:
    float targetForPitch (const TuneParams&) const;

    double sr = 48000.0;
    int latency = 2306;
    float smoothedSemis = 0.0f;
    float lastHz = 0.0f;
    bool octaveActive = false;

    DetuneHead octL, octR;
    YinDetector yin;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace vx

#pragma once

#include "Theme.h"
#include "../core/Modules.h"
#include "../core/Params.h"

namespace vx::ui
{

/** Live EQ response: every band plus the Tone tilt, as the DSP computes it. */
class EqGraph : public juce::Component
{
public:
    EqGraph();

    void setParams (const ChainParams&);
    void paint (juce::Graphics&) override;

private:
    EqModule eq;
    ChainParams params;
};

/** Reverb decay envelope with the delay taps on top. */
class SpaceGraph : public juce::Component
{
public:
    SpaceGraph();

    void setParams (const ChainParams&);
    void paint (juce::Graphics&) override;

    /** The reverb's real RT60 for a Size value, matching ReverbModule. */
    static float rt60Seconds (float size);

private:
    ChainParams params;
};

/** Gain reduction of the two compressors and the limiter. */
class DynamicsGraph : public juce::Component
{
public:
    DynamicsGraph();

    void setValues (const ChainParams&, float fastGrDb, float smoothGrDb, float limiterGrDb);
    void paint (juce::Graphics&) override;

private:
    ChainParams params;
    float shown[3] {};
};

} // namespace vx::ui

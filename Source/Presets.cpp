#include "Presets.h"

#include "core/Params.h"

namespace vx
{

namespace
{
    using P = Param;
    using Values = std::vector<std::pair<Param, float>>;

    // Every factory preset is built from one call per module, and each call
    // sets every parameter of that module. A preset cannot leave a stage to
    // chance: reverb, limiter, both compressors, EQ, de-esser, pitch, drive,
    // stereo width and chorus/motion are all stated explicitly.
    //
    // Key and scale are deliberately not part of a preset: they belong to the
    // song, so they survive preset changes.

    struct Builder
    {
        Values v;

        Builder& add (Param p, float value) { v.emplace_back (p, value); return *this; }

        /** press, tone, tune amount, grit, width (0..200), space, output level dB */
        Builder& macros (float press, float tone, float tune, float grit, float width, float space, float level)
        {
            return add (P::Press, press).add (P::Tone, tone).add (P::Tune, tune).add (P::Grit, grit)
                  .add (P::Width, width).add (P::Space, space).add (P::Level, level);
        }

        /** pitch correction: retune speed ms, formant st, sub-octave % */
        Builder& pitch (bool on, float speedMs, float formant, float subOctave)
        {
            return add (P::TuneOn, on ? 1.0f : 0.0f).add (P::TuneSpeed, speedMs)
                  .add (P::Formant, formant).add (P::OctaveMix, subOctave);
        }

        /** EQ: low cut, top roll-off, then freq/gain(/Q) for low, body, presence, air */
        Builder& eq (bool on, float lowCut, float topCut,
                     float lowF, float lowG,
                     float bodyF, float bodyG, float bodyQ,
                     float presF, float presG, float presQ,
                     float airF, float airG)
        {
            return add (P::EqOn, on ? 1.0f : 0.0f).add (P::Hpf, lowCut).add (P::LpFreq, topCut)
                  .add (P::LowFreq, lowF).add (P::LowGain, lowG)
                  .add (P::BodyFreq, bodyF).add (P::BodyGain, bodyG).add (P::BodyQ, bodyQ)
                  .add (P::PresFreq, presF).add (P::PresGain, presG).add (P::PresQ, presQ)
                  .add (P::AirFreq, airF).add (P::AirGain, airG);
        }

        /** de-esser: sibilance Hz, threshold dB, max reduction dB */
        Builder& deess (bool on, float freq, float threshold, float reduction)
        {
            return add (P::DeessOn, on ? 1.0f : 0.0f).add (P::DeessFreq, freq)
                  .add (P::DeessThresh, threshold).add (P::DeessAmount, reduction);
        }

        /** fast compressor: threshold, ratio, attack ms, release ms, mix %, makeup dB */
        Builder& fastComp (bool on, float thr, float ratio, float att, float rel, float mix, float makeup)
        {
            return add (P::Comp1On, on ? 1.0f : 0.0f).add (P::Comp1Thresh, thr).add (P::Comp1Ratio, ratio)
                  .add (P::Comp1Attack, att).add (P::Comp1Release, rel)
                  .add (P::Comp1Mix, mix).add (P::Comp1Makeup, makeup);
        }

        /** smooth compressor: threshold, ratio, attack ms, release ms, mix %, makeup dB */
        Builder& smoothComp (bool on, float thr, float ratio, float att, float rel, float mix, float makeup)
        {
            return add (P::Comp2On, on ? 1.0f : 0.0f).add (P::Comp2Thresh, thr).add (P::Comp2Ratio, ratio)
                  .add (P::Comp2Attack, att).add (P::Comp2Release, rel)
                  .add (P::Comp2Mix, mix).add (P::Comp2Makeup, makeup);
        }

        /** distortion: character, drive %, tone %, mix %, crush bits */
        Builder& drive (bool on, SatMode mode, float amount, float tone, float mix, float bits)
        {
            return add (P::SatOn, on ? 1.0f : 0.0f).add (P::SatMode, (float) mode).add (P::SatDrive, amount)
                  .add (P::SatTone, tone).add (P::SatMix, mix).add (P::SatBits, bits);
        }

        /** chorus / motion: mode, rate Hz, depth %, feedback %, width %, mix % */
        Builder& motion (bool on, MotionMode mode, float rate, float depth, float feedback, float width, float mix)
        {
            return add (P::MotionOn, on ? 1.0f : 0.0f).add (P::MotionMode, (float) mode)
                  .add (P::MotionRate, rate).add (P::MotionDepth, depth).add (P::MotionFeedback, feedback)
                  .add (P::MotionWidth, width).add (P::MotionMix, mix);
        }

        /** delay: free time ms, sync, feedback %, tone Hz, send %, ping-pong, ducking % */
        Builder& delay (bool on, float timeMs, DelaySync sync, float feedback, float tone,
                        float send, bool pingPong, float duck)
        {
            return add (P::DelayOn, on ? 1.0f : 0.0f).add (P::DelayTime, timeMs).add (P::DelaySync, (float) sync)
                  .add (P::DelayFeedback, feedback).add (P::DelayTone, tone).add (P::DelayMix, send)
                  .add (P::DelayPingPong, pingPong ? 1.0f : 0.0f).add (P::DelayDuck, duck);
        }

        /** reverb: size, pre-delay ms, damping, width, send, ducking, return low cut, return top, shimmer */
        Builder& reverb (bool on, float size, float preMs, float damp, float width, float send,
                         float duck, float lowCut, float top, float shimmer)
        {
            return add (P::VerbOn, on ? 1.0f : 0.0f).add (P::VerbSize, size).add (P::VerbPre, preMs)
                  .add (P::VerbDamp, damp).add (P::VerbWidth, width).add (P::VerbMix, send)
                  .add (P::VerbDuck, duck).add (P::VerbHP, lowCut).add (P::VerbLP, top)
                  .add (P::VerbShimmer, shimmer);
        }

        /** limiter: ceiling dB, release ms */
        Builder& limiter (bool on, float ceilingDb, float releaseMs)
        {
            return add (P::LimiterOn, on ? 1.0f : 0.0f).add (P::Ceiling, ceilingDb)
                  .add (P::LimiterRelease, releaseMs);
        }
    };

    constexpr bool On = true, Off = false;
}

bool isPresetParam (Param p)
{
    return p != Param::Bypass && p != Param::TuneKey && p != Param::TuneScale;
}

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets =
    {
        { "Close / Intimate", "dry  |  forward  |  detailed",
          Builder()
            .macros     (62, 55, 55, 18, 95, 30, 0)
            .pitch      (On, 25, 0, 0)
            .eq         (On, 85, 20000,  150, 2.5f,  300, 1.0f, 0.8f,  3200, 1.5f, 0.9f,  11000, 3.0f)
            .deess      (On, 6800, -30, 7)
            .fastComp   (On, -24, 4, 2, 70, 100, 2)
            .smoothComp (On, -18, 3, 25, 320, 100, 1.5f)
            .drive      (On, SatTape, 18, 0, 12, 8)
            .motion     (Off, MotionChorus, 0.5f, 35, 0, 70, 12)
            .delay      (On, 75, SyncFree, 0, 3000, 12, false, 20)
            .reverb     (On, 25, 8, 55, 80, 10, 50, 300, 8000, 0)
            .limiter    (On, -0.3f, 80).v },

        { "Grand / Stadium", "wide  |  long tail  |  ducked",
          Builder()
            .macros     (70, 58, 35, 30, 130, 62, -1)
            .pitch      (On, 40, 0, 0)
            .eq         (On, 70, 20000,  120, 1.0f,  300, 0.0f, 0.8f,  3000, 2.5f, 0.9f,  11000, 1.5f)
            .deess      (On, 7000, -30, 7)
            .fastComp   (On, -16, 5, 5, 100, 100, 1)
            .smoothComp (On, -10, 12, 2, 150, 35, 0)
            .drive      (On, SatTube, 35, 0, 25, 8)
            .motion     (On, MotionMicroshift, 0.4f, 60, 0, 70, 10)
            .delay      (On, 300, Sync8D, 35, 5000, 18, true, 55)
            .reverb     (On, 65, 55, 45, 90, 30, 55, 350, 7000, 0)
            .limiter    (On, -0.8f, 120).v },

        { "Chorus Stack", "layered doubles  |  wide  |  glued",
          Builder()
            .macros     (65, 60, 60, 20, 140, 45, -1.5f)
            .pitch      (On, 30, 0, 0)
            .eq         (On, 150, 20000,  200, -2.0f,  300, -2.0f, 0.8f,  3200, -1.5f, 0.9f,  11000, 2.0f)
            .deess      (On, 7200, -32, 9)
            .fastComp   (On, -24, 6, 1, 60, 100, 1.5f)
            .smoothComp (On, -20, 4, 20, 250, 100, 1)
            .drive      (On, SatTape, 15, 0, 12, 8)
            .motion     (On, MotionDoubler, 0.3f, 50, 0, 85, 28)
            .delay      (On, 300, Sync8D, 30, 5000, 15, true, 50)
            .reverb     (On, 45, 30, 50, 90, 22, 45, 300, 8500, 0)
            .limiter    (On, -0.5f, 80).v },

        { "Polished Pop", "hard tuned  |  bright  |  throw",
          Builder()
            .macros     (68, 65, 85, 12, 115, 38, -0.5f)
            .pitch      (On, 8, 0, 12)
            .eq         (On, 90, 20000,  140, 1.0f,  300, -1.5f, 0.8f,  3500, 2.5f, 0.9f,  12000, 4.0f)
            .deess      (On, 7300, -32, 9)
            .fastComp   (On, -26, 3.5f, 2, 60, 100, 2)
            .smoothComp (On, -22, 3, 15, 200, 100, 1)
            .drive      (On, SatTube, 15, 0, 10, 8)
            .motion     (On, MotionMicroshift, 0.4f, 60, 0, 70, 16)
            .delay      (On, 300, Sync8, 25, 6000, 14, true, 60)
            .reverb     (On, 45, 25, 45, 85, 18, 50, 300, 9000, 0)
            .limiter    (On, -0.3f, 80).v },

        { "Ethereal", "washed  |  shimmering  |  soft",
          Builder()
            .macros     (55, 32, 28, 15, 135, 88, -2)
            .pitch      (On, 80, -1, 0)
            .eq         (On, 100, 8000,  140, 0.0f,  300, 0.0f, 0.8f,  4000, -1.0f, 0.9f,  10000, -2.0f)
            .deess      (On, 6500, -28, 5)
            .fastComp   (On, -22, 3, 8, 120, 100, 1)
            .smoothComp (On, -20, 2.5f, 40, 400, 100, 1)
            .drive      (On, SatTape, 20, 0, 15, 8)
            .motion     (On, MotionMicroshift, 0.25f, 70, 0, 95, 30)
            .delay      (On, 300, Sync8D, 45, 4000, 22, true, 30)
            .reverb     (On, 88, 0, 35, 100, 50, 20, 250, 6000, 45)
            .limiter    (On, -1.0f, 80).v },

        { "Robotic", "hard tuned  |  crushed  |  metallic",
          Builder()
            .macros     (75, 52, 100, 45, 80, 22, -1)
            .pitch      (On, 1, 3, 0)
            .eq         (On, 120, 12000,  120, -2.0f,  300, 1.0f, 0.8f,  3000, 2.0f, 0.9f,  11000, 1.0f)
            .deess      (On, 7000, -28, 6)
            .fastComp   (On, -30, 10, 1, 40, 100, 2)
            .smoothComp (On, -28, 8, 5, 120, 100, 1.5f)
            .drive      (On, SatCrush, 30, 0, 25, 6)
            .motion     (On, MotionChorus, 3.5f, 30, 0, 50, 25)
            .delay      (On, 300, Sync1_16, 30, 6000, 12, true, 30)
            .reverb     (On, 20, 5, 30, 60, 8, 40, 400, 9000, 0)
            .limiter    (On, -0.5f, 30).v },

        { "Grunge", "parallel grit  |  fast limit  |  raw",
          Builder()
            .macros     (80, 50, 10, 70, 105, 25, -2)
            .pitch      (On, 50, 0, 0)
            .eq         (On, 110, 16000,  120, -2.0f,  250, 2.0f, 0.8f,  2800, 3.0f, 0.9f,  10000, -1.0f)
            .deess      (On, 6500, -30, 6)
            .fastComp   (On, -28, 8, 1, 50, 100, 2)
            .smoothComp (On, -24, 8, 5, 120, 100, 2)
            .drive      (On, SatFuzz, 65, 10, 45, 8)
            .motion     (On, MotionChorus, 0.5f, 40, 0, 60, 6)
            .delay      (On, 90, SyncFree, 15, 3500, 8, false, 35)
            .reverb     (On, 30, 15, 50, 80, 12, 30, 300, 7000, 0)
            .limiter    (On, -1.0f, 40).v },

        // Band-limited to roughly 400 Hz - 3 kHz with a honky mid bump. The old
        // table asked for a 500 Hz low cut and a 1.2 kHz body band, both past
        // their parameter ranges, so they were silently clamped.
        { "Telephone", "band-passed  |  mono  |  lo-fi",
          Builder()
            .macros     (85, 40, 15, 55, 0, 0, -1.5f)
            .pitch      (On, 40, 0, 0)
            .eq         (On, 400, 3000,  140, 0.0f,  800, 4.0f, 1.5f,  1800, 3.0f, 1.2f,  8000, -6.0f)
            .deess      (Off, 6800, -30, 6)
            .fastComp   (On, -34, 8, 1, 50, 100, 3)
            .smoothComp (On, -30, 6, 10, 150, 100, 2)
            .drive      (On, SatFuzz, 45, 0, 40, 8)
            .motion     (Off, MotionChorus, 0.6f, 50, 0, 70, 15)
            .delay      (Off, 120, SyncFree, 20, 3000, 10, false, 35)
            .reverb     (Off, 20, 10, 60, 50, 8, 40, 500, 4000, 0)
            .limiter    (On, -1.0f, 80).v },

        { "Rap / Front", "tight  |  dry  |  present",
          Builder()
            .macros     (72, 55, 40, 20, 100, 22, 0)
            .pitch      (On, 20, 0, 0)
            .eq         (On, 95, 20000,  130, 1.0f,  300, 0.0f, 0.8f,  3500, 2.0f, 0.9f,  11000, 2.0f)
            .deess      (On, 7000, -30, 7)
            .fastComp   (On, -26, 5, 1, 60, 100, 2)
            .smoothComp (On, -22, 4, 15, 220, 100, 1)
            .drive      (On, SatTube, 15, 0, 8, 8)
            .motion     (On, MotionMicroshift, 0.35f, 50, 0, 60, 10)
            .delay      (On, 60, SyncFree, 0, 3000, 6, false, 20)
            .reverb     (On, 18, 5, 55, 70, 6, 60, 300, 8000, 0)
            .limiter    (On, -0.3f, 60).v },

        { "Air / Whisper", "breathy  |  close  |  delicate",
          Builder()
            .macros     (78, 70, 25, 8, 120, 55, -1)
            .pitch      (On, 60, 0, 0)
            .eq         (On, 120, 20000,  120, -2.0f,  300, -1.0f, 0.8f,  4000, 1.0f, 0.9f,  12000, 5.0f)
            .deess      (On, 8000, -34, 10)
            .fastComp   (On, -36, 4, 1, 80, 100, 2)
            .smoothComp (On, -30, 3, 20, 300, 100, 1.5f)
            .drive      (On, SatTape, 10, 0, 8, 8)
            .motion     (On, MotionMicroshift, 0.3f, 55, 0, 80, 20)
            .delay      (On, 90, SyncFree, 10, 3000, 10, false, 20)
            .reverb     (On, 50, 20, 40, 90, 30, 30, 300, 7000, 0)
            .limiter    (On, -0.5f, 80).v }
    };

    return presets;
}

int factoryPresetCount()
{
    return (int) factoryPresets().size();
}

juce::String factoryPresetName (int index)
{
    const auto& p = factoryPresets();
    if (p.empty())
        return {};

    return p[(size_t) juce::jlimit (0, (int) p.size() - 1, index)].name;
}

juce::String factoryPresetTag (int index)
{
    const auto& p = factoryPresets();
    if (p.empty())
        return {};

    return p[(size_t) juce::jlimit (0, (int) p.size() - 1, index)].tag;
}

juce::StringArray factoryPresetProblems (int index)
{
    juce::StringArray problems;
    const auto& presets = factoryPresets();

    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return { "no such preset" };

    std::vector<int> seen ((size_t) Param::Count, 0);

    for (const auto& [p, value] : presets[(size_t) index].values)
    {
        const auto& d = paramDef (p);
        ++seen[(size_t) p];

        if (! isPresetParam (p))
            problems.add (juce::String (d.id) + " is not a preset parameter");

        if (value < d.min - 1.0e-4f || value > d.max + 1.0e-4f)
            problems.add (juce::String (d.id) + " = " + juce::String (value) + " is outside "
                          + juce::String (d.min) + ".." + juce::String (d.max));
    }

    for (int i = 0; i < (int) Param::Count; ++i)
    {
        const auto p = (Param) i;

        if (isPresetParam (p) && seen[(size_t) i] != 1)
            problems.add (juce::String (paramDef (p).id)
                          + (seen[(size_t) i] == 0 ? " is not set" : " is set more than once"));
    }

    return problems;
}

void applyFactoryPreset (juce::AudioProcessorValueTreeState& state, int index)
{
    const auto& presets = factoryPresets();
    if (presets.empty())
        return;

    index = juce::jlimit (0, (int) presets.size() - 1, index);

    // Every preset parameter is stated by the preset, so there is no reset-to-
    // default pass; key, scale and bypass are left exactly as they were.
    jassert (factoryPresetProblems (index).isEmpty());

    for (const auto& [p, value] : presets[(size_t) index].values)
    {
        if (auto* param = state.getParameter (paramId (p)))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    }
}

} // namespace vx

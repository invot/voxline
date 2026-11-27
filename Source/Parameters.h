#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace vx
{

enum class Param : int
{
    Bypass = 0,

    Press, Tone, Tune, Grit, Width, Space, Level,

    TuneOn, TuneSpeed, TuneKey, TuneScale, Formant, OctaveMix,

    EqOn, Hpf, LowFreq, LowGain, BodyFreq, BodyGain, BodyQ,
    PresFreq, PresGain, PresQ, AirFreq, AirGain, LpFreq,

    DeessOn, DeessFreq, DeessThresh, DeessAmount,

    Comp1On, Comp1Thresh, Comp1Ratio, Comp1Attack, Comp1Release, Comp1Mix, Comp1Makeup,
    Comp2On, Comp2Thresh, Comp2Ratio, Comp2Attack, Comp2Release, Comp2Mix, Comp2Makeup,

    SatOn, SatMode, SatDrive, SatTone, SatMix, SatBits,

    MotionOn, MotionMode, MotionRate, MotionDepth, MotionFeedback, MotionWidth, MotionMix,

    DelayOn, DelayTime, DelaySync, DelayFeedback, DelayTone, DelayMix, DelayPingPong, DelayDuck,

    VerbOn, VerbSize, VerbPre, VerbDamp, VerbWidth, VerbMix, VerbDuck, VerbHP, VerbLP, VerbShimmer,

    LimiterOn, Ceiling, LimiterRelease,

    Count
};

enum class ParamKind { Float, Choice, Bool };

struct ParamDef
{
    const char* id = "";
    const char* name = "";
    ParamKind kind = ParamKind::Float;
    float min = 0.0f, max = 1.0f, def = 0.0f, skew = 1.0f;
    const char* suffix = "";
    int decimals = 0;
    const char* choices[12] = {};
};

const ParamDef& paramDef (Param);
const char* paramId (Param);
juce::NormalisableRange<float> paramRange (Param);
juce::String formatParamValue (Param, float value);
juce::StringArray paramChoices (Param);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace vx

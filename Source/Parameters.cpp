#include "Parameters.h"

#include <array>
#include <iterator>
#include <cmath>

namespace vx
{

namespace
{
    constexpr ParamKind F = ParamKind::Float;
    constexpr ParamKind C = ParamKind::Choice;
    constexpr ParamKind B = ParamKind::Bool;

    const ParamDef defs[] =
    {
        { "bypass", "Bypass", B, 0, 1, 0, 1, "", 0 },

        { "press", "Press", F, 0, 100, 50, 1, "%", 0 },
        { "tone", "Tone", F, 0, 100, 50, 1, "%", 0 },
        { "tune", "Tune", F, 0, 100, 35, 1, "%", 0 },
        { "grit", "Grit", F, 0, 100, 15, 1, "%", 0 },
        { "width", "Width", F, 0, 200, 100, 1, "%", 0 },
        { "space", "Space", F, 0, 100, 35, 1, "%", 0 },
        { "level", "Level", F, -24, 12, 0, 1, "dB", 1 },

        { "tuneOn", "Pitch correction", B, 0, 1, 1, 1, "", 0 },
        { "tuneSpeed", "Retune speed", F, 1, 200, 35, 0.4f, "ms", 0 },
        { "tuneKey", "Key", C, 0, 11, 0, 1, "", 0,
          { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" } },
        { "tuneScale", "Scale", C, 0, 7, 0, 1, "", 0,
          { "Chromatic", "Major", "Natural minor", "Harmonic minor",
            "Major pentatonic", "Minor pentatonic", "Dorian", "Mixolydian" } },
        { "formant", "Formant", F, -12, 12, 0, 1, "st", 1 },
        { "octaveMix", "Sub octave", F, 0, 100, 0, 1, "%", 0 },

        { "eqOn", "EQ", B, 0, 1, 1, 1, "", 0 },
        { "hpf", "Low cut", F, 20, 400, 85, 0.5f, "Hz", 0 },
        { "lowFreq", "Body low", F, 40, 300, 140, 0.7f, "Hz", 0 },
        { "lowGain", "Body low gain", F, -12, 12, 0, 1, "dB", 1 },
        { "bodyFreq", "Body", F, 150, 800, 300, 0.7f, "Hz", 0 },
        { "bodyGain", "Body gain", F, -12, 12, 0, 1, "dB", 1 },
        { "bodyQ", "Body Q", F, 0.3f, 3, 0.8f, 0.8f, "", 2 },
        { "presFreq", "Presence", F, 1500, 7000, 3200, 0.7f, "Hz", 0 },
        { "presGain", "Presence gain", F, -12, 12, 0, 1, "dB", 1 },
        { "presQ", "Presence Q", F, 0.3f, 3, 0.9f, 0.8f, "", 2 },
        { "airFreq", "Air", F, 6000, 16000, 11000, 0.7f, "Hz", 0 },
        { "airGain", "Air gain", F, -12, 12, 0, 1, "dB", 1 },
        { "lpFreq", "Top roll-off", F, 2000, 20000, 20000, 0.5f, "Hz", 0 },

        { "deessOn", "De-esser", B, 0, 1, 1, 1, "", 0 },
        { "deessFreq", "Sibilance", F, 3000, 10000, 6800, 0.7f, "Hz", 0 },
        { "deessThresh", "Threshold", F, -60, 0, -30, 1, "dB", 1 },
        { "deessAmount", "Reduction", F, 0, 15, 6, 1, "dB", 1 },

        { "comp1On", "Fast comp", B, 0, 1, 1, 1, "", 0 },
        { "comp1Thresh", "Threshold", F, -60, 12, -22, 1, "dB", 1 },
        { "comp1Ratio", "Ratio", F, 1, 20, 4, 0.8f, ":1", 1 },
        { "comp1Attack", "Attack", F, 0.1f, 30, 3, 0.4f, "ms", 1 },
        { "comp1Release", "Release", F, 20, 600, 80, 0.5f, "ms", 0 },
        { "comp1Mix", "Mix", F, 0, 100, 100, 1, "%", 0 },
        { "comp1Makeup", "Makeup", F, -12, 12, 0, 1, "dB", 1 },

        { "comp2On", "Smooth comp", B, 0, 1, 1, 1, "", 0 },
        { "comp2Thresh", "Threshold", F, -60, 12, -18, 1, "dB", 1 },
        { "comp2Ratio", "Ratio", F, 1, 20, 2.5f, 0.8f, ":1", 1 },
        { "comp2Attack", "Attack", F, 1, 200, 30, 0.5f, "ms", 0 },
        { "comp2Release", "Release", F, 50, 2000, 300, 0.5f, "ms", 0 },
        { "comp2Mix", "Mix", F, 0, 100, 100, 1, "%", 0 },
        { "comp2Makeup", "Makeup", F, -12, 12, 0, 1, "dB", 1 },

        { "satOn", "Drive", B, 0, 1, 1, 1, "", 0 },
        { "satMode", "Character", C, 0, 3, 0, 1, "", 0,
          { "Tube", "Tape", "Fuzz", "Crush" } },
        { "satDrive", "Drive", F, 0, 100, 20, 1, "%", 0 },
        { "satTone", "Tone", F, -100, 100, 0, 1, "%", 0 },
        { "satMix", "Mix", F, 0, 100, 12, 1, "%", 0 },
        { "satBits", "Crush bits", F, 2, 16, 8, 1, "bit", 0 },

        { "motionOn", "Motion", B, 0, 1, 0, 1, "", 0 },
        { "motionMode", "Mode", C, 0, 3, 0, 1, "", 0,
          { "Chorus", "Flanger", "Doubler", "Microshift" } },
        { "motionRate", "Rate", F, 0.05f, 5, 0.6f, 0.5f, "Hz", 2 },
        { "motionDepth", "Depth", F, 0, 100, 50, 1, "%", 0 },
        { "motionFeedback", "Feedback", F, -90, 90, 0, 1, "%", 0 },
        { "motionWidth", "Width", F, 0, 100, 70, 1, "%", 0 },
        { "motionMix", "Mix", F, 0, 100, 15, 1, "%", 0 },

        { "delayOn", "Delay", B, 0, 1, 0, 1, "", 0 },
        { "delayTime", "Time", F, 20, 1500, 300, 0.5f, "ms", 0 },
        { "delaySync", "Sync", C, 0, 9, 0, 1, "", 0,
          { "Free", "1/32", "1/16", "1/8T", "1/16D", "1/8", "1/8D", "1/4", "1/4D", "1/2" } },
        { "delayFeedback", "Feedback", F, 0, 92, 25, 1, "%", 0 },
        { "delayTone", "Tone", F, 800, 18000, 5000, 0.5f, "Hz", 0 },
        { "delayMix", "Send", F, 0, 100, 12, 1, "%", 0 },
        { "delayPingPong", "Ping-pong", B, 0, 1, 1, 1, "", 0 },
        { "delayDuck", "Ducking", F, 0, 100, 35, 1, "%", 0 },

        { "verbOn", "Reverb", B, 0, 1, 1, 1, "", 0 },
        { "verbSize", "Size", F, 0, 100, 35, 1, "%", 0 },
        { "verbPre", "Pre-delay", F, 0, 150, 20, 1, "ms", 0 },
        { "verbDamp", "Damping", F, 0, 100, 50, 1, "%", 0 },
        { "verbWidth", "Width", F, 0, 100, 85, 1, "%", 0 },
        { "verbMix", "Send", F, 0, 100, 12, 1, "%", 0 },
        { "verbDuck", "Ducking", F, 0, 100, 40, 1, "%", 0 },
        { "verbHP", "Return low cut", F, 100, 1200, 300, 0.7f, "Hz", 0 },
        { "verbLP", "Return top", F, 2000, 16000, 9000, 0.6f, "Hz", 0 },
        { "verbShimmer", "Shimmer", F, 0, 100, 0, 1, "%", 0 },

        { "limiterOn", "Limiter", B, 0, 1, 1, 1, "", 0 },
        { "ceiling", "Ceiling", F, -12, 0, -0.3f, 1, "dB", 1 },
        { "limiterRelease", "Release", F, 10, 500, 80, 0.5f, "ms", 0 }
    };

    static_assert (std::size (defs) == (size_t) Param::Count, "parameter table size");
}

const ParamDef& paramDef (Param p)
{
    const int i = (int) p;
    return defs[(size_t) (i >= 0 && i < (int) Param::Count ? i : 0)];
}

const char* paramId (Param p)
{
    return paramDef (p).id;
}

juce::NormalisableRange<float> paramRange (Param p)
{
    const auto& d = paramDef (p);
    return juce::NormalisableRange<float> (d.min, d.max, 0.0f, d.skew > 0.0f ? d.skew : 1.0f);
}

juce::StringArray paramChoices (Param p)
{
    juce::StringArray out;

    for (const auto* c : paramDef (p).choices)
        if (c != nullptr && *c != 0)
            out.add (c);

    return out;
}

juce::String formatParamValue (Param p, float value)
{
    const auto& d = paramDef (p);

    if (d.kind == ParamKind::Bool)
        return value >= 0.5f ? "On" : "Off";

    if (d.kind == ParamKind::Choice)
    {
        const auto choices = paramChoices (p);
        const int index = juce::jlimit (0, choices.size() - 1, (int) std::lround (value));
        return choices.isEmpty() ? juce::String (index) : choices[index];
    }

    const juce::String suffix = d.suffix;

    if (suffix == "%")
        return juce::String (juce::roundToInt (value)) + "%";

    if (suffix == "Hz")
    {
        if (value >= 1000.0f)
            return juce::String (value / 1000.0f, 1) + " kHz";

        return juce::String (juce::roundToInt (value)) + " Hz";
    }

    if (suffix == "dB")
        return (value >= 0.0f ? "+" : "") + juce::String (value, 1) + " dB";

    if (suffix == ":1")
        return juce::String (value, 1) + ":1";

    if (suffix == "ms")
        return juce::String (value, value < 10.0f ? 1 : 0) + " ms";

    if (suffix == "st")
        return (value >= 0.0f ? "+" : "") + juce::String (value, 1) + " st";

    if (suffix == "bit")
        return juce::String (juce::roundToInt (value)) + " bit";

    return juce::String (value, d.decimals);
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < (int) Param::Count; ++i)
    {
        const auto p = (Param) i;
        const auto& d = defs[(size_t) i];
        const juce::String id = d.id;
        const juce::String name = d.name;

        if (d.kind == ParamKind::Bool)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (id, name, d.def >= 0.5f));
        }
        else if (d.kind == ParamKind::Choice)
        {
            const auto choices = paramChoices (p);
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                id, name, choices, juce::jlimit (0, choices.size() - 1, (int) std::lround (d.def))));
        }
        else
        {
            auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [p] (float v, int) { return formatParamValue (p, v); });

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                id, name, paramRange (p), d.def, attrs));
        }
    }

    return layout;
}

} // namespace vx

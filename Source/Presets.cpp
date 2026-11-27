#include "Presets.h"

namespace vx
{

using P = Param;

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets =
    {
        {
            "Close / Intimate", "dry  |  forward  |  detailed",
            {
                { P::Press, 62 }, { P::Tone, 55 }, { P::Tune, 55 }, { P::Grit, 18 },
                { P::Width, 95 }, { P::Space, 30 }, { P::Level, 0 },
                { P::TuneSpeed, 25 }, { P::Formant, 0 }, { P::OctaveMix, 0 },
                { P::Hpf, 85 }, { P::LowFreq, 150 }, { P::LowGain, 2.5f },
                { P::BodyFreq, 300 }, { P::BodyGain, 1.0f },
                { P::PresFreq, 3200 }, { P::PresGain, 1.5f },
                { P::AirFreq, 11000 }, { P::AirGain, 3.0f },
                { P::DeessFreq, 6800 }, { P::DeessThresh, -30 }, { P::DeessAmount, 7 },
                { P::Comp1Thresh, -24 }, { P::Comp1Ratio, 4 }, { P::Comp1Attack, 2 },
                { P::Comp1Release, 70 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -18 }, { P::Comp2Ratio, 3 }, { P::Comp2Attack, 25 },
                { P::Comp2Release, 320 }, { P::Comp2Makeup, 1.5f },
                { P::SatOn, 1 }, { P::SatMode, 1 }, { P::SatDrive, 18 }, { P::SatMix, 12 },
                { P::MotionOn, 0 },
                { P::DelayOn, 1 }, { P::DelayTime, 75 }, { P::DelayFeedback, 0 },
                { P::DelayTone, 3000 }, { P::DelayMix, 12 }, { P::DelayPingPong, 0 },
                { P::DelayDuck, 20 },
                { P::VerbOn, 1 }, { P::VerbSize, 25 }, { P::VerbPre, 8 },
                { P::VerbDamp, 55 }, { P::VerbWidth, 80 }, { P::VerbMix, 10 },
                { P::VerbDuck, 50 }, { P::VerbHP, 300 }, { P::VerbLP, 8000 },
                { P::VerbShimmer, 0 },
                { P::Ceiling, -0.3f }
            }
        },
        {
            "Grand / Stadium", "wide  |  long tail  |  ducked",
            {
                { P::Press, 70 }, { P::Tone, 58 }, { P::Tune, 35 }, { P::Grit, 30 },
                { P::Width, 130 }, { P::Space, 62 }, { P::Level, -1.0f },
                { P::TuneSpeed, 40 },
                { P::Hpf, 70 }, { P::LowFreq, 120 }, { P::LowGain, 1.0f },
                { P::PresFreq, 3000 }, { P::PresGain, 2.5f },
                { P::AirFreq, 11000 }, { P::AirGain, 1.5f },
                { P::Comp1Thresh, -16 }, { P::Comp1Ratio, 5 }, { P::Comp1Attack, 5 },
                { P::Comp1Release, 100 }, { P::Comp1Makeup, 1 },
                { P::Comp2Thresh, -10 }, { P::Comp2Ratio, 12 }, { P::Comp2Attack, 2 },
                { P::Comp2Release, 150 }, { P::Comp2Mix, 35 },
                { P::SatMode, 0 }, { P::SatDrive, 35 }, { P::SatMix, 25 },
                { P::MotionOn, 1 }, { P::MotionMode, 3 }, { P::MotionRate, 0.4f },
                { P::MotionDepth, 60 }, { P::MotionWidth, 70 }, { P::MotionMix, 10 },
                { P::DelayOn, 1 }, { P::DelaySync, 6 }, { P::DelayFeedback, 35 },
                { P::DelayTone, 5000 }, { P::DelayMix, 18 }, { P::DelayPingPong, 1 },
                { P::DelayDuck, 55 },
                { P::VerbOn, 1 }, { P::VerbSize, 65 }, { P::VerbPre, 55 },
                { P::VerbDamp, 45 }, { P::VerbWidth, 90 }, { P::VerbMix, 30 },
                { P::VerbDuck, 55 }, { P::VerbHP, 350 }, { P::VerbLP, 7000 },
                { P::Ceiling, -0.8f }, { P::LimiterRelease, 120 }
            }
        },
        {
            "Chorus Stack", "layered doubles  |  wide  |  glued",
            {
                { P::Press, 65 }, { P::Tone, 60 }, { P::Tune, 60 }, { P::Grit, 20 },
                { P::Width, 140 }, { P::Space, 45 }, { P::Level, -1.5f },
                { P::TuneSpeed, 30 },
                { P::Hpf, 150 }, { P::LowFreq, 200 }, { P::LowGain, -2.0f },
                { P::BodyFreq, 300 }, { P::BodyGain, -2.0f },
                { P::PresFreq, 3200 }, { P::PresGain, -1.5f },
                { P::AirFreq, 11000 }, { P::AirGain, 2.0f },
                { P::DeessFreq, 7200 }, { P::DeessThresh, -32 }, { P::DeessAmount, 9 },
                { P::Comp1Thresh, -24 }, { P::Comp1Ratio, 6 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 60 }, { P::Comp1Makeup, 1.5f },
                { P::Comp2Thresh, -20 }, { P::Comp2Ratio, 4 }, { P::Comp2Attack, 20 },
                { P::Comp2Release, 250 }, { P::Comp2Makeup, 1 },
                { P::SatMode, 1 }, { P::SatDrive, 15 }, { P::SatMix, 12 },
                { P::MotionOn, 1 }, { P::MotionMode, 2 }, { P::MotionRate, 0.3f },
                { P::MotionDepth, 50 }, { P::MotionWidth, 85 }, { P::MotionMix, 28 },
                { P::DelayOn, 1 }, { P::DelaySync, 6 }, { P::DelayFeedback, 30 },
                { P::DelayTone, 5000 }, { P::DelayMix, 15 }, { P::DelayPingPong, 1 },
                { P::DelayDuck, 50 },
                { P::VerbOn, 1 }, { P::VerbSize, 45 }, { P::VerbPre, 30 },
                { P::VerbDamp, 50 }, { P::VerbWidth, 90 }, { P::VerbMix, 22 },
                { P::VerbDuck, 45 }, { P::VerbHP, 300 }, { P::VerbLP, 8500 },
                { P::Ceiling, -0.5f }
            }
        },
        {
            "Polished Pop", "hard tuned  |  bright  |  throw",
            {
                { P::Press, 68 }, { P::Tone, 65 }, { P::Tune, 85 }, { P::Grit, 12 },
                { P::Width, 115 }, { P::Space, 38 }, { P::Level, -0.5f },
                { P::TuneSpeed, 8 }, { P::OctaveMix, 12 },
                { P::Hpf, 90 }, { P::LowFreq, 140 }, { P::LowGain, 1.0f },
                { P::BodyFreq, 300 }, { P::BodyGain, -1.5f },
                { P::PresFreq, 3500 }, { P::PresGain, 2.5f },
                { P::AirFreq, 12000 }, { P::AirGain, 4.0f },
                { P::DeessFreq, 7300 }, { P::DeessThresh, -32 }, { P::DeessAmount, 9 },
                { P::Comp1Thresh, -26 }, { P::Comp1Ratio, 3.5f }, { P::Comp1Attack, 2 },
                { P::Comp1Release, 60 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -22 }, { P::Comp2Ratio, 3 }, { P::Comp2Attack, 15 },
                { P::Comp2Release, 200 }, { P::Comp2Makeup, 1 },
                { P::SatMode, 0 }, { P::SatDrive, 15 }, { P::SatMix, 10 },
                { P::MotionOn, 1 }, { P::MotionMode, 3 }, { P::MotionRate, 0.4f },
                { P::MotionDepth, 60 }, { P::MotionWidth, 70 }, { P::MotionMix, 16 },
                { P::DelayOn, 1 }, { P::DelaySync, 5 }, { P::DelayFeedback, 25 },
                { P::DelayTone, 6000 }, { P::DelayMix, 14 }, { P::DelayPingPong, 1 },
                { P::DelayDuck, 60 },
                { P::VerbOn, 1 }, { P::VerbSize, 45 }, { P::VerbPre, 25 },
                { P::VerbDamp, 45 }, { P::VerbWidth, 85 }, { P::VerbMix, 18 },
                { P::VerbDuck, 50 }, { P::VerbHP, 300 }, { P::VerbLP, 9000 },
                { P::Ceiling, -0.3f }
            }
        },
        {
            "Ethereal", "washed  |  shimmering  |  soft",
            {
                { P::Press, 55 }, { P::Tone, 32 }, { P::Tune, 28 }, { P::Grit, 15 },
                { P::Width, 135 }, { P::Space, 88 }, { P::Level, -2.0f },
                { P::TuneSpeed, 80 }, { P::Formant, -1.0f },
                { P::Hpf, 100 }, { P::PresFreq, 4000 }, { P::PresGain, -1.0f },
                { P::AirFreq, 10000 }, { P::AirGain, -2.0f }, { P::LpFreq, 8000 },
                { P::DeessFreq, 6500 }, { P::DeessThresh, -28 }, { P::DeessAmount, 5 },
                { P::Comp1Thresh, -22 }, { P::Comp1Ratio, 3 }, { P::Comp1Attack, 8 },
                { P::Comp1Release, 120 }, { P::Comp1Makeup, 1 },
                { P::Comp2Thresh, -20 }, { P::Comp2Ratio, 2.5f }, { P::Comp2Attack, 40 },
                { P::Comp2Release, 400 }, { P::Comp2Makeup, 1 },
                { P::SatMode, 1 }, { P::SatDrive, 20 }, { P::SatMix, 15 },
                { P::MotionOn, 1 }, { P::MotionMode, 3 }, { P::MotionRate, 0.25f },
                { P::MotionDepth, 70 }, { P::MotionWidth, 95 }, { P::MotionMix, 30 },
                { P::DelayOn, 1 }, { P::DelaySync, 6 }, { P::DelayFeedback, 45 },
                { P::DelayTone, 4000 }, { P::DelayMix, 22 }, { P::DelayPingPong, 1 },
                { P::DelayDuck, 30 },
                { P::VerbOn, 1 }, { P::VerbSize, 88 }, { P::VerbPre, 0 },
                { P::VerbDamp, 35 }, { P::VerbWidth, 100 }, { P::VerbMix, 50 },
                { P::VerbDuck, 20 }, { P::VerbHP, 250 }, { P::VerbLP, 6000 },
                { P::VerbShimmer, 45 }, { P::Ceiling, -1.0f }
            }
        },
        {
            "Robotic", "hard tuned  |  crushed  |  metallic",
            {
                { P::Press, 75 }, { P::Tone, 52 }, { P::Tune, 100 }, { P::Grit, 45 },
                { P::Width, 80 }, { P::Space, 22 }, { P::Level, -1.0f },
                { P::TuneSpeed, 1 }, { P::Formant, 3.0f },
                { P::Hpf, 120 }, { P::LowFreq, 120 }, { P::LowGain, -2.0f },
                { P::BodyFreq, 300 }, { P::BodyGain, 1.0f },
                { P::PresFreq, 3000 }, { P::PresGain, 2.0f },
                { P::AirFreq, 11000 }, { P::AirGain, 1.0f }, { P::LpFreq, 12000 },
                { P::DeessFreq, 7000 }, { P::DeessThresh, -28 }, { P::DeessAmount, 6 },
                { P::Comp1Thresh, -30 }, { P::Comp1Ratio, 10 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 40 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -28 }, { P::Comp2Ratio, 8 }, { P::Comp2Attack, 5 },
                { P::Comp2Release, 120 }, { P::Comp2Makeup, 1.5f },
                { P::SatMode, 3 }, { P::SatDrive, 30 }, { P::SatMix, 25 }, { P::SatBits, 6 },
                { P::MotionOn, 1 }, { P::MotionMode, 0 }, { P::MotionRate, 3.5f },
                { P::MotionDepth, 30 }, { P::MotionWidth, 50 }, { P::MotionMix, 25 },
                { P::DelayOn, 1 }, { P::DelaySync, 2 }, { P::DelayFeedback, 30 },
                { P::DelayTone, 6000 }, { P::DelayMix, 12 }, { P::DelayDuck, 30 },
                { P::VerbOn, 1 }, { P::VerbSize, 20 }, { P::VerbPre, 5 },
                { P::VerbDamp, 30 }, { P::VerbWidth, 60 }, { P::VerbMix, 8 },
                { P::VerbDuck, 40 }, { P::VerbHP, 400 }, { P::VerbLP, 9000 },
                { P::Ceiling, -0.5f }, { P::LimiterRelease, 30 }
            }
        },
        {
            "Grunge", "parallel grit  |  fast limit  |  raw",
            {
                { P::Press, 80 }, { P::Tone, 50 }, { P::Tune, 10 }, { P::Grit, 70 },
                { P::Width, 105 }, { P::Space, 25 }, { P::Level, -2.0f },
                { P::TuneSpeed, 50 },
                { P::Hpf, 110 }, { P::LowFreq, 120 }, { P::LowGain, -2.0f },
                { P::BodyFreq, 250 }, { P::BodyGain, 2.0f },
                { P::PresFreq, 2800 }, { P::PresGain, 3.0f },
                { P::AirFreq, 10000 }, { P::AirGain, -1.0f }, { P::LpFreq, 16000 },
                { P::DeessFreq, 6500 }, { P::DeessThresh, -30 }, { P::DeessAmount, 6 },
                { P::Comp1Thresh, -28 }, { P::Comp1Ratio, 8 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 50 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -24 }, { P::Comp2Ratio, 8 }, { P::Comp2Attack, 5 },
                { P::Comp2Release, 120 }, { P::Comp2Makeup, 2 },
                { P::SatMode, 2 }, { P::SatDrive, 65 }, { P::SatTone, 10 }, { P::SatMix, 45 },
                { P::MotionOn, 1 }, { P::MotionMode, 0 }, { P::MotionRate, 0.5f },
                { P::MotionDepth, 40 }, { P::MotionWidth, 60 }, { P::MotionMix, 6 },
                { P::DelayOn, 1 }, { P::DelayTime, 90 }, { P::DelayFeedback, 15 },
                { P::DelayTone, 3500 }, { P::DelayMix, 8 }, { P::DelayPingPong, 0 },
                { P::VerbOn, 1 }, { P::VerbSize, 30 }, { P::VerbPre, 15 },
                { P::VerbDamp, 50 }, { P::VerbWidth, 80 }, { P::VerbMix, 12 },
                { P::VerbDuck, 30 }, { P::VerbHP, 300 }, { P::VerbLP, 7000 },
                { P::Ceiling, -1.0f }, { P::LimiterRelease, 40 }
            }
        },
        {
            "Telephone", "band-passed  |  mono  |  lo-fi",
            {
                { P::Press, 85 }, { P::Tone, 40 }, { P::Tune, 15 }, { P::Grit, 55 },
                { P::Width, 0 }, { P::Space, 0 }, { P::Level, -1.5f },
                { P::TuneSpeed, 40 },
                { P::Hpf, 500 }, { P::LowGain, 0 }, { P::BodyFreq, 1200 },
                { P::BodyGain, 5.0f }, { P::BodyQ, 2.5f },
                { P::PresFreq, 3000 }, { P::PresGain, 0 },
                { P::AirFreq, 8000 }, { P::AirGain, -6.0f }, { P::LpFreq, 3000 },
                { P::DeessOn, 0 },
                { P::Comp1Thresh, -34 }, { P::Comp1Ratio, 8 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 50 }, { P::Comp1Makeup, 3 },
                { P::Comp2Thresh, -30 }, { P::Comp2Ratio, 6 }, { P::Comp2Attack, 10 },
                { P::Comp2Release, 150 }, { P::Comp2Makeup, 2 },
                { P::SatMode, 2 }, { P::SatDrive, 45 }, { P::SatMix, 40 },
                { P::MotionOn, 0 },
                { P::DelayOn, 0 }, { P::VerbOn, 0 },
                { P::Ceiling, -1.0f }
            }
        },
        {
            "Rap / Front", "tight  |  dry  |  present",
            {
                { P::Press, 72 }, { P::Tone, 55 }, { P::Tune, 40 }, { P::Grit, 20 },
                { P::Width, 100 }, { P::Space, 22 }, { P::Level, 0 },
                { P::TuneSpeed, 20 },
                { P::Hpf, 95 }, { P::LowFreq, 130 }, { P::LowGain, 1.0f },
                { P::PresFreq, 3500 }, { P::PresGain, 2.0f },
                { P::AirFreq, 11000 }, { P::AirGain, 2.0f },
                { P::DeessFreq, 7000 }, { P::DeessThresh, -30 }, { P::DeessAmount, 7 },
                { P::Comp1Thresh, -26 }, { P::Comp1Ratio, 5 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 60 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -22 }, { P::Comp2Ratio, 4 }, { P::Comp2Attack, 15 },
                { P::Comp2Release, 220 }, { P::Comp2Makeup, 1 },
                { P::SatMode, 0 }, { P::SatDrive, 15 }, { P::SatMix, 8 },
                { P::MotionOn, 1 }, { P::MotionMode, 3 }, { P::MotionRate, 0.35f },
                { P::MotionDepth, 50 }, { P::MotionWidth, 60 }, { P::MotionMix, 10 },
                { P::DelayOn, 1 }, { P::DelayTime, 60 }, { P::DelayFeedback, 0 },
                { P::DelayTone, 3000 }, { P::DelayMix, 6 }, { P::DelayPingPong, 0 },
                { P::DelayDuck, 20 },
                { P::VerbOn, 1 }, { P::VerbSize, 18 }, { P::VerbPre, 5 },
                { P::VerbDamp, 55 }, { P::VerbWidth, 70 }, { P::VerbMix, 6 },
                { P::VerbDuck, 60 }, { P::VerbHP, 300 }, { P::VerbLP, 8000 },
                { P::Ceiling, -0.3f }, { P::LimiterRelease, 60 }
            }
        },
        {
            "Air / Whisper", "breathy  |  close  |  delicate",
            {
                { P::Press, 78 }, { P::Tone, 70 }, { P::Tune, 25 }, { P::Grit, 8 },
                { P::Width, 120 }, { P::Space, 55 }, { P::Level, -1.0f },
                { P::TuneSpeed, 60 },
                { P::Hpf, 120 }, { P::LowFreq, 120 }, { P::LowGain, -2.0f },
                { P::BodyFreq, 300 }, { P::BodyGain, -1.0f },
                { P::PresFreq, 4000 }, { P::PresGain, 1.0f },
                { P::AirFreq, 12000 }, { P::AirGain, 5.0f },
                { P::DeessFreq, 8000 }, { P::DeessThresh, -34 }, { P::DeessAmount, 10 },
                { P::Comp1Thresh, -36 }, { P::Comp1Ratio, 4 }, { P::Comp1Attack, 1 },
                { P::Comp1Release, 80 }, { P::Comp1Makeup, 2 },
                { P::Comp2Thresh, -30 }, { P::Comp2Ratio, 3 }, { P::Comp2Attack, 20 },
                { P::Comp2Release, 300 }, { P::Comp2Makeup, 1.5f },
                { P::SatMode, 1 }, { P::SatDrive, 10 }, { P::SatMix, 8 },
                { P::MotionOn, 1 }, { P::MotionMode, 3 }, { P::MotionRate, 0.3f },
                { P::MotionDepth, 55 }, { P::MotionWidth, 80 }, { P::MotionMix, 20 },
                { P::DelayOn, 1 }, { P::DelayTime, 90 }, { P::DelayFeedback, 10 },
                { P::DelayTone, 3000 }, { P::DelayMix, 10 }, { P::DelayPingPong, 0 },
                { P::DelayDuck, 20 },
                { P::VerbOn, 1 }, { P::VerbSize, 50 }, { P::VerbPre, 20 },
                { P::VerbDamp, 40 }, { P::VerbWidth, 90 }, { P::VerbMix, 30 },
                { P::VerbDuck, 30 }, { P::VerbHP, 300 }, { P::VerbLP, 7000 },
                { P::Ceiling, -0.5f }
            }
        }
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

void applyFactoryPreset (juce::AudioProcessorValueTreeState& state, int index)
{
    const auto& presets = factoryPresets();
    if (presets.empty())
        return;

    index = juce::jlimit (0, (int) presets.size() - 1, index);

    for (int i = 0; i < (int) Param::Count; ++i)
    {
        if (auto* param = state.getParameter (paramId ((Param) i)))
        {
            param->setValueNotifyingHost (param->convertTo0to1 (paramDef ((Param) i).def));
        }
    }

    for (const auto& [p, value] : presets[(size_t) index].values)
    {
        if (auto* param = state.getParameter (paramId (p)))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    }
}

} // namespace vx

#pragma once

#include "Parameters.h"

#include <vector>

namespace vx
{

struct FactoryPreset
{
    juce::String name;
    juce::String tag;
    std::vector<std::pair<Param, float>> values;
};

const std::vector<FactoryPreset>& factoryPresets();
int factoryPresetCount();
juce::String factoryPresetName (int);
juce::String factoryPresetTag (int);

/** True for every parameter a factory preset must state. Bypass, key and scale
    are not preset parameters: they belong to the session and the song. */
bool isPresetParam (Param);

/** Empty when the preset sets every preset parameter exactly once, in range. */
juce::StringArray factoryPresetProblems (int index);

/** Applies every value of the preset. Key, scale and bypass are untouched. */
void applyFactoryPreset (juce::AudioProcessorValueTreeState&, int index);

} // namespace vx

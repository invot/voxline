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

/** Resets every parameter to its neutral default, then applies the preset. */
void applyFactoryPreset (juce::AudioProcessorValueTreeState&, int index);

} // namespace vx

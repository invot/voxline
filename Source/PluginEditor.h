#pragma once

#include "PluginProcessor.h"
#include "ui/ChainView.h"
#include "ui/Widgets.h"

#include <memory>

namespace vx
{

/** One page: a header (presets, meters, throw, bypass) over the chain view. */
class VoxlineEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VoxlineEditor (VoxlineProcessor&);
    ~VoxlineEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Used by the smoke/snapshot harness.
    ui::ChainView& getChainView() { return chain; }
    void showPresetForSnapshot (int index) { setPreset (index); }

private:
    void timerCallback() override;
    void setPreset (int index);
    void refreshPresetDisplay (bool force = false);
    void refreshLatencyText();
    void showPresetMenu();

    VoxlineProcessor& processor;
    ui::VoxlineLNF lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    ui::ChainView chain;
    ui::LevelMeter meter;
    ui::PresetBox presetBox;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton bypassButton { "BYPASS" };
    ui::ThrowButton throwButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    int shownProgram = -1;
    juce::String latencyText = "PDC --";
    juce::Rectangle<int> headerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxlineEditor)
};

} // namespace vx

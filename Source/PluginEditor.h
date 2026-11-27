#pragma once

#include "PluginProcessor.h"
#include "ui/DetailPage.h"
#include "ui/ShapePage.h"
#include "ui/Widgets.h"

#include <memory>

namespace vx
{

class VoxlineEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VoxlineEditor (VoxlineProcessor&);
    ~VoxlineEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Used by the smoke/snapshot harness.
    ui::ShapePage& getShapePage() { return shape; }
    ui::DetailPage& getDetailPage() { return detail; }
    ui::TabBar& getTabBar() { return tabs; }
    void showPresetForSnapshot (int index) { setPreset (index); }

private:
    void timerCallback() override;
    void setPreset (int index);
    void refreshPresetDisplay();
    void refreshLatencyText();
    void showPresetMenu();

    VoxlineProcessor& processor;
    ui::VoxlineLNF lookAndFeel;

    ui::TabBar tabs { juce::StringArray { "Shape", "Detail" } };
    ui::ShapePage shape;
    ui::DetailPage detail;
    ui::LevelMeter meter;

    juce::TextButton presetButton;
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::TextButton bypassButton;
    ui::ThrowButton throwButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    int shownProgram = -1;
    juce::String latencyText = "PDC --";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxlineEditor)
};

} // namespace vx

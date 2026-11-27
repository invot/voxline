#pragma once

#include "Widgets.h"

#include <functional>
#include <memory>

namespace vx
{
class VoxlineProcessor;
}

namespace vx::ui
{

/** The default view: presets on the left, seven macros in the middle, tone and
    space graphs, and a clickable signal path. */
class ShapePage : public juce::Component
{
public:
    explicit ShapePage (VoxlineProcessor&);
    ~ShapePage() override;

    std::function<void (int)> onOpenModule;
    std::function<void (int)> onSelectPreset;

    void setSelectedPreset (int index);
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class EqGraph;
    class SpaceGraph;

    VoxlineProcessor& processor;
    PresetRail rail;
    juce::OwnedArray<MacroKnob> macros;
    std::unique_ptr<EqGraph> eqGraph;
    std::unique_ptr<SpaceGraph> spaceGraph;
    SignalPath path;
    juce::Rectangle<int> macroPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapePage)
};

} // namespace vx::ui

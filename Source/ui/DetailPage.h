#pragma once

#include "Widgets.h"

#include <memory>
#include <vector>

namespace vx
{
class VoxlineProcessor;
}

namespace vx::ui
{

class DetailPage : public juce::Component
{
public:
    explicit DetailPage (VoxlineProcessor&);
    ~DetailPage() override;

    void selectModule (int index);
    int getSelectedModule() const { return selected; }
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ControlSpec
    {
        enum class Type { Slider, Combo, Toggle };
        Type type = Type::Slider;
        Param param = Param::Bypass;
    };

    struct ModuleInfo
    {
        juce::String name;
        juce::String blurb;
        juce::Colour accent;
        bool hasOn = false;
        Param onParam = Param::Bypass;
        std::vector<ControlSpec> controls;
    };

    class ModuleList;

    float valueOf (Param) const;
    void rebuildControls();

    VoxlineProcessor& processor;
    std::vector<ModuleInfo> modules;
    juce::OwnedArray<juce::Component> controls;
    std::unique_ptr<PowerPill> power;
    std::unique_ptr<ModuleList> list;

    int selected = 0;
    int columns = 2;
    juce::Rectangle<int> panelArea, headerArea, controlsArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DetailPage)
};

} // namespace vx::ui

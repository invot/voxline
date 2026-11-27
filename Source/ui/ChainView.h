#pragma once

#include "Graphs.h"
#include "Widgets.h"

#include <memory>
#include <vector>

namespace vx
{
class VoxlineProcessor;
}

namespace vx::ui
{

/** The whole editor body. A row of effect tiles, in signal order, each with an
    on/off switch and the one control that matters most; below it, every
    control of the selected effect, grouped, with its graph. */
class ChainView : public juce::Component
{
public:
    enum Stage { Pitch = 0, Eq, DeEss, Comp, Drive, Chorus, Delay, Reverb, Stereo, Output, NumStages };

    explicit ChainView (VoxlineProcessor&);
    ~ChainView() override;

    void selectModule (int index);
    int getSelectedModule() const noexcept { return selected; }
    static juce::String stageName (int index);

    /** Every parameter the view gives a control to, for the coverage check. */
    static std::vector<Param> reachableParams();

    /** Pulls parameter and meter state; called at the editor's UI rate. */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

    struct Control
    {
        Param param;
        juce::String label {};
    };

    struct Group
    {
        juce::String title;
        std::vector<Param> power;       // empty: follows the stage switch
        std::vector<Control> controls;
    };

    enum class Graph { None, Eq, Space, Dynamics };

    struct StageSpec
    {
        juce::String name;
        juce::String blurb;
        juce::Colour accent;
        std::vector<Param> power;       // empty: always in the chain
        Param knob;
        juce::String knobLabel;
        std::vector<Group> groups;
        Graph graph = Graph::None;
    };

private:
    class Tile;
    class GroupPanel;

    void rebuildDetail();
    juce::String statusFor (int stage, const ChainParams&) const;
    static bool isRelevant (Param, const ChainParams&);

    VoxlineProcessor& processor;
    std::vector<StageSpec> stages;
    juce::OwnedArray<Tile> tiles;
    juce::OwnedArray<GroupPanel> groups;

    EqGraph eqGraph;
    SpaceGraph spaceGraph;
    DynamicsGraph dynamicsGraph;

    int selected = 0;
    juce::Rectangle<int> tileRow, detailArea, detailHeader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainView)
};

} // namespace vx::ui

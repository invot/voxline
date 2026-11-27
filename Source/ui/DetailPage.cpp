#include "DetailPage.h"
#include "../PluginProcessor.h"

namespace vx::ui
{

//==============================================================================
class DetailPage::ModuleList : public juce::Component
{
public:
    explicit ModuleList (DetailPage& o) : owner (o)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat().reduced (0.5f), 14.0f, pal::panel);

        g.setColour (pal::dim);
        g.setFont (uiFont (9.0f, true));
        g.drawText ("MODULES", 16, 12, 150, 12, juce::Justification::centredLeft, false);

        g.setColour (pal::dim.withAlpha (0.6f));
        g.setFont (uiFont (8.5f));
        g.drawText ("the whole chain", 16, 28, 150, 12, juce::Justification::centredLeft, false);

        const int count = (int) owner.modules.size();
        const int top = 48;
        const int rowH = juce::jlimit (32, 42, (getHeight() - top - 10) / std::max (1, count));

        for (int i = 0; i < count; ++i)
        {
            auto row = juce::Rectangle<int> (9, top + i * rowH, getWidth() - 18, rowH - 3);
            const bool on = i == owner.selected;
            const bool hot = i == hover && ! on;
            const auto& m = owner.modules[(size_t) i];
            const bool enabled = ! m.hasOn || owner.valueOf (m.onParam) > 0.5f;

            if (on || hot)
            {
                g.setColour (on ? pal::panelHi : pal::panelSoft.withAlpha (0.55f));
                g.fillRoundedRectangle (row.toFloat(), 8.0f);
            }

            if (on)
            {
                g.setColour (m.accent);
                g.fillRoundedRectangle ((float) row.getX() + 3.0f, (float) row.getY() + 8.0f,
                                        2.5f, (float) row.getHeight() - 16.0f, 1.25f);
            }

            g.setColour (m.accent.withAlpha (enabled ? 1.0f : 0.22f));
            g.fillEllipse ((float) row.getX() + 13.0f, (float) row.getCentreY() - 2.5f, 5.0f, 5.0f);

            g.setColour (enabled ? (on ? pal::text : pal::text.withAlpha (0.86f))
                                 : pal::dim.withAlpha (0.55f));
            g.setFont (uiFont (11.5f, on));
            g.drawText (m.name.toUpperCase(), row.getX() + 24, row.getY(),
                        row.getWidth() - 52, row.getHeight(),
                        juce::Justification::centredLeft, false);

            g.setColour (pal::dim.withAlpha (0.55f));
            g.setFont (uiFont (7.5f, true));
            g.drawText (juce::String (i + 1).paddedLeft ('0', 2), row.getRight() - 26,
                        row.getY(), 16, row.getHeight(),
                        juce::Justification::centredRight, false);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int count = (int) owner.modules.size();
        const int top = 48;
        const int rowH = juce::jlimit (32, 42, (getHeight() - top - 10) / std::max (1, count));
        const int index = (e.y - top) / std::max (1, rowH);
        const int next = (e.y >= top && index >= 0 && index < count) ? index : -1;

        if (next != hover)
        {
            hover = next;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hover = -1;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        const int count = (int) owner.modules.size();
        const int top = 48;
        const int rowH = juce::jlimit (32, 42, (getHeight() - top - 10) / std::max (1, count));
        const int index = (e.y - top) / std::max (1, rowH);

        if (e.y >= top && index >= 0 && index < count)
            owner.selectModule (index);
    }

private:
    DetailPage& owner;
    int hover = -1;
};

//==============================================================================
DetailPage::DetailPage (VoxlineProcessor& p)
    : processor (p)
{
    const auto S = [] (Param param) { return ControlSpec { ControlSpec::Type::Slider, param }; };
    const auto C = [] (Param param) { return ControlSpec { ControlSpec::Type::Combo, param }; };
    const auto T = [] (Param param) { return ControlSpec { ControlSpec::Type::Toggle, param }; };

    modules =
    {
        { "Tune",
          "Key, glide and formant. Correction is exact to the cent, from natural to robotic.",
          pal::purple, true, Param::TuneOn,
          { S (Param::Tune), S (Param::TuneSpeed), C (Param::TuneKey), C (Param::TuneScale),
            S (Param::Formant), S (Param::OctaveMix) } },

        { "EQ",
          "Low cut, body, presence and air. The Tone macro tilts the whole curve.",
          pal::teal, true, Param::EqOn,
          { S (Param::Hpf), S (Param::LowFreq), S (Param::LowGain),
            S (Param::BodyFreq), S (Param::BodyGain), S (Param::BodyQ),
            S (Param::PresFreq), S (Param::PresGain), S (Param::PresQ),
            S (Param::AirFreq), S (Param::AirGain), S (Param::LpFreq) } },

        { "De-ess",
          "Split-band control that takes the sibilance down without dulling the body.",
          pal::pink, true, Param::DeessOn,
          { S (Param::DeessFreq), S (Param::DeessThresh), S (Param::DeessAmount) } },

        { "Comp 1",
          "Fast peak control for the first few dB. Serial, with a parallel blend.",
          pal::amber, true, Param::Comp1On,
          { S (Param::Comp1Thresh), S (Param::Comp1Ratio), S (Param::Comp1Attack),
            S (Param::Comp1Release), S (Param::Comp1Mix), S (Param::Comp1Makeup) } },

        { "Comp 2",
          "Slow optical leveling. Push the ratio and pull the mix back for parallel smash.",
          pal::amber, true, Param::Comp2On,
          { S (Param::Comp2Thresh), S (Param::Comp2Ratio), S (Param::Comp2Attack),
            S (Param::Comp2Release), S (Param::Comp2Mix), S (Param::Comp2Makeup) } },

        { "Drive",
          "Tube, tape, fuzz or crush. Blended under the clean signal.",
          pal::pink, true, Param::SatOn,
          { C (Param::SatMode), S (Param::SatDrive), S (Param::SatTone),
            S (Param::SatMix), S (Param::SatBits) } },

        { "Motion",
          "Chorus, flanger, doubles, or a microshift halo. Width and depth do the work.",
          pal::purple, true, Param::MotionOn,
          { C (Param::MotionMode), S (Param::MotionRate), S (Param::MotionDepth),
            S (Param::MotionFeedback), S (Param::MotionWidth), S (Param::MotionMix) } },

        { "Delay",
          "Slapback, ping-pong and ducked throws. Sync to the host or set it free.",
          pal::teal, true, Param::DelayOn,
          { S (Param::DelayTime), C (Param::DelaySync), S (Param::DelayFeedback),
            S (Param::DelayTone), S (Param::DelayMix), T (Param::DelayPingPong),
            S (Param::DelayDuck) } },

        { "Reverb",
          "Eight-line FDN with pre-delay, damping, return EQ and octave shimmer.",
          pal::purple, true, Param::VerbOn,
          { S (Param::VerbSize), S (Param::VerbPre), S (Param::VerbDamp),
            S (Param::VerbWidth), S (Param::VerbMix), S (Param::VerbDuck),
            S (Param::VerbHP), S (Param::VerbLP), S (Param::VerbShimmer) } },

        { "Width",
          "Mid/side spread. 0% collapses to mono, 100% is natural, 200% is wide.",
          pal::teal, false, Param::Bypass,
          { S (Param::Width) } },

        { "Output",
          "Output trim, limiter and ceiling. The last thing the vocal sees.",
          pal::yellow, false, Param::Bypass,
          { S (Param::Level), T (Param::LimiterOn), S (Param::Ceiling), S (Param::LimiterRelease) } }
    };

    list = std::make_unique<ModuleList> (*this);
    addAndMakeVisible (*list);

    selectModule (0);
}

DetailPage::~DetailPage() = default;

float DetailPage::valueOf (Param p) const
{
    if (auto* value = processor.apvts.getRawParameterValue (paramId (p)))
        return value->load();

    return 0.0f;
}

void DetailPage::selectModule (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) modules.size()))
        return;

    selected = index;
    rebuildControls();
    resized();
    repaint();

    if (list != nullptr)
        list->repaint();
}

void DetailPage::rebuildControls()
{
    controls.clear();
    power.reset();

    const auto& m = modules[(size_t) selected];

    if (m.hasOn)
    {
        power = std::make_unique<PowerPill> (m.onParam, processor.apvts, m.accent);
        addAndMakeVisible (*power);
    }

    for (const auto& c : m.controls)
    {
        juce::Component* comp = nullptr;

        if (c.type == ControlSpec::Type::Slider)
            comp = new ParamSlider (c.param, processor.apvts, m.accent);
        else if (c.type == ControlSpec::Type::Combo)
            comp = new ParamCombo (c.param, processor.apvts, m.accent);
        else
            comp = new ParamToggle (c.param, processor.apvts, m.accent);

        addAndMakeVisible (comp);
        controls.add (comp);
    }

    const int count = controls.size();
    columns = count > 8 ? 3 : (count >= 4 ? 2 : 1);

    if (count == 3)
        columns = 3;
    else if (count <= 2)
        columns = std::max (1, count);
}

void DetailPage::refresh()
{
    if (list != nullptr)
        list->repaint();

    // The controls maintain themselves; only the module strip and the small
    // header carry state that can change from the host or another page.
    repaint (0, 0, getWidth(), 96);
}

void DetailPage::paint (juce::Graphics& g)
{
    if (panelArea.isEmpty())
        return;

    drawPanel (g, panelArea.toFloat(), 14.0f, pal::panel);

    const auto& m = modules[(size_t) selected];

    g.setColour (m.accent);
    g.fillEllipse ((float) panelArea.getX() + 20.0f, (float) panelArea.getY() + 24.0f, 6.0f, 6.0f);

    g.setColour (pal::text);
    g.setFont (uiFont (17.0f, true));
    g.drawText (m.name.toUpperCase(), panelArea.getX() + 34, panelArea.getY() + 15,
                panelArea.getWidth() - 150, 24, juce::Justification::centredLeft, false);

    g.setColour (pal::dim);
    g.setFont (uiFont (10.5f));
    g.drawFittedText (m.blurb, panelArea.getX() + 20, panelArea.getY() + 44,
                      panelArea.getWidth() - 140, 34, juce::Justification::topLeft, 2);

    g.setColour (pal::stroke.withAlpha (0.7f));
    g.drawHorizontalLine (panelArea.getY() + 84, (float) panelArea.getX() + 20.0f,
                          (float) panelArea.getRight() - 20.0f);

    g.setColour (pal::dim.withAlpha (0.55f));
    g.setFont (uiFont (8.0f));
    g.drawText ("double-click a control to reset  |  drag to shape",
                panelArea.getX() + 20, panelArea.getBottom() - 22,
                panelArea.getWidth() - 40, 12, juce::Justification::centredLeft, false);

    g.setFont (uiFont (7.5f, true));
    g.drawText (juce::String (selected + 1).paddedLeft ('0', 2) + " / "
                    + juce::String ((int) modules.size()).paddedLeft ('0', 2),
                panelArea.getRight() - 80, panelArea.getBottom() - 22, 60, 12,
                juce::Justification::centredRight, false);
}

void DetailPage::resized()
{
    auto r = getLocalBounds();

    if (list != nullptr)
        list->setBounds (r.removeFromLeft (200));

    r.removeFromLeft (16);
    panelArea = r;
    headerArea = panelArea.reduced (20).removeFromTop (76);

    if (power != nullptr)
        power->setBounds (panelArea.getRight() - 82, panelArea.getY() + 20, 62, 26);

    controlsArea = panelArea.reduced (20, 0).withTrimmedTop (96).withTrimmedBottom (38);

    const int count = controls.size();

    if (count == 0 || controlsArea.getWidth() <= 0)
        return;

    const int gap = 12;
    const int rows = (count + columns - 1) / columns;
    int cellW = (controlsArea.getWidth() - gap * (columns - 1)) / std::max (1, columns);

    if (columns == 1)
        cellW = juce::jmin (cellW, 460);

    const int rowGap = 2;
    const int cellH = juce::jlimit (44, 58, (controlsArea.getHeight() - rowGap * (rows - 1)) / std::max (1, rows));

    for (int i = 0; i < count; ++i)
    {
        const int col = i % columns;
        const int row = i / columns;
        controls[i]->setBounds (controlsArea.getX() + col * (cellW + gap),
                                controlsArea.getY() + row * (cellH + rowGap),
                                cellW, cellH);
    }
}

} // namespace vx::ui

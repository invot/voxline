#include "ChainView.h"
#include "../PluginProcessor.h"

#include <cmath>

namespace vx::ui
{

namespace
{
    using P = Param;

    std::vector<ChainView::StageSpec> buildStages()
    {
        using G = ChainView::Graph;

        return
        {
            { "Pitch", "Pulls each note toward the key. Amount sets how hard, speed how fast. "
                       "Key and scale stay put when you change presets.",
              pal::purple, { P::TuneOn }, P::Tune, "Amount",
              { { "Correction", {}, { { P::Tune, "Amount" }, { P::TuneSpeed, "Speed" },
                                      { P::TuneKey, "Key" }, { P::TuneScale, "Scale" } } },
                { "Voice", {}, { { P::Formant, "Formant" }, { P::OctaveMix, "Sub octave" } } } },
              G::None },

            { "EQ", "Low cut, four bands and a top roll-off. Tone tilts the whole curve darker or brighter.",
              pal::brand, { P::EqOn }, P::Tone, "Tone",
              { { "Cut + tilt", {}, { { P::Tone, "Tilt" }, { P::Hpf, "Low cut" }, { P::LpFreq, "Top cut" } } },
                { "Low", {}, { { P::LowGain, "Gain" }, { P::LowFreq, "Freq" } } },
                { "Body", {}, { { P::BodyGain, "Gain" }, { P::BodyFreq, "Freq" }, { P::BodyQ, "Q" } } },
                { "Presence", {}, { { P::PresGain, "Gain" }, { P::PresFreq, "Freq" }, { P::PresQ, "Q" } } },
                { "Air", {}, { { P::AirGain, "Gain" }, { P::AirFreq, "Freq" } } } },
              G::Eq },

            { "De-ess", "Turns sibilance down above its frequency and leaves everything below it untouched.",
              pal::pink, { P::DeessOn }, P::DeessAmount, "Reduction",
              { { "Sibilance", {}, { { P::DeessAmount, "Max reduction" }, { P::DeessThresh, "Threshold" },
                                     { P::DeessFreq, "Frequency" } } } },
              G::Eq },

            { "Comp", "Two compressors in series: fast for peaks, smooth for level. Press pushes both.",
              pal::amber, { P::Comp1On, P::Comp2On }, P::Press, "Press",
              { { "Both", {}, { { P::Press, "Press" } } },
                { "Fast", { P::Comp1On }, { { P::Comp1Thresh }, { P::Comp1Ratio }, { P::Comp1Attack },
                                            { P::Comp1Release }, { P::Comp1Mix }, { P::Comp1Makeup } } },
                { "Smooth", { P::Comp2On }, { { P::Comp2Thresh }, { P::Comp2Ratio }, { P::Comp2Attack },
                                              { P::Comp2Release }, { P::Comp2Mix }, { P::Comp2Makeup } } } },
              G::Dynamics },

            { "Drive", "Tube, tape, fuzz or crush distortion, blended under the clean voice. Grit scales the drive.",
              pal::pink, { P::SatOn }, P::Grit, "Grit",
              { { "Character", {}, { { P::SatMode, "Type" }, { P::Grit, "Grit" }, { P::SatDrive, "Drive" } } },
                { "Blend", {}, { { P::SatMix, "Mix" }, { P::SatTone, "Tone" }, { P::SatBits, "Crush bits" } } } },
              G::None },

            { "Chorus", "Chorus, flanger, doubler or microshift: movement and width around the voice.",
              pal::purple, { P::MotionOn }, P::MotionMix, "Mix",
              { { "Mode", {}, { { P::MotionMode, "Type" }, { P::MotionMix, "Mix" }, { P::MotionWidth, "Width" } } },
                { "Movement", {}, { { P::MotionRate, "Rate" }, { P::MotionDepth, "Depth" },
                                    { P::MotionFeedback, "Feedback" } } } },
              G::None },

            { "Delay", "Free or tempo-synced echoes, ducked so the repeats sit between phrases. "
                       "Hold THROW for a send spike.",
              pal::brand, { P::DelayOn }, P::DelayMix, "Send",
              { { "Time", {}, { { P::DelaySync, "Sync" }, { P::DelayTime, "Free time" },
                                { P::DelayFeedback, "Feedback" }, { P::DelayPingPong, "Ping-pong" } } },
                { "Send", {}, { { P::DelayMix, "Send" }, { P::DelayDuck, "Ducking" }, { P::DelayTone, "Tone" } } } },
              G::Space },

            { "Reverb", "Eight-line room with pre-delay, damping and shimmer. Space sets how much reverb "
                        "and delay surround the voice.",
              pal::purple, { P::VerbOn }, P::Space, "Space",
              { { "Room", {}, { { P::VerbSize, "Size" }, { P::VerbPre, "Pre-delay" }, { P::VerbDamp, "Damping" },
                                { P::VerbShimmer, "Shimmer" } } },
                { "Send", {}, { { P::Space, "Space (verb + delay)" }, { P::VerbMix, "Send" },
                                { P::VerbDuck, "Ducking" } } },
                { "Return", {}, { { P::VerbHP, "Low cut" }, { P::VerbLP, "Top cut" }, { P::VerbWidth, "Width" } } } },
              G::Space },

            { "Stereo", "Stereo separation: overall mid/side width, plus the chorus and reverb widths. "
                        "0% is mono, 100% natural, 200% wide.",
              pal::brand, {}, P::Width, "Width",
              { { "Width", {}, { { P::Width, "Overall width" }, { P::MotionWidth, "Chorus width" },
                                 { P::VerbWidth, "Reverb width" } } } },
              G::None },

            { "Output", "Output trim, then a look-ahead limiter that keeps every peak under the ceiling.",
              pal::yellow, { P::LimiterOn }, P::Level, "Level",
              { { "Trim", {}, { { P::Level, "Level" } } },
                { "Limiter", { P::LimiterOn }, { { P::Ceiling, "Ceiling" }, { P::LimiterRelease, "Release" } } } },
              G::Dynamics }
        };
    }

    /** The Output stage's switch is the limiter: trim and level never dim. */
    bool followsStageSwitch (int stage, const ChainView::Group& group)
    {
        return ! (stage == ChainView::Output && group.title == "Trim");
    }

    juce::String shortDb (float db)
    {
        return juce::String (db, 1) + " dB";
    }
}

//==============================================================================
class ChainView::Tile : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    Tile (ChainView& o, int i, const StageSpec& s, APVTS& state)
        : owner (o), index (i), spec (s), knob (s.accent)
    {
        knob.format = [p = s.knob] (float v) { return formatParamValue (p, v); };
        knob.defaultValue = paramDef (s.knob).def;
        knob.setTooltip (juce::String (paramDef (s.knob).name) + "  (double-click to reset)");
        addAndMakeVisible (knob);
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId (s.knob), knob);
        knob.addMouseListener (this, false);

        if (! s.power.empty())
        {
            power = std::make_unique<PowerSwitch> (state, s.power, s.accent);
            addAndMakeVisible (*power);
        }

        setTooltip ("Click to see every " + s.name + " control");
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    ~Tile() override
    {
        knob.removeMouseListener (this);
    }

    bool isActive() const { return power == nullptr || power->isOn(); }

    void update (bool isSelected, const juce::String& newStatus)
    {
        if (power != nullptr)
            power->refresh();

        const bool on = isActive();
        knob.setDimmed (! on && index != ChainView::Output);

        if (isSelected != selected || newStatus != status || on != active)
        {
            selected = isSelected;
            status = newStatus;
            active = on;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        owner.selectModule (index);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.75f);

        g.setColour (selected ? pal::panelHi : pal::panel);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (selected ? spec.accent.withAlpha (0.9f) : pal::stroke);
        g.drawRoundedRectangle (r, 12.0f, selected ? 1.5f : 1.0f);

        g.setColour (active ? pal::text : pal::dim);
        g.setFont (uiFont (10.5f, true));
        g.drawText (spec.name.toUpperCase(), 10, 8, getWidth() - 14 - (power != nullptr ? 36 : 0), 20,
                    juce::Justification::centredLeft, true);

        g.setColour (pal::dim);
        g.setFont (uiFont (8.5f, true));
        g.drawText (spec.knobLabel.toUpperCase(), knobLabelArea, juce::Justification::centred, false);

        g.setColour (active ? pal::dim.brighter (0.25f) : pal::dim.withAlpha (0.6f));
        g.setFont (uiFont (8.5f));
        g.drawText (active ? status : juce::String ("off"), statusArea, juce::Justification::centred, true);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 6);
        auto top = r.removeFromTop (24);

        if (power != nullptr)
            power->setBounds (top.removeFromRight (30).withSizeKeepingCentre (30, 16));

        statusArea = r.removeFromBottom (14);
        knobLabelArea = r.removeFromBottom (14);
        r.removeFromBottom (2);

        const int d = juce::jmin (r.getWidth(), r.getHeight(), 92);
        knob.setBounds (r.withSizeKeepingCentre (d, d));
    }

private:
    ChainView& owner;
    int index;
    const StageSpec& spec;
    KnobSlider knob;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
    std::unique_ptr<PowerSwitch> power;
    juce::Rectangle<int> knobLabelArea, statusArea;
    juce::String status;
    bool selected = false;
    bool active = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tile)
};

//==============================================================================
class ChainView::GroupPanel : public juce::Component
{
public:
    GroupPanel (const Group& g, bool followsStage, juce::Colour accentColour, APVTS& state)
        : title (g.title), accent (accentColour), follows (followsStage)
    {
        if (! g.power.empty())
        {
            power = std::make_unique<PowerSwitch> (state, g.power, accent);
            addAndMakeVisible (*power);
        }

        for (const auto& c : g.controls)
        {
            auto control = makeControl (c.param, state, accent, c.label);
            addAndMakeVisible (*control);
            controls.push_back (std::move (control));
        }
    }

    static constexpr int kHeader = 26;
    static constexpr int kGap = 8;

    int preferredHeight() const
    {
        return kHeader + (int) controls.size() * (LabelledControl::kPreferredHeight + kGap);
    }

    void update (const ChainParams& p, bool stageOn)
    {
        if (power != nullptr)
            power->refresh();

        const bool on = (stageOn || ! follows) && (power == nullptr || power->isOn());

        for (auto& c : controls)
        {
            // Off stages and controls that do nothing in the current mode fade
            // back but stay editable, so a preset can be set up before it is used.
            const float alpha = ! on ? 0.45f : (ChainView::isRelevant (c->getParam(), p) ? 1.0f : 0.35f);

            if (std::abs (c->getAlpha() - alpha) > 0.01f)
                c->setAlpha (alpha);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto header = getLocalBounds().removeFromTop (kHeader - 6);

        g.setColour (pal::dim);
        g.setFont (uiFont (9.5f, true));
        g.drawText (title.toUpperCase(), header, juce::Justification::centredLeft, false);

        g.setColour (pal::stroke.withAlpha (0.7f));
        g.drawHorizontalLine (kHeader - 4, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto header = r.removeFromTop (kHeader);

        if (power != nullptr)
            power->setBounds (header.removeFromTop (kHeader - 6).removeFromRight (36).withSizeKeepingCentre (36, 17));

        const int h = LabelledControl::kPreferredHeight;

        for (auto& c : controls)
        {
            c->setBounds (r.removeFromTop (h));
            r.removeFromTop (kGap);
        }
    }

private:
    juce::String title;
    juce::Colour accent;
    bool follows = true;
    std::unique_ptr<PowerSwitch> power;
    std::vector<std::unique_ptr<LabelledControl>> controls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroupPanel)
};

//==============================================================================
ChainView::ChainView (VoxlineProcessor& p)
    : processor (p), stages (buildStages())
{
    jassert ((int) stages.size() == NumStages);

    for (int i = 0; i < (int) stages.size(); ++i)
    {
        auto* tile = new Tile (*this, i, stages[(size_t) i], processor.apvts);
        addAndMakeVisible (tile);
        tiles.add (tile);
    }

    addChildComponent (eqGraph);
    addChildComponent (spaceGraph);
    addChildComponent (dynamicsGraph);

    selectModule (0);
}

ChainView::~ChainView() = default;

juce::String ChainView::stageName (int index)
{
    static const auto names = []
    {
        juce::StringArray out;
        for (const auto& s : buildStages())
            out.add (s.name);
        return out;
    }();

    return names[index];
}

std::vector<Param> ChainView::reachableParams()
{
    std::vector<Param> out;

    for (const auto& s : buildStages())
    {
        out.insert (out.end(), s.power.begin(), s.power.end());
        out.push_back (s.knob);

        for (const auto& g : s.groups)
        {
            out.insert (out.end(), g.power.begin(), g.power.end());

            for (const auto& c : g.controls)
                out.push_back (c.param);
        }
    }

    return out;
}

bool ChainView::isRelevant (Param param, const ChainParams& p)
{
    if (param == P::SatBits)
        return p.sat.mode == SatCrush;

    if (param == P::MotionRate)
        return p.motion.mode != MotionMicroshift;

    if (param == P::MotionDepth || param == P::MotionFeedback)
        return p.motion.mode == MotionChorus || p.motion.mode == MotionFlanger;

    if (param == P::DelayTime)
        return p.delay.sync == SyncFree;

    if (param == P::MotionWidth)
        return p.motion.on;

    if (param == P::VerbWidth)
        return p.verb.on;

    if (param == P::TuneSpeed || param == P::TuneKey || param == P::TuneScale)
        return p.tune.amount > 0.0f;

    return true;
}

void ChainView::selectModule (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) stages.size()))
        return;

    selected = index;
    rebuildDetail();
    resized();
    refresh();
    repaint();
}

void ChainView::rebuildDetail()
{
    groups.clear();

    const auto& stage = stages[(size_t) selected];

    for (const auto& g : stage.groups)
    {
        auto* panel = new GroupPanel (g, followsStageSwitch (selected, g), stage.accent, processor.apvts);
        addAndMakeVisible (panel);
        groups.add (panel);
    }

    eqGraph.setVisible (stage.graph == Graph::Eq);
    spaceGraph.setVisible (stage.graph == Graph::Space);
    dynamicsGraph.setVisible (stage.graph == Graph::Dynamics);
}

juce::String ChainView::statusFor (int stage, const ChainParams& p) const
{
    switch (stage)
    {
        case Pitch:
        {
            auto s = paramChoices (P::TuneKey)[p.tune.key] + " " + paramChoices (P::TuneScale)[p.tune.scale].toLowerCase();
            const float cents = processor.tuneCents();
            if (std::abs (cents) >= 1.0f)
                s += "  " + juce::String (cents > 0.0f ? "+" : "") + juce::String (juce::roundToInt (cents)) + " ct";
            return s;
        }
        case Eq:     return "low cut " + formatParamValue (P::Hpf, p.eq.hpf);
        case DeEss:  return "above " + formatParamValue (P::DeessFreq, p.deess.freq);
        case Comp:
        {
            const float gr = std::max (processor.comp1Gr(), processor.comp2Gr());
            return gr > 0.1f ? "GR -" + shortDb (gr)
                             : juce::String (p.comp1.on && p.comp2.on ? "fast + smooth" : p.comp1.on ? "fast" : "smooth");
        }
        case Drive:  return paramChoices (P::SatMode)[p.sat.mode].toLowerCase();
        case Chorus: return paramChoices (P::MotionMode)[p.motion.mode].toLowerCase();
        case Delay:
        {
            auto s = p.delay.sync != SyncFree ? paramChoices (P::DelaySync)[p.delay.sync]
                                              : formatParamValue (P::DelayTime, p.delay.timeMs);
            return p.delay.pingPong ? s + " ping-pong" : s;
        }
        case Reverb: return "RT60 " + juce::String (SpaceGraph::rt60Seconds (p.verb.size), 1) + " s";
        case Stereo:
        {
            const float w = p.width;
            return w < 0.05f ? "mono" : w < 0.95f ? "narrow" : w <= 1.05f ? "natural" : "wide";
        }
        case Output:
        {
            const float gr = processor.limiterGr();
            if (! p.limiterOn)
                return "limiter off";
            return gr > 0.1f ? "limit -" + shortDb (gr) : "ceiling " + shortDb (p.ceilingDb);
        }
        default: return {};
    }
}

void ChainView::refresh()
{
    const auto p = processor.getParamsForUI();

    for (int i = 0; i < tiles.size(); ++i)
        tiles[i]->update (i == selected, statusFor (i, p));

    const bool stageOn = tiles[selected]->isActive();

    for (auto* g : groups)
        g->update (p, stageOn);

    if (eqGraph.isVisible())
        eqGraph.setParams (p);

    if (spaceGraph.isVisible())
        spaceGraph.setParams (p);

    if (dynamicsGraph.isVisible())
        dynamicsGraph.setValues (p, processor.comp1Gr(), processor.comp2Gr(), processor.limiterGr());
}

void ChainView::paint (juce::Graphics& g)
{
    // Signal flow between the tiles.
    for (int i = 0; i + 1 < tiles.size(); ++i)
    {
        const auto a = tiles[i]->getBounds();
        const float x = (float) (a.getRight() + tiles[i + 1]->getX()) * 0.5f;
        const float y = (float) a.getCentreY();

        juce::Path chevron;
        chevron.startNewSubPath (x - 1.5f, y - 3.5f);
        chevron.lineTo (x + 1.5f, y);
        chevron.lineTo (x - 1.5f, y + 3.5f);
        g.setColour (pal::dim.withAlpha (0.6f));
        g.strokePath (chevron, juce::PathStrokeType (1.3f));
    }

    if (detailArea.isEmpty())
        return;

    drawPanel (g, detailArea.toFloat(), 14.0f, pal::panel);

    const auto& stage = stages[(size_t) selected];

    g.setColour (stage.accent);
    g.fillEllipse ((float) detailHeader.getX(), (float) detailHeader.getY() + 9.0f, 6.0f, 6.0f);

    g.setColour (pal::text);
    g.setFont (uiFont (16.0f, true));
    g.drawText (stage.name.toUpperCase(), detailHeader.getX() + 14, detailHeader.getY(),
                140, 24, juce::Justification::centredLeft, false);

    g.setColour (pal::dim);
    g.setFont (uiFont (10.5f));
    g.drawFittedText (stage.blurb, detailHeader.withTrimmedLeft (14).withTrimmedTop (24),
                      juce::Justification::topLeft, 1, 0.9f);

    g.setColour (pal::dim.withAlpha (0.55f));
    g.setFont (uiFont (8.0f));
    g.drawText (juce::String ("drag to adjust") + sep() + "double-click to reset",
                detailHeader.withHeight (24), juce::Justification::centredRight, false);
}

void ChainView::resized()
{
    auto r = getLocalBounds();

    tileRow = r.removeFromTop (juce::jlimit (150, 180, getHeight() / 4));
    r.removeFromTop (12);
    detailArea = r;

    {
        const int gap = 10;
        const int n = tiles.size();
        const int w = (tileRow.getWidth() - gap * (n - 1)) / juce::jmax (1, n);
        auto row = tileRow;

        for (auto* tile : tiles)
        {
            tile->setBounds (row.removeFromLeft (w));
            row.removeFromLeft (gap);
        }
    }

    auto inner = detailArea.reduced (20, 16);
    detailHeader = inner.removeFromTop (44);
    inner.removeFromTop (14);

    const auto& stage = stages[(size_t) selected];

    if (stage.graph != Graph::None)
    {
        const int graphW = juce::jlimit (320, 520, (int) (inner.getWidth() * 0.4f));
        auto graphArea = inner.removeFromRight (graphW);
        inner.removeFromRight (24);

        eqGraph.setBounds (graphArea);
        spaceGraph.setBounds (graphArea);
        dynamicsGraph.setBounds (graphArea);
    }

    const int n = groups.size();

    if (n == 0)
        return;

    const int gap = 22;
    const int colW = juce::jmin (280, (inner.getWidth() - gap * (n - 1)) / n);

    for (auto* group : groups)
    {
        group->setBounds (inner.removeFromLeft (colW).withHeight (juce::jmin (inner.getHeight(), group->preferredHeight())));
        inner.removeFromLeft (gap);
    }
}

} // namespace vx::ui

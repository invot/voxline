#include "ShapePage.h"
#include "../PluginProcessor.h"
#include "../core/Modules.h"

#include <cmath>

namespace vx::ui
{

namespace
{
    juce::String signedDb (float db, int decimals = 1)
    {
        return (db >= 0.0f ? "+" : "") + juce::String (db, decimals) + " dB";
    }
}

//==============================================================================
class ShapePage::EqGraph : public juce::Component
{
public:
    explicit EqGraph (std::function<ChainParams()> source)
        : params (std::move (source))
    {
        eq.prepare (48000.0);
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto p = params();
        eq.setParams (p.eq, p.tone);

        auto r = getLocalBounds().toFloat().reduced (0.5f);
        drawPanel (g, r, 12.0f, pal::panel);

        g.setColour (pal::teal);
        g.fillEllipse (14.0f, 14.0f, 5.0f, 5.0f);

        g.setColour (pal::text);
        g.setFont (uiFont (11.0f, true));
        g.drawText ("TONE", 26, 9, 100, 16, juce::Justification::centredLeft, false);

        g.setColour (pal::dim);
        g.setFont (uiFont (8.0f));
        g.drawText ("EQ RESPONSE", 26, 24, 120, 11, juce::Justification::centredLeft, false);

        g.setFont (uiFont (8.5f));
        const auto summary = p.eq.on
            ? "HPF " + juce::String (p.eq.hpf, 0) + " Hz   AIR " + signedDb (p.eq.airGain)
            : juce::String ("EQ OFF");
        g.setColour (p.eq.on ? pal::dim : pal::pink);
        g.drawText (summary, getWidth() - 240, 14, 226, 12,
                    juce::Justification::centredRight, false);

        auto plot = r.reduced (14.0f, 0.0f).withTrimmedTop (40.0f).withTrimmedBottom (22.0f);

        g.setColour (pal::bg.withAlpha (0.55f));
        g.fillRoundedRectangle (plot, 6.0f);

        const auto xFor = [&plot] (float freq)
        {
            const float t = std::log10 (juce::jlimit (20.0f, 20000.0f, freq) / 20.0f) / 3.0f;
            return plot.getX() + t * plot.getWidth();
        };

        const auto yFor = [&plot] (float db)
        {
            return plot.getCentreY() - (juce::jlimit (-18.0f, 18.0f, db) / 18.0f) * (plot.getHeight() * 0.5f - 2.0f);
        };

        // frequency grid
        const float freqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
        const char* labels[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };

        g.setFont (uiFont (7.5f));

        for (int i = 0; i < 8; ++i)
        {
            const float x = xFor (freqs[i]);
            g.setColour (pal::stroke.withAlpha (0.55f));
            g.drawVerticalLine ((int) std::round (x), plot.getY() + 3.0f, plot.getBottom() - 3.0f);

            g.setColour (pal::dim.withAlpha (0.65f));
            g.drawText (labels[i], juce::Rectangle<float> (x - 14.0f, plot.getBottom() + 2.0f, 28.0f, 10.0f),
                        juce::Justification::centred, false);
        }

        for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
        {
            const float y = yFor (db);
            g.setColour (db == 0.0f ? pal::stroke : pal::stroke.withAlpha (0.35f));
            g.drawHorizontalLine ((int) std::round (y), plot.getX() + 3.0f, plot.getRight() - 3.0f);
        }

        g.setColour (pal::dim.withAlpha (0.6f));
        g.drawText ("+12", plot.getX() + 3.0f, yFor (12.0f) - 1.0f, 22.0f, 9.0f,
                    juce::Justification::centredLeft, false);
        g.drawText ("0", plot.getX() + 3.0f, yFor (0.0f) - 1.0f, 22.0f, 9.0f,
                    juce::Justification::centredLeft, false);
        g.drawText ("-12", plot.getX() + 3.0f, yFor (-12.0f) - 1.0f, 22.0f, 9.0f,
                    juce::Justification::centredLeft, false);

        juce::Path curve, fill;
        bool first = true;

        for (int i = 0; i <= 180; ++i)
        {
            const float t = (float) i / 180.0f;
            const float freq = 20.0f * std::pow (1000.0f, t);
            const float db = (float) eq.responseDb ((double) freq);
            const float x = xFor (freq);
            const float y = yFor (db);

            if (first)
            {
                curve.startNewSubPath (x, y);
                fill.startNewSubPath (x, plot.getBottom() - 2.0f);
                fill.lineTo (x, y);
                first = false;
            }
            else
            {
                curve.lineTo (x, y);
                fill.lineTo (x, y);
            }
        }

        fill.lineTo (plot.getRight() - 2.0f, plot.getBottom() - 2.0f);
        fill.closeSubPath();

        const auto accent = p.eq.on ? pal::teal : pal::teal.withAlpha (0.35f);

        juce::ColourGradient grad (accent.withAlpha (p.eq.on ? 0.22f : 0.08f), plot.getX(), plot.getY(),
                                   accent.withAlpha (0.0f), plot.getX(), plot.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (fill);

        g.setColour (accent);
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));

        if (p.eq.on)
        {
            const float bands[] = { p.eq.lowFreq, p.eq.bodyFreq, p.eq.presFreq, p.eq.airFreq };

            for (float f : bands)
            {
                const float x = xFor (f);
                const float y = yFor ((float) eq.responseDb ((double) f));
                g.setColour (pal::panel);
                g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
                g.setColour (pal::teal);
                g.drawEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f, 1.2f);
            }
        }
    }

private:
    EqModule eq;
    std::function<ChainParams()> params;
};

//==============================================================================
class ShapePage::SpaceGraph : public juce::Component
{
public:
    explicit SpaceGraph (std::function<ChainParams()> source)
        : params (std::move (source))
    {
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto p = params();

        auto r = getLocalBounds().toFloat().reduced (0.5f);
        drawPanel (g, r, 12.0f, pal::panel);

        const float tail = 0.35f + 7.0f * std::pow (juce::jlimit (0.0f, 1.0f, p.verb.size), 2.0f);

        g.setColour (pal::purple);
        g.fillEllipse (14.0f, 14.0f, 5.0f, 5.0f);

        g.setColour (pal::text);
        g.setFont (uiFont (11.0f, true));
        g.drawText ("SPACE", 26, 9, 100, 16, juce::Justification::centredLeft, false);

        g.setColour (pal::dim);
        g.setFont (uiFont (8.0f));
        g.drawText ("TAIL + ECHO", 26, 24, 120, 11, juce::Justification::centredLeft, false);

        juce::String summary;

        if (p.verb.on)
        {
            summary = juce::String (tail, 1) + " s  |  " + juce::String (p.verb.mix * 100.0f, 0) + "% wet";
            if (p.verb.shimmer > 0.01f)
                summary += "  |  SHIMMER";
        }
        else
        {
            summary = "REVERB OFF";
        }

        g.setColour (p.verb.on ? pal::dim : pal::pink);
        g.setFont (uiFont (8.5f));
        g.drawText (summary, getWidth() - 250, 14, 236, 12,
                    juce::Justification::centredRight, false);

        auto plot = r.reduced (14.0f, 0.0f).withTrimmedTop (40.0f).withTrimmedBottom (22.0f);

        g.setColour (pal::bg.withAlpha (0.55f));
        g.fillRoundedRectangle (plot, 6.0f);

        const float window = 4.0f;
        const auto xFor = [&plot, window] (float seconds)
        {
            return plot.getX() + juce::jlimit (0.0f, window, seconds) / window * plot.getWidth();
        };

        for (int second = 1; second <= 3; ++second)
        {
            const float x = xFor ((float) second);
            g.setColour (pal::stroke.withAlpha (0.5f));
            g.drawVerticalLine ((int) std::round (x), plot.getY() + 3.0f, plot.getBottom() - 3.0f);
        }

        g.setColour (pal::dim.withAlpha (0.6f));
        g.setFont (uiFont (7.5f));
        g.drawText ("0", plot.getX() + 3.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f,
                    juce::Justification::centredLeft, false);
        g.drawText ("2 s", xFor (2.0f) - 10.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f,
                    juce::Justification::centred, false);
        g.drawText ("4 s", plot.getRight() - 22.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f,
                    juce::Justification::centredRight, false);

        if (p.delay.on)
        {
            const float beatMs = 60000.0f / juce::jmax (40.0f, p.bpm);
            const float delayMs = p.delay.sync > 0 ? beatMs * syncBeats (p.delay.sync) : p.delay.timeMs;

            for (int tap = 1; tap <= 3; ++tap)
            {
                const float seconds = (delayMs / 1000.0f) * (float) tap;

                if (seconds > window || seconds < 0.01f)
                    continue;

                const float x = xFor (seconds);
                g.setColour (pal::teal.withAlpha (0.26f / (float) tap));
                g.drawVerticalLine ((int) std::round (x), plot.getY() + 4.0f, plot.getBottom() - 4.0f);
                g.fillEllipse (x - 2.0f, plot.getY() + 3.0f, 4.0f, 4.0f);
            }
        }

        juce::Path curve, fill;
        const auto accent = p.verb.on ? pal::purple : pal::purple.withAlpha (0.35f);

        for (int i = 0; i <= 160; ++i)
        {
            const float t = window * (float) i / 160.0f;
            const float amp = p.verb.on
                ? std::exp (-3.0f * t / tail) * (0.30f + 0.70f * p.verb.mix)
                : 0.015f;
            const float x = xFor (t);
            const float y = plot.getBottom() - 4.0f - juce::jlimit (0.0f, 1.0f, amp) * (plot.getHeight() - 12.0f);

            if (i == 0)
            {
                curve.startNewSubPath (x, y);
                fill.startNewSubPath (x, plot.getBottom() - 3.0f);
                fill.lineTo (x, y);
            }
            else
            {
                curve.lineTo (x, y);
                fill.lineTo (x, y);
            }
        }

        fill.lineTo (plot.getRight() - 3.0f, plot.getBottom() - 3.0f);
        fill.closeSubPath();

        juce::ColourGradient grad (accent.withAlpha (p.verb.on ? 0.22f : 0.06f), plot.getX(), plot.getY(),
                                   accent.withAlpha (0.0f), plot.getX(), plot.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (fill);

        g.setColour (accent);
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
    }

private:
    std::function<ChainParams()> params;
};

//==============================================================================
ShapePage::ShapePage (VoxlineProcessor& p)
    : processor (p),
      eqGraph (std::make_unique<EqGraph> ([this] { return processor.getParamsForUI(); })),
      spaceGraph (std::make_unique<SpaceGraph> ([this] { return processor.getParamsForUI(); }))
{
    addAndMakeVisible (rail);
    addAndMakeVisible (*eqGraph);
    addAndMakeVisible (*spaceGraph);
    addAndMakeVisible (path);

    static const Param macroParams[] = { Param::Press, Param::Tone, Param::Tune, Param::Grit,
                                         Param::Width, Param::Space, Param::Level };
    static const juce::Colour macroColours[] = { pal::teal, pal::purple, pal::pink, pal::amber,
                                                 pal::teal, pal::purple, pal::yellow };

    for (int i = 0; i < 7; ++i)
    {
        auto* knob = new MacroKnob (macroParams[i], processor.apvts, macroColours[i]);
        addAndMakeVisible (knob);
        macros.add (knob);
    }

    rail.onSelect = [this] (int index)
    {
        if (onSelectPreset != nullptr)
            onSelectPreset (index);
    };

    path.onOpen = [this] (int index)
    {
        if (onOpenModule != nullptr)
            onOpenModule (index);
    };
}

ShapePage::~ShapePage() = default;

void ShapePage::setSelectedPreset (int index)
{
    rail.setSelected (index);
}

void ShapePage::refresh()
{
    const auto p = processor.getParamsForUI();

    path.setActive (0, p.tune.on);
    path.setActive (1, p.eq.on);
    path.setActive (2, p.deess.on);
    path.setActive (3, p.comp1.on || p.comp2.on);
    path.setActive (4, p.sat.on);
    path.setActive (5, p.motion.on);
    path.setActive (6, p.delay.on);
    path.setActive (7, p.verb.on);
    path.setActive (8, true);
    path.setActive (9, p.limiterOn);

    eqGraph->repaint();
    spaceGraph->repaint();
}

void ShapePage::paint (juce::Graphics& g)
{
    drawPanel (g, macroPanel.toFloat(), 14.0f, pal::panel);

    g.setColour (pal::teal);
    g.fillEllipse ((float) macroPanel.getX() + 16.0f, (float) macroPanel.getY() + 15.0f, 5.0f, 5.0f);

    g.setColour (pal::text);
    g.setFont (uiFont (11.5f, true));
    g.drawText ("SHAPE", macroPanel.getX() + 28, macroPanel.getY() + 8, 120, 18,
                juce::Justification::centredLeft, false);

    g.setColour (pal::dim);
    g.setFont (uiFont (9.0f));
    g.drawText ("the fast controls", macroPanel.getX() + 16, macroPanel.getY() + 25, 160, 14,
                juce::Justification::centredLeft, false);

    g.setColour (pal::dim.withAlpha (0.65f));
    g.setFont (uiFont (8.5f));
    g.drawText ("every control lives in Detail", macroPanel.getRight() - 226,
                macroPanel.getY() + 13, 210, 14,
                juce::Justification::centredRight, false);
}

void ShapePage::resized()
{
    auto r = getLocalBounds();
    const int railWidth = juce::jlimit (220, 260, juce::roundToInt ((float) r.getWidth() * 0.21f));

    rail.setBounds (r.removeFromLeft (railWidth));
    r.removeFromLeft (14);

    macroPanel = r.removeFromTop (190);

    auto knobsArea = macroPanel.reduced (12, 0).withTrimmedTop (42).withTrimmedBottom (8);
    const int gap = 6;
    const int knobWidth = (knobsArea.getWidth() - gap * 6) / 7;

    for (auto* knob : macros)
    {
        knob->setBounds (knobsArea.removeFromLeft (knobWidth));
        knobsArea.removeFromLeft (gap);
    }

    r.removeFromTop (12);

    const int graphHeight = juce::jlimit (200, 330, r.getHeight() - 90 - 12);
    auto graphs = r.removeFromTop (graphHeight);

    const int eqWidth = juce::roundToInt ((float) graphs.getWidth() * 0.58f);
    eqGraph->setBounds (graphs.removeFromLeft (eqWidth));
    graphs.removeFromLeft (12);
    spaceGraph->setBounds (graphs);

    r.removeFromTop (12);
    path.setBounds (r.removeFromTop (juce::jmin (90, r.getHeight())));
}

} // namespace vx::ui

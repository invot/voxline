#include "Graphs.h"
#include "../Parameters.h"

#include <cmath>

namespace vx::ui
{

namespace
{
    juce::String signedDb (float db, int decimals = 1)
    {
        return (db >= 0.0f ? "+" : "") + juce::String (db, decimals) + " dB";
    }

    juce::Rectangle<float> plotArea (juce::Rectangle<float> r)
    {
        return r.reduced (14.0f, 0.0f).withTrimmedTop (34.0f).withTrimmedBottom (20.0f);
    }

    void drawGraphFrame (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent,
                         const juce::String& title, const juce::String& summary, bool active)
    {
        g.setColour (pal::bg.withAlpha (0.45f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (pal::stroke.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

        g.setColour (accent);
        g.fillEllipse (r.getX() + 14.0f, r.getY() + 14.0f, 5.0f, 5.0f);

        g.setColour (pal::text);
        g.setFont (uiFont (10.5f, true));
        g.drawText (title, (int) r.getX() + 26, (int) r.getY() + 8, 160, 16,
                    juce::Justification::centredLeft, false);

        g.setColour (active ? pal::dim : pal::pink);
        g.setFont (uiFont (9.0f));
        g.drawText (summary, (int) r.getRight() - 260, (int) r.getY() + 8, 246, 16,
                    juce::Justification::centredRight, false);

        g.setColour (pal::bg.withAlpha (0.55f));
        g.fillRoundedRectangle (plotArea (r), 6.0f);
    }
}

//==============================================================================
EqGraph::EqGraph()
{
    eq.prepare (48000.0);
    setInterceptsMouseClicks (false, false);
}

void EqGraph::setParams (const ChainParams& p)
{
    params = p;
    repaint();
}

void EqGraph::paint (juce::Graphics& g)
{
    const auto& p = params;
    eq.setParams (p.eq, p.tone);

    auto r = getLocalBounds().toFloat();
    const auto summary = p.eq.on
        ? "low cut " + juce::String (juce::roundToInt (p.eq.hpf)) + " Hz" + sep() + "air " + signedDb (p.eq.airGain)
        : juce::String ("EQ OFF");

    drawGraphFrame (g, r, pal::brand, "EQ RESPONSE", summary, p.eq.on);

    const auto plot = plotArea (r);

    const auto xFor = [&plot] (float freq)
    {
        const float t = std::log10 (juce::jlimit (20.0f, 20000.0f, freq) / 20.0f) / 3.0f;
        return plot.getX() + t * plot.getWidth();
    };

    const auto yFor = [&plot] (float db)
    {
        return plot.getCentreY() - (juce::jlimit (-18.0f, 18.0f, db) / 18.0f) * (plot.getHeight() * 0.5f - 2.0f);
    };

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
        g.setColour (std::abs (db) < 0.01f ? pal::stroke : pal::stroke.withAlpha (0.35f));
        g.drawHorizontalLine ((int) std::round (y), plot.getX() + 3.0f, plot.getRight() - 3.0f);
    }

    g.setColour (pal::dim.withAlpha (0.6f));
    g.drawText ("+12", plot.getX() + 3.0f, yFor (12.0f) - 1.0f, 22.0f, 9.0f, juce::Justification::centredLeft, false);
    g.drawText ("0", plot.getX() + 3.0f, yFor (0.0f) - 1.0f, 22.0f, 9.0f, juce::Justification::centredLeft, false);
    g.drawText ("-12", plot.getX() + 3.0f, yFor (-12.0f) - 1.0f, 22.0f, 9.0f, juce::Justification::centredLeft, false);

    juce::Path curve, fill;

    for (int i = 0; i <= 180; ++i)
    {
        const float t = (float) i / 180.0f;
        const float freq = 20.0f * std::pow (1000.0f, t);
        const float db = p.eq.on ? (float) eq.responseDb ((double) freq) : 0.0f;
        const float x = xFor (freq);
        const float y = yFor (db);

        if (i == 0)
        {
            curve.startNewSubPath (x, y);
            fill.startNewSubPath (x, plot.getBottom() - 2.0f);
            fill.lineTo (x, y);
        }
        else
        {
            curve.lineTo (x, y);
            fill.lineTo (x, y);
        }
    }

    fill.lineTo (plot.getRight() - 2.0f, plot.getBottom() - 2.0f);
    fill.closeSubPath();

    const auto accent = p.eq.on ? pal::brand : pal::brand.withAlpha (0.35f);

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
            g.setColour (pal::brand);
            g.drawEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f, 1.2f);
        }
    }

    // Where the de-esser listens, so the two can be set against each other.
    if (p.deess.on)
    {
        const float x = xFor (p.deess.freq);
        g.setColour (pal::pink.withAlpha (0.55f));
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine ({ x, plot.getY() + 4.0f, x, plot.getBottom() - 4.0f }, dashes, 2, 1.0f);
        g.setFont (uiFont (7.5f, true));
        g.drawText ("S", juce::Rectangle<float> (x + 2.0f, plot.getY() + 3.0f, 10.0f, 10.0f),
                    juce::Justification::centredLeft, false);
    }
}

//==============================================================================
SpaceGraph::SpaceGraph()
{
    setInterceptsMouseClicks (false, false);
}

float SpaceGraph::rt60Seconds (float size)
{
    // Same curve as ReverbModule::updateSize.
    return 0.45f + 7.0f * std::pow (juce::jlimit (0.0f, 1.0f, size), 2.2f);
}

void SpaceGraph::setParams (const ChainParams& p)
{
    params = p;
    repaint();
}

void SpaceGraph::paint (juce::Graphics& g)
{
    const auto& p = params;
    const float tail = rt60Seconds (p.verb.size);
    const float spaceScale = 0.35f + 1.3f * juce::jlimit (0.0f, 1.0f, p.space);
    const float verbSend = juce::jlimit (0.0f, 1.0f, p.verb.mix * spaceScale);
    const float delaySend = juce::jlimit (0.0f, 1.0f, p.delay.mix * spaceScale);

    juce::String summary;

    if (p.verb.on)
    {
        summary = "RT60 " + juce::String (tail, 1) + " s" + sep() + juce::String (juce::roundToInt (verbSend * 100.0f)) + "% send";
        if (p.verb.shimmer > 0.01f)
            summary += sep() + "shimmer";
    }
    else
    {
        summary = "REVERB OFF";
    }

    auto r = getLocalBounds().toFloat();
    drawGraphFrame (g, r, pal::purple, "TAIL + ECHO", summary, p.verb.on);

    const auto plot = plotArea (r);
    const float window = 4.0f;

    const auto xFor = [&plot, window] (float seconds)
    {
        return plot.getX() + juce::jlimit (0.0f, window, seconds) / window * plot.getWidth();
    };

    for (int second = 1; second <= 3; ++second)
    {
        g.setColour (pal::stroke.withAlpha (0.5f));
        g.drawVerticalLine ((int) std::round (xFor ((float) second)), plot.getY() + 3.0f, plot.getBottom() - 3.0f);
    }

    g.setColour (pal::dim.withAlpha (0.6f));
    g.setFont (uiFont (7.5f));
    g.drawText ("0", plot.getX() + 3.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f, juce::Justification::centredLeft, false);
    g.drawText ("2 s", xFor (2.0f) - 10.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f, juce::Justification::centred, false);
    g.drawText ("4 s", plot.getRight() - 22.0f, plot.getBottom() + 2.0f, 20.0f, 10.0f, juce::Justification::centredRight, false);

    // Delay taps, fading with the feedback.
    if (p.delay.on)
    {
        const float beatMs = 60000.0f / juce::jmax (20.0f, p.bpm);
        const float delayMs = p.delay.sync != SyncFree ? beatMs * syncBeats (p.delay.sync) : p.delay.timeMs;
        float level = 0.25f + 0.75f * delaySend;

        for (int tap = 1; tap <= 12 && level > 0.03f; ++tap)
        {
            const float seconds = (delayMs / 1000.0f) * (float) tap;

            if (seconds > window)
                break;

            const float x = xFor (seconds);
            const float h = (plot.getHeight() - 12.0f) * juce::jlimit (0.0f, 1.0f, level);
            g.setColour (pal::brand.withAlpha (0.65f));
            g.drawLine (x, plot.getBottom() - 4.0f, x, plot.getBottom() - 4.0f - h, 1.6f);
            g.fillEllipse (x - 2.0f, plot.getBottom() - 6.0f - h, 4.0f, 4.0f);

            level *= juce::jlimit (0.0f, 0.92f, p.delay.feedback);
        }
    }

    juce::Path curve, fill;
    const auto accent = p.verb.on ? pal::purple : pal::purple.withAlpha (0.35f);

    for (int i = 0; i <= 160; ++i)
    {
        const float t = window * (float) i / 160.0f;
        // -60 dB at RT60, drawn on a 60 dB scale so the line ends where the tail does.
        const float db = -60.0f * t / tail;
        const float amp = p.verb.on ? juce::jlimit (0.0f, 1.0f, (1.0f + db / 60.0f)) * (0.35f + 0.65f * verbSend)
                                    : 0.0f;
        const float x = xFor (t);
        const float y = plot.getBottom() - 4.0f - amp * (plot.getHeight() - 12.0f);

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

//==============================================================================
DynamicsGraph::DynamicsGraph()
{
    setInterceptsMouseClicks (false, false);
}

void DynamicsGraph::setValues (const ChainParams& p, float fastGrDb, float smoothGrDb, float limiterGrDb)
{
    params = p;
    const float in[3] = { fastGrDb, smoothGrDb, limiterGrDb };

    for (int i = 0; i < 3; ++i)
    {
        // Instant on the way up, ~300 ms fall at the UI rate, so peaks are readable.
        const float v = std::max (0.0f, in[i]);
        shown[i] = v > shown[i] ? v : shown[i] * 0.8f;
    }

    repaint();
}

void DynamicsGraph::paint (juce::Graphics& g)
{
    const auto& p = params;
    auto r = getLocalBounds().toFloat();

    const auto summary = "press " + juce::String (juce::roundToInt (p.press * 100.0f)) + "%" + sep() + "ceiling "
                       + signedDb (p.ceilingDb);
    drawGraphFrame (g, r, pal::amber, "GAIN REDUCTION", summary, true);

    auto plot = plotArea (r).reduced (14.0f, 10.0f);

    const struct { const char* name; bool on; float db; float range; juce::Colour colour; } rows[] =
    {
        { "FAST COMP",   p.comp1.on, shown[0], 24.0f, pal::amber },
        { "SMOOTH COMP", p.comp2.on, shown[1], 24.0f, pal::amber },
        { "LIMITER",     p.limiterOn, shown[2], 12.0f, pal::yellow }
    };

    const float rowH = plot.getHeight() / 3.0f;

    for (const auto& row : rows)
    {
        auto area = plot.removeFromTop (rowH);
        auto labelArea = area.removeFromTop (16.0f);

        g.setColour (row.on ? pal::text : pal::dim.withAlpha (0.6f));
        g.setFont (uiFont (9.0f, true));
        g.drawText (row.name, labelArea, juce::Justification::centredLeft, false);

        g.setFont (uiFont (9.0f));
        g.drawText (row.on ? (row.db > 0.05f ? "-" + juce::String (row.db, 1) + " dB" : "0.0 dB") : "off",
                    labelArea, juce::Justification::centredRight, false);

        auto track = area.withHeight (8.0f).withY (area.getY() + 4.0f);
        g.setColour (pal::stroke);
        g.fillRoundedRectangle (track, 4.0f);

        // Gain reduction grows from the right, like a hardware GR meter.
        const float t = row.on ? juce::jlimit (0.0f, 1.0f, row.db / row.range) : 0.0f;
        g.setColour (row.colour);
        g.fillRoundedRectangle (track.withLeft (track.getRight() - track.getWidth() * t), 4.0f);

        g.setColour (pal::dim.withAlpha (0.5f));
        g.setFont (uiFont (7.0f));
        g.drawText ("-" + juce::String ((int) row.range), track.translated (0.0f, 10.0f).withHeight (9.0f),
                    juce::Justification::centredLeft, false);
        g.drawText ("0", track.translated (0.0f, 10.0f).withHeight (9.0f),
                    juce::Justification::centredRight, false);
    }
}

} // namespace vx::ui

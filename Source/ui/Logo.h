#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

#include "Theme.h"

namespace vx::ui
{

/** The voxline wave, drawn from one tile rectangle. The geometry is expressed
    relative to the 30 px logo in the editor header, so the header mark and the
    standalone app icon are the same drawing at any size. */
inline juce::Path logoWavePath (juce::Rectangle<float> tile)
{
    const float scale = tile.getWidth() / 30.0f;
    juce::Path wave;

    for (int i = 0; i <= 28; ++i)
    {
        const float t = (float) i / 28.0f;
        const float x = tile.getX() + (7.0f + t * 16.0f) * scale;
        const float envelope = 0.55f + 0.45f * std::sin (t * juce::MathConstants<float>::pi);
        const float y = tile.getCentreY()
                        + std::sin (t * juce::MathConstants<float>::pi * 3.0f) * 5.5f * envelope * scale;

        if (i == 0)
            wave.startNewSubPath (x, y);
        else
            wave.lineTo (x, y);
    }

    return wave;
}

/** The rounded tile with the teal waveform. */
inline void drawLogoMark (juce::Graphics& g, juce::Rectangle<float> tile)
{
    const float scale = tile.getWidth() / 30.0f;

    g.setColour (pal::panelSoft);
    g.fillRoundedRectangle (tile, 8.0f * scale);

    g.setColour (pal::teal.withAlpha (0.85f));
    g.drawRoundedRectangle (tile, 8.0f * scale, 1.2f * scale);

    g.setColour (pal::teal);
    g.strokePath (logoWavePath (tile),
                  juce::PathStrokeType (1.6f * scale, juce::PathStrokeType::curved));
}

} // namespace vx::ui

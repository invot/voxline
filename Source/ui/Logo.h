#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vx::ui
{
// Approved ioline mark, matching Assets/icon.svg. Parsed once for all paint calls.
inline void drawLogoMark (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    static constexpr char svg[] = R"ioline(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 30 30" color="#5bdd83" role="img" aria-label="voxline"><rect x=".6" y=".6" width="28.8" height="28.8" rx="8" fill="#26262a" stroke="#5bdd83" stroke-opacity=".85" stroke-width="1.2"/><g transform="translate(0 30) scale(1 -1)"><path d="M7.00 15.00 L7.57 16.09 L8.14 17.23 L8.71 18.25 L9.29 19.00 L9.86 19.31 L10.43 19.12 L11.00 18.38 L11.57 17.15 L12.14 15.57 L12.71 13.83 L13.29 12.15 L13.86 10.75 L14.43 9.82 L15.00 9.50 L15.57 9.82 L16.14 10.75 L16.71 12.15 L17.29 13.83 L17.86 15.57 L18.43 17.15 L19.00 18.38 L19.57 19.12 L20.14 19.31 L20.71 19.00 L21.29 18.25 L21.86 17.23 L22.43 16.09 L23.00 15.00" fill="none" stroke="#5bdd83" stroke-width="1.6" stroke-linejoin="round" stroke-linecap="round"/></g></svg>)ioline";
    static const auto drawable = juce::Drawable::createFromImageData (svg, sizeof (svg) - 1);
    if (drawable != nullptr)
        drawable->drawWithin (g, bounds, juce::RectanglePlacement::stretchToFit, 1.0f);
}
} // namespace vx::ui

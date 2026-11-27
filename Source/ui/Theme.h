#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vx::ui
{

namespace pal
{
    inline const juce::Colour bg        { 0xff101012 };
    inline const juce::Colour header    { 0xff151517 };
    inline const juce::Colour panel     { 0xff1e1e21 };
    inline const juce::Colour panelSoft { 0xff26262a };
    inline const juce::Colour panelHi   { 0xff303036 };
    inline const juce::Colour stroke    { 0xff36363c };
    inline const juce::Colour text      { 0xffe8e8ec };
    inline const juce::Colour dim       { 0xff8c8c94 };
    inline const juce::Colour teal      { 0xff4fd8c8 };
    inline const juce::Colour pink      { 0xffff8a9b };
    inline const juce::Colour purple    { 0xffa99bff };
    inline const juce::Colour yellow    { 0xfff3e3a6 };
    inline const juce::Colour amber     { 0xffe9b06a };
}

inline juce::Font uiFont (float height, bool bold = false)
{
   #if defined (JUCE_MAJOR_VERSION) && JUCE_MAJOR_VERSION >= 8
    juce::Font f { juce::FontOptions{}.withHeight (height) };
   #else
    juce::Font f { height };
   #endif

    return bold ? f.boldened() : f;
}

inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r,
                       float radius = 14.0f, juce::Colour fill = pal::panel)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r, radius);
    g.setColour (pal::stroke.withAlpha (0.85f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

inline void drawCaption (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text)
{
    g.setColour (pal::dim);
    g.setFont (uiFont (10.0f, true));
    g.drawText (text.toUpperCase(), area.toFloat(), juce::Justification::centredLeft, false);
}

//==============================================================================
class VoxlineLNF : public juce::LookAndFeel_V4
{
public:
    VoxlineLNF()
    {
        setColour (juce::ResizableWindow::backgroundColourId, pal::bg);
        setColour (juce::PopupMenu::backgroundColourId, pal::panelSoft);
        setColour (juce::PopupMenu::textColourId, pal::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::purple.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::ComboBox::backgroundColourId, pal::panelSoft);
        setColour (juce::ComboBox::textColourId, pal::text);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId, pal::dim);

        setColour (juce::Label::textColourId, pal::text);
        setColour (juce::TextEditor::backgroundColourId, pal::panelSoft);
        setColour (juce::TextEditor::textColourId, pal::text);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, pal::purple);

        setColour (juce::Slider::backgroundColourId, pal::stroke);
        setColour (juce::Slider::trackColourId, pal::teal.withAlpha (0.6f));
        setColour (juce::Slider::thumbColourId, pal::teal);

        setColour (juce::TextButton::buttonColourId, pal::panelSoft);
        setColour (juce::TextButton::buttonOnColourId, pal::purple);
        setColour (juce::TextButton::textColourOffId, pal::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override        { return uiFont (12.0f); }
    juce::Font getPopupMenuFont() override                       { return uiFont (13.0f); }
    juce::Font getLabelFont (juce::Label&) override              { return uiFont (12.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (11.0f, true); }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (pal::stroke);
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) width - 14.0f;
        const float cy = (float) height * 0.5f;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.5f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (pal::dim);
        g.strokePath (arrow, juce::PathStrokeType (1.6f));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (9, 0, box.getWidth() - 26, box.getHeight());
        label.setFont (uiFont (12.0f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);

        juce::Colour c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId)
                                            : b.findColour (juce::TextButton::buttonColourId);

        if (down)
            c = c.brighter (0.16f);
        else if (highlighted)
            c = c.brighter (0.07f);

        g.setColour (c);
        g.fillRoundedRectangle (r, 7.0f);

        if (! b.getToggleState())
        {
            g.setColour (pal::stroke);
            g.drawRoundedRectangle (r, 7.0f, 1.0f);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
            return;
        }

        const float cy     = (float) y + (float) height * 0.5f;
        const float left   = (float) x + 5.0f;
        const float right  = (float) x + (float) width - 5.0f;
        const float trackH = 3.0f;

        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.fillRoundedRectangle (left, cy - trackH * 0.5f, std::max (0.0f, right - left), trackH, trackH * 0.5f);

        const float filled = juce::jlimit (0.0f, std::max (0.0f, right - left), sliderPos - left);
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRoundedRectangle (left, cy - trackH * 0.5f, filled, trackH, trackH * 0.5f);

        const float radius = 5.0f;
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        g.fillEllipse (sliderPos - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override {}
};

} // namespace vx::ui

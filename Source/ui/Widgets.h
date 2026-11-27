#pragma once

#include "../Parameters.h"
#include "../Presets.h"
#include "Theme.h"

#include <functional>
#include <memory>
#include <vector>

namespace vx::ui
{

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
/** A slider that returns to its parameter default on double-click. */
class ResetSlider : public juce::Slider
{
public:
    ResetSlider() = default;

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        setValue (defaultValue, juce::sendNotificationSync);
    }

    double defaultValue = 0.0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResetSlider)
};

//==============================================================================
/** The one big knob on each effect tile. Draws its value in the centre. */
class KnobSlider : public ResetSlider
{
public:
    explicit KnobSlider (juce::Colour accentColour)
        : accent (accentColour)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setMouseDragSensitivity (220);
        setRotaryParameters (kStart, kEnd, true);
        setWantsKeyboardFocus (false);
    }

    std::function<juce::String (float)> format;

    void setDimmed (bool shouldDim)
    {
        if (dimmed != shouldDim)
        {
            dimmed = shouldDim;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (3.0f);
        const float d = std::min (b.getWidth(), b.getHeight());
        auto circle = juce::Rectangle<float> (d, d).withCentre (b.getCentre());

        const float t     = (float) valueToProportionOfLength (getValue());
        const float angle = kStart + juce::jlimit (0.0f, 1.0f, t) * (kEnd - kStart);

        const auto arc = [&circle] (float from, float to)
        {
            juce::Path p;
            p.addCentredArc (circle.getCentreX(), circle.getCentreY(),
                             circle.getWidth() * 0.5f - 3.5f,
                             circle.getHeight() * 0.5f - 3.5f,
                             0.0f, from, to, true);
            return p;
        };

        const juce::PathStrokeType stroke (4.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded);

        g.setColour (pal::stroke);
        g.strokePath (arc (kStart, kEnd), stroke);
        g.setColour (accent.withAlpha (dimmed ? 0.3f : 0.92f));
        g.strokePath (arc (kStart, angle), stroke);

        auto inner = circle.reduced (9.0f);
        g.setColour (pal::panelHi);
        g.fillEllipse (inner);
        g.setColour (pal::stroke.withAlpha (0.8f));
        g.drawEllipse (inner, 1.0f);

        const float dir = angle - juce::MathConstants<float>::halfPi;
        const auto point = juce::Point<float> (std::cos (dir), std::sin (dir));
        const auto centre = circle.getCentre();
        g.setColour (pal::text.withAlpha (dimmed ? 0.4f : 0.9f));
        g.drawLine ({ centre + point * (circle.getWidth() * 0.19f),
                      centre + point * (circle.getWidth() * 0.34f) }, 2.0f);

        if (format != nullptr)
        {
            g.setColour (dimmed ? pal::dim : pal::text);
            g.setFont (uiFont (10.5f, true));
            g.drawText (format ((float) getValue()), inner.reduced (2.0f).toNearestInt(),
                        juce::Justification::centred, false);
        }
    }

private:
    static constexpr float kStart = 3.92699f;   // 225 degrees
    static constexpr float kEnd   = 8.63938f;   // 495 degrees

    juce::Colour accent;
    bool dimmed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobSlider)
};

//==============================================================================
/** Shared look for a labelled detail control: dot, name, value on the right. */
class LabelledControl : public juce::Component
{
public:
    LabelledControl (Param p, juce::String labelText, juce::Colour accentColour)
        : param (p),
          label (labelText.isNotEmpty() ? labelText : juce::String (paramDef (p).name)),
          accent (accentColour)
    {
    }

    Param getParam() const noexcept { return param; }

    void paint (juce::Graphics& g) override
    {
        auto top = getLocalBounds().removeFromTop (kLabelHeight);

        g.setColour (accent.withAlpha (0.9f));
        g.fillEllipse ((float) top.getX() + 1.0f, (float) top.getCentreY() - 2.0f, 4.0f, 4.0f);

        g.setColour (pal::dim);
        g.setFont (uiFont (10.5f, true));
        g.drawText (label, top.withTrimmedLeft (10), juce::Justification::centredLeft, true);

        const auto value = valueText();

        if (value.isNotEmpty())
        {
            g.setColour (pal::text);
            g.setFont (uiFont (10.5f));
            g.drawText (value, top, juce::Justification::centredRight, false);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (kLabelHeight + 1);

        if (auto* c = control())
            c->setBounds (r.reduced (0, 1));
    }

    static constexpr int kLabelHeight = 16;
    static constexpr int kPreferredHeight = 44;

protected:
    virtual juce::Component* control() = 0;
    virtual juce::String valueText() const { return {}; }

    Param param;
    juce::String label;
    juce::Colour accent;
};

//==============================================================================
class ParamSlider : public LabelledControl
{
public:
    ParamSlider (Param p, APVTS& state, juce::Colour accentColour, juce::String labelText = {})
        : LabelledControl (p, std::move (labelText), accentColour)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.75f));
        slider.setColour (juce::Slider::thumbColourId, accent);
        slider.defaultValue = paramDef (p).def;
        slider.setTooltip (paramDef (p).name + juce::String ("  (double-click to reset)"));

        addAndMakeVisible (slider);
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId (p), slider);
        slider.onValueChange = [this] { repaint(); };
    }

protected:
    juce::Component* control() override { return &slider; }
    juce::String valueText() const override { return formatParamValue (param, (float) slider.getValue()); }

private:
    ResetSlider slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSlider)
};

//==============================================================================
class ParamCombo : public LabelledControl
{
public:
    ParamCombo (Param p, APVTS& state, juce::Colour accentColour, juce::String labelText = {})
        : LabelledControl (p, std::move (labelText), accentColour)
    {
        addAndMakeVisible (box);
        box.addItemList (paramChoices (p), 1);
        box.setTooltip (paramDef (p).name);
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId (p), box);
    }

protected:
    juce::Component* control() override { return &box; }

private:
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamCombo)
};

//==============================================================================
class ParamToggle : public LabelledControl
{
public:
    ParamToggle (Param p, APVTS& state, juce::Colour accentColour, juce::String labelText = {})
        : LabelledControl (p, std::move (labelText), accentColour)
    {
        addAndMakeVisible (button);
        button.setClickingTogglesState (true);
        button.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.85f));
        button.setTooltip (paramDef (p).name);

        attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId (p), button);

        button.onStateChange = [this] { updateText(); };
        updateText();
    }

protected:
    juce::Component* control() override { return &button; }

private:
    void updateText() { button.setButtonText (button.getToggleState() ? "On" : "Off"); }

    juce::TextButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamToggle)
};

/** Makes the right detail control for a parameter's kind. */
inline std::unique_ptr<LabelledControl> makeControl (Param p, APVTS& state, juce::Colour accent,
                                                     juce::String label = {})
{
    switch (paramDef (p).kind)
    {
        case ParamKind::Choice: return std::make_unique<ParamCombo> (p, state, accent, std::move (label));
        case ParamKind::Bool:   return std::make_unique<ParamToggle> (p, state, accent, std::move (label));
        case ParamKind::Float:  break;
    }

    return std::make_unique<ParamSlider> (p, state, accent, std::move (label));
}

//==============================================================================
/** An ON/OFF pill bound to one or more bool parameters. With several (the two
    compressors) it reads as on when any is on, and a click sets them all. */
class PowerSwitch : public juce::Component,
                    public juce::SettableTooltipClient
{
public:
    PowerSwitch (APVTS& s, std::vector<Param> params, juce::Colour accentColour)
        : state (s), targets (std::move (params)), accent (accentColour)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("Switch this stage on or off");
        shown = isOn();
    }

    bool isOn() const
    {
        for (auto p : targets)
            if (auto* v = state.getRawParameterValue (paramId (p)))
                if (v->load() > 0.5f)
                    return true;

        return false;
    }

    /** Polled from the editor timer so host automation shows up. */
    void refresh()
    {
        const bool now = isOn();

        if (now != shown)
        {
            shown = now;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const float radius = r.getHeight() * 0.5f;

        g.setColour (shown ? accent.withAlpha (0.9f) : pal::panelSoft);
        g.fillRoundedRectangle (r, radius);

        if (! shown)
        {
            g.setColour (pal::stroke);
            g.drawRoundedRectangle (r, radius, 1.0f);
        }

        g.setColour (shown ? juce::Colours::black : pal::dim);
        g.setFont (uiFont (8.5f, true));
        g.drawText (shown ? "ON" : "OFF", getLocalBounds(), juce::Justification::centred, false);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition()) || e.mouseWasDraggedSinceMouseDown())
            return;

        const float next = isOn() ? 0.0f : 1.0f;

        for (auto p : targets)
        {
            if (auto* param = state.getParameter (paramId (p)))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (next);
                param->endChangeGesture();
            }
        }

        refresh();
    }

private:
    APVTS& state;
    std::vector<Param> targets;
    juce::Colour accent;
    bool shown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerSwitch)
};

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    LevelMeter() = default;

    void setLevels (float inputLevel, float outputLevel)
    {
        const auto nextIn  = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (inputLevel, -52.0f) + 52.0f) / 52.0f);
        const auto nextOut = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (outputLevel, -52.0f) + 52.0f) / 52.0f);

        if (std::abs (nextIn - inLevel) > 0.01f || std::abs (nextOut - outLevel) > 0.01f)
        {
            inLevel = nextIn;
            outLevel = nextOut;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop (r.getHeight() / 2);

        drawBar (g, top, "IN", inLevel);
        drawBar (g, r, "OUT", outLevel);
    }

private:
    void drawBar (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, float level)
    {
        g.setColour (pal::dim);
        g.setFont (uiFont (7.5f, true));
        g.drawText (label, area.removeFromLeft (20), juce::Justification::centredLeft, false);

        auto track = area.toFloat().withSizeKeepingCentre ((float) area.getWidth(), 3.0f);
        g.setColour (pal::stroke);
        g.fillRoundedRectangle (track, 1.5f);

        auto fill = track.withWidth (track.getWidth() * level);
        g.setColour (level > 0.92f ? pal::pink : pal::teal);
        g.fillRoundedRectangle (fill, 1.5f);
    }

    float inLevel = 0.0f;
    float outLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

//==============================================================================
/** Header preset selector: name and character tag, click for the list. */
class PresetBox : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    PresetBox()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("Choose a starting point. Key and scale stay as you set them.");
    }

    std::function<void()> onClick;

    void setPreset (const juce::String& presetName, const juce::String& presetTag)
    {
        if (name != presetName || tag != presetTag)
        {
            name = presetName;
            tag = presetTag;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? pal::panelHi : pal::panelSoft);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (pal::stroke);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        auto text = getLocalBounds().reduced (12, 4).withTrimmedRight (16);
        g.setColour (pal::text);
        g.setFont (uiFont (12.5f, true));
        g.drawText (name, text.removeFromTop (text.getHeight() / 2 + 2), juce::Justification::bottomLeft, true);

        g.setColour (pal::dim);
        g.setFont (uiFont (8.5f));
        g.drawText (tag.replace ("  |  ", sep()), text, juce::Justification::topLeft, true);

        juce::Path arrow;
        const float cx = (float) getWidth() - 14.0f;
        const float cy = (float) getHeight() * 0.5f;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.5f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (pal::dim);
        g.strokePath (arrow, juce::PathStrokeType (1.6f));
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()) && onClick != nullptr)
            onClick();
    }

private:
    juce::String name, tag;
    bool hover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBox)
};

//==============================================================================
/** Momentary delay throw: opens the delay send while held, even when the
    Delay stage is switched off, and lets the echoes ring out on release. */
class ThrowButton : public juce::Component,
                    public juce::SettableTooltipClient
{
public:
    ThrowButton()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("Hold to throw the vocal into the delay");
    }

    std::function<void (bool)> onHold;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (held ? pal::pink.withAlpha (0.22f) : pal::panelSoft);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (held ? pal::pink : pal::stroke);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);

        g.setColour (held ? pal::pink : pal::text.withAlpha (0.85f));
        g.setFont (uiFont (11.0f, true));
        g.drawText ("THROW", getLocalBounds(), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent&) override { setHeld (true); }
    void mouseUp (const juce::MouseEvent&) override   { setHeld (false); }

    /** Releases a held throw, e.g. when the editor closes mid-press. */
    void release() { setHeld (false); }

private:
    void setHeld (bool shouldHold)
    {
        if (held == shouldHold)
            return;

        held = shouldHold;
        repaint();

        if (onHold != nullptr)
            onHold (held);
    }

    bool held = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThrowButton)
};

} // namespace vx::ui

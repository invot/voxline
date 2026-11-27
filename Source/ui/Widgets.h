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
class TabBar : public juce::Component
{
public:
    TabBar (juce::StringArray labels)
        : tabs (std::move (labels))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void (int)> onTabChanged;

    void setSelected (int index, bool notify = false)
    {
        index = juce::jlimit (0, tabs.size() - 1, index);

        if (selected == index)
            return;

        selected = index;
        repaint();

        if (notify && onTabChanged != nullptr)
            onTabChanged (selected);
    }

    int getSelected() const { return selected; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        drawPanel (g, r, 12.0f, pal::header);

        auto area = getLocalBounds().reduced (8, 7);
        const int w = area.getWidth() / std::max (1, tabs.size());

        for (int i = 0; i < tabs.size(); ++i)
        {
            auto tab = area.removeFromLeft (w);
            const bool on = i == selected;

            if (on)
            {
                g.setColour (pal::panelSoft);
                g.fillRoundedRectangle (tab.toFloat(), 8.0f);
                g.setColour (pal::teal.withAlpha (0.8f));
                g.fillRoundedRectangle ((float) tab.getX() + 12.0f,
                                        (float) tab.getBottom() - 5.0f,
                                        (float) tab.getWidth() - 24.0f, 2.0f, 1.0f);
            }

            g.setColour (on ? pal::text : pal::dim);
            g.setFont (uiFont (12.5f, true));
            g.drawText (tabs[i].toUpperCase(), tab, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto area = getLocalBounds().reduced (8, 7);
        const int w = area.getWidth() / std::max (1, tabs.size());
        const int index = juce::jlimit (0, tabs.size() - 1, (e.x - area.getX()) / std::max (1, w));

        if (index != selected)
        {
            setSelected (index, true);
        }
    }

private:
    juce::StringArray tabs;
    int selected = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabBar)
};

//==============================================================================
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
class KnobSlider : public ResetSlider
{
public:
    explicit KnobSlider (juce::Colour accentColour)
        : accent (accentColour)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setMouseDragSensitivity (220);
        setRotaryParameters (3.92699f, 8.63938f, true);
        setWantsKeyboardFocus (false);
    }

    std::function<juce::String (float)> format;

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (4.0f);
        const float d = std::min (b.getWidth(), b.getHeight());
        auto circle = juce::Rectangle<float> (d, d).withCentre (b.getCentre());

        const float start = 3.92699f;
        const float end   = 8.63938f;
        const float t     = (float) ((getValue() - getMinimum())
                                     / std::max (1.0e-9, getMaximum() - getMinimum()));
        const float angle = start + juce::jlimit (0.0f, 1.0f, t) * (end - start);

        const auto arc = [&circle] (float from, float to)
        {
            juce::Path p;
            p.addCentredArc (circle.getCentreX(), circle.getCentreY(),
                             circle.getWidth() * 0.5f - 3.5f,
                             circle.getHeight() * 0.5f - 3.5f,
                             0.0f, from, to, true);
            return p;
        };

        g.setColour (pal::stroke);
        g.strokePath (arc (start, end),
                      juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
        g.setColour (accent.withAlpha (0.92f));
        g.strokePath (arc (start, angle),
                      juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        auto inner = circle.reduced (10.0f);
        g.setColour (pal::panelHi);
        g.fillEllipse (inner);
        g.setColour (pal::stroke.withAlpha (0.8f));
        g.drawEllipse (inner, 1.0f);

        const float dir = angle - juce::MathConstants<float>::halfPi;
        const auto point = juce::Point<float> (std::cos (dir), std::sin (dir));
        const auto centre = circle.getCentre();
        g.setColour (pal::text.withAlpha (0.9f));
        g.drawLine ({ centre + point * (circle.getWidth() * 0.17f),
                      centre + point * (circle.getWidth() * 0.34f) }, 2.0f);

        if (format != nullptr)
        {
            g.setColour (pal::text);
            g.setFont (uiFont (11.5f, true));
            g.drawText (format ((float) getValue()), inner.reduced (2.0f).toNearestInt(),
                        juce::Justification::centred, false);
        }
    }

private:
    juce::Colour accent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobSlider)
};

//==============================================================================
class MacroKnob : public juce::Component
{
public:
    MacroKnob (Param p, APVTS& state, juce::Colour accent)
        : param (p), knob (accent)
    {
        knob.format = [p] (float v) { return formatParamValue (p, v); };
        knob.defaultValue = paramDef (p).def;

        addAndMakeVisible (knob);
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId (p), knob);
        knob.onValueChange = [this] { repaint(); };
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (pal::dim);
        g.setFont (uiFont (10.0f, true));
        g.drawText (juce::String (paramDef (param).name).toUpperCase(), labelArea,
                    juce::Justification::centred, false);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        labelArea = b.removeFromBottom (16);
        knob.setBounds (b);
    }

private:
    Param param;
    KnobSlider knob;
    juce::Rectangle<int> labelArea;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroKnob)
};

//==============================================================================
class ParamSlider : public juce::Component
{
public:
    ParamSlider (Param p, APVTS& state, juce::Colour accentColour)
        : param (p), accent (accentColour)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::trackColourId, accent.withAlpha (0.75f));
        slider.setColour (juce::Slider::thumbColourId, accent);
        slider.defaultValue = paramDef (p).def;

        addAndMakeVisible (slider);
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId (p), slider);
        slider.onValueChange = [this] { repaint(); };
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop (16);

        g.setColour (accent.withAlpha (0.9f));
        g.fillEllipse ((float) top.getX() + 1.0f, (float) top.getY() + 6.0f, 4.0f, 4.0f);

        g.setColour (pal::dim);
        g.setFont (uiFont (10.5f, true));
        g.drawText (paramDef (param).name, top.withTrimmedLeft (10),
                    juce::Justification::centredLeft, false);

        g.setColour (pal::text);
        g.setFont (uiFont (10.5f));
        g.drawText (formatParamValue (param, (float) slider.getValue()), top,
                    juce::Justification::centredRight, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (17);
        slider.setBounds (r.reduced (0, 1));
    }

private:
    Param param;
    juce::Colour accent;
    ResetSlider slider;
    std::unique_ptr<APVTS::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSlider)
};

//==============================================================================
class ParamCombo : public juce::Component
{
public:
    ParamCombo (Param p, APVTS& state, juce::Colour accentColour)
        : param (p), accent (accentColour)
    {
        addAndMakeVisible (box);
        box.addItemList (paramChoices (p), 1);

        attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId (p), box);
    }

    void paint (juce::Graphics& g) override
    {
        auto top = getLocalBounds().removeFromTop (16);

        g.setColour (accent.withAlpha (0.9f));
        g.fillEllipse ((float) top.getX() + 1.0f, (float) top.getY() + 6.0f, 4.0f, 4.0f);

        g.setColour (pal::dim);
        g.setFont (uiFont (10.5f, true));
        g.drawText (paramDef (param).name, top.withTrimmedLeft (10),
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (17);
        box.setBounds (r.reduced (0, 1));
    }

private:
    Param param;
    juce::Colour accent;
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamCombo)
};

//==============================================================================
class ParamToggle : public juce::Component
{
public:
    ParamToggle (Param p, APVTS& state, juce::Colour accentColour)
        : param (p), accent (accentColour)
    {
        addAndMakeVisible (button);
        button.setClickingTogglesState (true);
        button.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.85f));

        attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId (p), button);

        button.onStateChange = [this]
        {
            button.setButtonText (button.getToggleState() ? "On" : "Off");
        };
        button.setButtonText (button.getToggleState() ? "On" : "Off");
    }

    void paint (juce::Graphics& g) override
    {
        auto top = getLocalBounds().removeFromTop (16);

        g.setColour (accent.withAlpha (0.9f));
        g.fillEllipse ((float) top.getX() + 1.0f, (float) top.getY() + 6.0f, 4.0f, 4.0f);

        g.setColour (pal::dim);
        g.setFont (uiFont (10.5f, true));
        g.drawText (paramDef (param).name, top.withTrimmedLeft (10),
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (17);
        button.setBounds (r.reduced (0, 1));
    }

private:
    Param param;
    juce::Colour accent;
    juce::TextButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamToggle)
};

//==============================================================================
class PowerPill : public juce::Component
{
public:
    PowerPill (Param p, APVTS& state, juce::Colour accentColour)
    {
        addAndMakeVisible (button);
        button.setClickingTogglesState (true);
        button.setColour (juce::TextButton::buttonOnColourId, accentColour.withAlpha (0.9f));

        attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId (p), button);

        button.onStateChange = [this]
        {
            button.setButtonText (button.getToggleState() ? "ON" : "OFF");
        };
        button.setButtonText (button.getToggleState() ? "ON" : "OFF");
    }

    void resized() override { button.setBounds (getLocalBounds()); }

private:
    juce::TextButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerPill)
};

//==============================================================================
class ModuleChip : public juce::Component
{
public:
    ModuleChip (juce::String label, juce::Colour accentColour)
        : name (std::move (label)), accent (accentColour)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void()> onClick;

    void setActive (bool shouldBeActive)
    {
        if (active != shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (hover ? pal::panelSoft : pal::panel);
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (hover ? accent.withAlpha (0.4f) : pal::stroke);
        g.drawRoundedRectangle (r, 9.0f, 1.0f);

        g.setColour (accent.withAlpha (active ? 1.0f : 0.22f));
        g.fillEllipse (r.getX() + 8.0f, r.getY() + 8.0f, 5.0f, 5.0f);

        g.setColour (active ? pal::text : pal::dim.withAlpha (0.75f));
        g.setFont (uiFont (9.0f, true));
        g.drawText (name.toUpperCase(), getLocalBounds().reduced (4, 10),
                    juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (onClick != nullptr)
            onClick();
    }

private:
    juce::String name;
    juce::Colour accent;
    bool active = true;
    bool hover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleChip)
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
        g.drawText (label, area.removeFromLeft (16), juce::Justification::centredLeft, false);

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
class PresetRail : public juce::Component
{
public:
    PresetRail()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    std::function<void (int)> onSelect;

    void setSelected (int index)
    {
        if (selected != index)
        {
            selected = index;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto full = getLocalBounds().toFloat();
        drawPanel (g, full.reduced (0.5f), 14.0f, pal::panel);

        g.setColour (pal::purple);
        g.fillEllipse (16.0f, 18.0f, 5.0f, 5.0f);

        g.setColour (pal::text);
        g.setFont (uiFont (11.5f, true));
        g.drawText ("PRESETS", 28, 12, getWidth() - 40, 18,
                    juce::Justification::centredLeft, false);

        g.setColour (pal::dim);
        g.setFont (uiFont (9.0f));
        g.drawText ("starting points, not rules", 16, 32, getWidth() - 32, 14,
                    juce::Justification::centredLeft, false);

        const int count = (int) factoryPresets().size();
        const int top = 54;
        const int bottomReserve = getHeight() > 560 ? 26 : 6;
        const int rowH = juce::jlimit (36, 44, (getHeight() - top - bottomReserve) / std::max (1, count));

        for (int i = 0; i < count; ++i)
        {
            auto row = juce::Rectangle<int> (10, top + i * rowH, getWidth() - 20, rowH - 4);
            const bool on = i == selected;
            const bool hot = i == hover && ! on;

            if (on || hot)
            {
                g.setColour (on ? pal::panelHi : pal::panelSoft.withAlpha (0.6f));
                g.fillRoundedRectangle (row.toFloat(), 8.0f);
            }

            if (on)
            {
                g.setColour (pal::teal);
                g.fillRoundedRectangle ((float) row.getX() + 3.0f, (float) row.getY() + 8.0f,
                                        2.5f, (float) row.getHeight() - 16.0f, 1.25f);
            }

            g.setColour (on ? pal::text : pal::text.withAlpha (0.85f));
            g.setFont (uiFont (12.0f, on));
            g.drawText (factoryPresets()[(size_t) i].name, row.getX() + 14,
                        row.getY() + 5, row.getWidth() - 30, 16,
                        juce::Justification::centredLeft, false);

            g.setColour (on ? pal::dim : pal::dim.withAlpha (0.7f));
            g.setFont (uiFont (8.5f));
            g.drawText (factoryPresets()[(size_t) i].tag, row.getX() + 14,
                        row.getY() + 21, row.getWidth() - 30, 12,
                        juce::Justification::centredLeft, false);

            if (on)
            {
                juce::Path chevron;
                const float x = (float) row.getRight() - 15.0f;
                const float y = (float) row.getCentreY();
                chevron.startNewSubPath (x - 2.0f, y - 4.0f);
                chevron.lineTo (x + 2.0f, y);
                chevron.lineTo (x - 2.0f, y + 4.0f);
                g.setColour (pal::teal);
                g.strokePath (chevron, juce::PathStrokeType (1.5f));
            }
        }

        if (getHeight() > 560)
        {
            g.setColour (pal::dim.withAlpha (0.65f));
            g.setFont (uiFont (8.5f));
            g.drawText ("TIP  move Tone and Space first.", 16, getHeight() - 22,
                        getWidth() - 32, 14, juce::Justification::centredLeft, false);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int count = (int) factoryPresets().size();
        const int top = 54;
        const int rowH = juce::jlimit (36, 44, (getHeight() - top - (getHeight() > 560 ? 26 : 6)) / std::max (1, count));
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

        const int count = (int) factoryPresets().size();
        const int top = 54;
        const int rowH = juce::jlimit (36, 44, (getHeight() - top - (getHeight() > 560 ? 26 : 6)) / std::max (1, count));
        const int index = (e.y - top) / std::max (1, rowH);

        if (e.y >= top && index >= 0 && index < count)
        {
            setSelected (index);
            if (onSelect != nullptr)
                onSelect (index);
        }
    }

private:
    int selected = 0;
    int hover = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetRail)
};

//==============================================================================
class SignalPath : public juce::Component
{
public:
    SignalPath()
    {
        static const char* names[] = { "Tune", "EQ", "De-ess", "Comp", "Drive",
                                       "Motion", "Delay", "Verb", "Width", "Limit" };
        static const juce::Colour colours[] = {
            pal::purple, pal::teal, pal::pink, pal::amber, pal::pink,
            pal::purple, pal::teal, pal::purple, pal::teal, pal::yellow
        };

        for (int i = 0; i < 10; ++i)
        {
            auto* chip = new ModuleChip (names[i], colours[i]);
            chip->onClick = [this, i] { if (onOpen != nullptr) onOpen (i); };
            addAndMakeVisible (chip);
            chips.add (chip);
        }
    }

    std::function<void (int)> onOpen;

    void setActive (int index, bool active)
    {
        if (juce::isPositiveAndBelow (index, chips.size()))
            chips[index]->setActive (active);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat().reduced (0.5f), 12.0f, pal::panel);

        g.setColour (pal::dim);
        g.setFont (uiFont (9.0f, true));
        g.drawText ("SIGNAL PATH", 14, 8, 160, 12, juce::Justification::centredLeft, false);

        g.setColour (pal::dim.withAlpha (0.6f));
        g.setFont (uiFont (8.5f));
        g.drawText ("click a stage for every control", getWidth() - 220, 8, 206, 12,
                    juce::Justification::centredRight, false);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 0);
        area.removeFromTop (24);
        area = area.removeFromTop (juce::jmin (52, getHeight() - 30));

        const int gap = 6;
        const int w = (area.getWidth() - gap * (chips.size() - 1)) / std::max (1, chips.size());

        for (auto* chip : chips)
        {
            chip->setBounds (area.removeFromLeft (w));
            area.removeFromLeft (gap);
        }
    }

private:
    juce::OwnedArray<ModuleChip> chips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalPath)
};

//==============================================================================
class ThrowButton : public juce::Component
{
public:
    ThrowButton()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
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
        g.setFont (uiFont (9.5f, true));
        g.drawText ("THROW", getLocalBounds().removeFromTop (getHeight() - 10),
                    juce::Justification::centred, false);

        g.setColour (pal::dim.withAlpha (0.8f));
        g.setFont (uiFont (7.0f));
        g.drawText ("delay send", getLocalBounds().removeFromBottom (10),
                    juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        held = true;
        repaint();
        if (onHold != nullptr)
            onHold (true);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (held)
        {
            held = false;
            repaint();
            if (onHold != nullptr)
                onHold (false);
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (held)
        {
            held = false;
            repaint();
            if (onHold != nullptr)
                onHold (false);
        }
    }

private:
    bool held = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThrowButton)
};

} // namespace vx::ui

#include "PluginEditor.h"
#include "Presets.h"

#include <cmath>

namespace vx
{

VoxlineEditor::VoxlineEditor (VoxlineProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      shape (p),
      detail (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    addAndMakeVisible (tabs);
    addAndMakeVisible (shape);
    addAndMakeVisible (detail);
    addAndMakeVisible (meter);
    addAndMakeVisible (presetButton);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (bypassButton);
    addAndMakeVisible (throwButton);

    detail.setVisible (false);

    tabs.onTabChanged = [this] (int index)
    {
        shape.setVisible (index == 0);
        detail.setVisible (index == 1);
    };

    shape.onOpenModule = [this] (int index)
    {
        tabs.setSelected (1, true);
        detail.selectModule (index);
    };

    shape.onSelectPreset = [this] (int index) { setPreset (index); };

    presetButton.setTooltip ("Choose a starting point");
    presetButton.onClick = [this] { showPresetMenu(); };

    prevButton.setButtonText ("<");
    nextButton.setButtonText (">");
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");

    prevButton.onClick = [this]
    {
        const int count = juce::jmax (1, factoryPresetCount());
        setPreset ((processor.getCurrentProgram() - 1 + count) % count);
    };

    nextButton.onClick = [this]
    {
        const int count = juce::jmax (1, factoryPresetCount());
        setPreset ((processor.getCurrentProgram() + 1) % count);
    };

    bypassButton.setButtonText ("BYPASS");
    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::TextButton::buttonOnColourId, ui::pal::pink);
    bypassButton.setTooltip ("Latency-compensated bypass");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, paramId (Param::Bypass), bypassButton);

    throwButton.onHold = [this] (bool held) { processor.setThrowHeld (held); };

    setResizable (true, true);
    setResizeLimits (1100, 720, 1600, 1000);
    setSize (1180, 800);

    refreshPresetDisplay();
    refreshLatencyText();
    startTimerHz (24);
}

VoxlineEditor::~VoxlineEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxlineEditor::setPreset (int index)
{
    processor.setCurrentProgram (index);
    shownProgram = processor.getCurrentProgram();
    shape.setSelectedPreset (shownProgram);
    presetButton.setButtonText (factoryPresetName (shownProgram));
    presetButton.setTooltip (factoryPresetTag (shownProgram));

    shape.refresh();
    detail.refresh();
    repaint();
}

void VoxlineEditor::refreshPresetDisplay()
{
    const int program = processor.getCurrentProgram();

    if (program == shownProgram)
        return;

    shownProgram = program;
    shape.setSelectedPreset (program);
    presetButton.setButtonText (factoryPresetName (program));
    presetButton.setTooltip (factoryPresetTag (program));
}

void VoxlineEditor::showPresetMenu()
{
    juce::PopupMenu menu;

    for (int i = 0; i < factoryPresetCount(); ++i)
        menu.addItem (i + 1, factoryPresetName (i), true, i == processor.getCurrentProgram());

    auto safe = juce::Component::SafePointer<VoxlineEditor> (this);

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&presetButton)
                            .withMinimumWidth (210),
                        [safe] (int result)
                        {
                            if (safe == nullptr || result <= 0)
                                return;

                            safe->setPreset (result - 1);
                        });
}

void VoxlineEditor::refreshLatencyText()
{
    const double sr = processor.getSampleRate();
    const int latency = processor.reportedLatency();

    juce::String next = "PDC --";

    if (sr > 0.0 && latency > 0)
        next = "PDC " + juce::String (1000.0 * (double) latency / sr, 1) + " ms  |  "
             + juce::String (sr / 1000.0, 1) + " kHz";

    if (next != latencyText)
    {
        latencyText = next;
        repaint();
    }
}

void VoxlineEditor::timerCallback()
{
    meter.setLevels (processor.inputMeter(), processor.outputMeter());
    refreshPresetDisplay();
    refreshLatencyText();
    shape.refresh();
    detail.refresh();
}

void VoxlineEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::pal::bg);

    auto header = getLocalBounds().removeFromTop (56).reduced (14, 6).toFloat();
    ui::drawPanel (g, header, 14.0f, ui::pal::header);

    auto logo = juce::Rectangle<float> (header.getX() + 14.0f, header.getY() + 8.0f, 30.0f, 30.0f);
    g.setColour (ui::pal::panelSoft);
    g.fillRoundedRectangle (logo, 8.0f);
    g.setColour (ui::pal::teal.withAlpha (0.85f));
    g.drawRoundedRectangle (logo, 8.0f, 1.2f);

    juce::Path wave;

    for (int i = 0; i <= 28; ++i)
    {
        const float t = (float) i / 28.0f;
        const float x = logo.getX() + 7.0f + t * (logo.getWidth() - 14.0f);
        const float env = 0.55f + 0.45f * std::sin (t * juce::MathConstants<float>::pi);
        const float y = logo.getCentreY() + std::sin (t * juce::MathConstants<float>::pi * 3.0f) * 5.5f * env;

        if (i == 0)
            wave.startNewSubPath (x, y);
        else
            wave.lineTo (x, y);
    }

    g.setColour (ui::pal::teal);
    g.strokePath (wave, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));

    g.setColour (ui::pal::text);
    g.setFont (ui::uiFont (18.0f, true));
    g.drawText ("voxline", logo.getRight() + 10.0f, header.getY() + 5.0f, 150.0f, 22.0f,
                juce::Justification::centredLeft, false);

    g.setColour (ui::pal::dim);
    g.setFont (ui::uiFont (9.5f));
    g.drawText ("vocal chain", logo.getRight() + 11.0f, header.getY() + 25.0f, 150.0f, 12.0f,
                juce::Justification::centredLeft, false);

    auto footer = getLocalBounds().removeFromBottom (42).reduced (14, 6).toFloat();
    ui::drawPanel (g, footer, 12.0f, ui::pal::header);

    g.setColour (ui::pal::dim.withAlpha (0.75f));
    g.setFont (ui::uiFont (8.5f));
    g.drawText ("SHAPE IS THE FAST VIEW  |  DETAIL HAS EVERY CONTROL",
                footer.getX() + 14.0f, footer.getY(), footer.getWidth() - 360.0f, footer.getHeight(),
                juce::Justification::centredLeft, false);

    g.setFont (ui::uiFont (8.5f));
    g.drawText (latencyText, footer.getRight() - 270.0f, footer.getY(), 180.0f, footer.getHeight(),
                juce::Justification::centredRight, false);
}

void VoxlineEditor::resized()
{
    auto b = getLocalBounds();

    auto header = b.removeFromTop (56).reduced (14, 6);
    auto footer = b.removeFromBottom (42).reduced (14, 6);

    auto h = header;
    bypassButton.setBounds (h.removeFromRight (76).withSizeKeepingCentre (76, 26));
    h.removeFromRight (8);
    nextButton.setBounds (h.removeFromRight (24).withSizeKeepingCentre (24, 26));
    presetButton.setBounds (h.removeFromRight (168).withSizeKeepingCentre (168, 26));
    prevButton.setBounds (h.removeFromRight (24).withSizeKeepingCentre (24, 26));
    h.removeFromRight (10);
    meter.setBounds (h.removeFromRight (86).withSizeKeepingCentre (86, 24));

    auto f = footer;
    throwButton.setBounds (f.removeFromRight (74).withSizeKeepingCentre (74, 26));

    b.reduce (14, 0);
    tabs.setBounds (b.removeFromTop (46));
    b.removeFromTop (8);
    b.removeFromBottom (8);

    shape.setBounds (b);
    detail.setBounds (b);
}

} // namespace vx

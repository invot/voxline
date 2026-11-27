#include "PluginEditor.h"
#include "Presets.h"

#include <cmath>

namespace vx
{

VoxlineEditor::VoxlineEditor (VoxlineProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      chain (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    addAndMakeVisible (chain);
    addAndMakeVisible (meter);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (bypassButton);
    addAndMakeVisible (throwButton);

    presetBox.onClick = [this] { showPresetMenu(); };

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

    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::TextButton::buttonOnColourId, ui::pal::pink);
    bypassButton.setTooltip ("Latency-compensated bypass with a short crossfade");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, paramId (Param::Bypass), bypassButton);

    throwButton.onHold = [this] (bool held) { processor.setThrowHeld (held); };

    setResizable (true, true);
    setResizeLimits (1080, 700, 1700, 1100);
    setSize (1180, 780);

    refreshPresetDisplay (true);
    refreshLatencyText();
    startTimerHz (24);
}

VoxlineEditor::~VoxlineEditor()
{
    stopTimer();

    // Closing the editor mid-press must not leave the delay thrown open.
    throwButton.onHold = nullptr;
    processor.setThrowHeld (false);

    setLookAndFeel (nullptr);
}

void VoxlineEditor::setPreset (int index)
{
    processor.setCurrentProgram (index);
    refreshPresetDisplay (true);
    chain.refresh();
}

void VoxlineEditor::refreshPresetDisplay (bool force)
{
    const int program = processor.getCurrentProgram();

    if (program == shownProgram && ! force)
        return;

    shownProgram = program;
    presetBox.setPreset (juce::String (program + 1).paddedLeft ('0', 2) + "  " + factoryPresetName (program),
                         factoryPresetTag (program));
}

void VoxlineEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Starting points");

    for (int i = 0; i < factoryPresetCount(); ++i)
        menu.addItem (i + 1, factoryPresetName (i) + "    " + factoryPresetTag (i).replace ("  |  ", ui::sep()),
                      true, i == processor.getCurrentProgram());

    auto safe = juce::Component::SafePointer<VoxlineEditor> (this);

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&presetBox)
                            .withMinimumWidth (presetBox.getWidth()),
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
        next = "PDC " + juce::String (1000.0 * (double) latency / sr, 1) + " ms" + ui::sep()
             + juce::String (sr / 1000.0, 1) + " kHz";

    if (next != latencyText)
    {
        latencyText = next;
        repaint (headerArea);
    }
}

void VoxlineEditor::timerCallback()
{
    meter.setLevels (processor.inputMeter(), processor.outputMeter());
    refreshPresetDisplay();
    refreshLatencyText();
    chain.refresh();
}

void VoxlineEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::pal::bg);

    const auto header = headerArea.toFloat();
    ui::drawPanel (g, header, 14.0f, ui::pal::header);

    auto logo = juce::Rectangle<float> (header.getX() + 12.0f, header.getCentreY() - 15.0f, 30.0f, 30.0f);
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
    g.drawText ("voxline", (int) logo.getRight() + 10, (int) logo.getY() - 2, 150, 20,
                juce::Justification::centredLeft, false);

    g.setColour (ui::pal::dim);
    g.setFont (ui::uiFont (9.0f));
    g.drawText (juce::String ("vocal chain") + ui::sep() + latencyText, (int) logo.getRight() + 11, (int) logo.getY() + 18, 240, 12,
                juce::Justification::centredLeft, false);
}

void VoxlineEditor::resized()
{
    auto b = getLocalBounds().reduced (14, 0);

    b.removeFromTop (10);
    headerArea = b.removeFromTop (54);
    b.removeFromTop (12);
    b.removeFromBottom (14);
    chain.setBounds (b);

    auto h = headerArea.reduced (12, 0);
    bypassButton.setBounds (h.removeFromRight (78).withSizeKeepingCentre (78, 30));
    h.removeFromRight (8);
    throwButton.setBounds (h.removeFromRight (74).withSizeKeepingCentre (74, 30));
    h.removeFromRight (16);
    meter.setBounds (h.removeFromRight (110).withSizeKeepingCentre (110, 24));

    // Presets sit in the middle of the header.
    const int presetW = 300;
    auto centre = headerArea.withSizeKeepingCentre (presetW + 2 * 34, 36);
    prevButton.setBounds (centre.removeFromLeft (28).withSizeKeepingCentre (28, 30));
    nextButton.setBounds (centre.removeFromRight (28).withSizeKeepingCentre (28, 30));
    presetBox.setBounds (centre.reduced (6, 0));
}

} // namespace vx

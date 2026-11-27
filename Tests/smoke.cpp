// Console smoke harness: builds the processor and editor without a DAW, renders
// every factory preset, checks every preset states every stage, round-trips
// state, and writes editor snapshots to PNG.

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include "ui/ChainView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace
{
    int failures = 0;

    void check (bool ok, const char* what)
    {
        std::fprintf (stderr, "%s  %s\n", ok ? "[ok]  " : "[FAIL]", what);

        if (! ok)
            ++failures;
    }

    struct TestVocal
    {
        void fill (float* l, float* r, int n)
        {
            constexpr double sr = 48000.0;

            for (int i = 0; i < n; ++i)
            {
                const double t = (double) (sampleIndex + i) / sr;
                const double vibrato = 0.22 * std::sin (juce::MathConstants<double>::twoPi * 5.1 * t);
                const double freq = 196.0 * std::pow (2.0, vibrato / 12.0);

                phase += juce::MathConstants<double>::twoPi * freq / sr;

                double s = 0.20 * (std::sin (phase)
                                   + 0.32 * std::sin (2.0 * phase + 0.3)
                                   + 0.12 * std::sin (3.0 * phase + 0.8));

                const double noise = (double) rng.nextFloat() * 2.0 - 1.0;
                s += noise * 0.005;

                l[i] = (float) s;
                r[i] = (float) (s * 0.97 + noise * 0.003);
            }

            sampleIndex += n;
        }

        double phase = 0.0;
        long long sampleIndex = 0;
        juce::Random rng { 0x5eed1234 };
    };

    float renderPreset (vx::VoxlineProcessor& proc, int index, const char* name)
    {
        proc.setCurrentProgram (index);
        proc.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        TestVocal vocal;

        float peak = 0.0f;
        bool finite = true;
        double sumSquares = 0.0;
        long long measured = 0;
        float comp1 = 0.0f, comp2 = 0.0f, limiter = 0.0f;

        constexpr int blocks = 188;
        constexpr int settleBlocks = blocks / 2;   // measure loudness once tails and envelopes settle

        for (int block = 0; block < blocks; ++block)
        {
            buffer.clear();
            vocal.fill (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
            proc.processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* data = buffer.getReadPointer (ch);

                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    if (! std::isfinite (data[i]))
                        finite = false;

                    peak = juce::jmax (peak, std::abs (data[i]));

                    if (block >= settleBlocks)
                    {
                        sumSquares += (double) data[i] * data[i];
                        ++measured;
                    }
                }
            }

            if (block >= settleBlocks)
            {
                comp1 = juce::jmax (comp1, proc.comp1Gr());
                comp2 = juce::jmax (comp2, proc.comp2Gr());
                limiter = juce::jmax (limiter, proc.limiterGr());
            }
        }

        // Not a pass/fail check: a level report so presets can be compared and
        // kept roughly loudness-matched against each other.
        const double rms = std::sqrt (sumSquares / (double) juce::jmax (1LL, measured));
        std::fprintf (stderr, "        %-26s rms %6.1f dBFS  peak %6.1f dBFS  GR fast %4.1f  smooth %4.1f  limit %4.1f dB\n",
                      name, juce::Decibels::gainToDecibels ((float) rms, -120.0f),
                      juce::Decibels::gainToDecibels (peak, -120.0f), comp1, comp2, limiter);

        juce::String prefix (name);

        check (finite, (prefix + ": output stays finite").toRawUTF8());

        const auto silent = prefix + ": output is not silent";
        check (peak > 0.0005f, silent.toRawUTF8());

        const auto ceiling = prefix + ": output respects the ceiling";
        check (peak <= 1.001f, ceiling.toRawUTF8());

        return peak;
    }

    bool savePng (juce::Component& component, const juce::File& file)
    {
        const auto image = component.createComponentSnapshot (component.getLocalBounds());

        file.deleteFile();
        juce::FileOutputStream out (file);

        return out.openedOk() && juce::PNGImageFormat().writeImageToStream (image, out);
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    vx::VoxlineProcessor proc;

    check (proc.getNumPrograms() == vx::factoryPresetCount(), "every preset is a host program");
    check (vx::factoryPresetCount() == 15, "preset table has fifteen entries");

    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    check (proc.reportedLatency() > 0, "chain reports latency");
    check (proc.getLatencySamples() == proc.reportedLatency(), "host latency matches the chain");

    // Every preset states every stage (reverb, limiter, both compressors, EQ,
    // de-esser, pitch, drive, width, chorus, delay), once, inside its range.
    for (int i = 0; i < vx::factoryPresetCount(); ++i)
    {
        const auto problems = vx::factoryPresetProblems (i);

        for (const auto& problem : problems)
            std::fprintf (stderr, "        %s: %s\n", vx::factoryPresetName (i).toRawUTF8(), problem.toRawUTF8());

        const auto what = vx::factoryPresetName (i) + ": sets every stage, in range";
        check (problems.isEmpty(), what.toRawUTF8());
    }

    // Every parameter except bypass (which lives in the header) has a control.
    {
        const auto reachable = vx::ui::ChainView::reachableParams();
        bool all = true;

        for (int i = 0; i < (int) vx::Param::Count; ++i)
        {
            const auto p = (vx::Param) i;

            if (p == vx::Param::Bypass)
                continue;

            if (std::find (reachable.begin(), reachable.end(), p) == reachable.end())
            {
                std::fprintf (stderr, "        no control for %s\n", vx::paramId (p));
                all = false;
            }
        }

        check (all, "every parameter has a control in the chain view");
    }

    // Presets keep the song's key and scale.
    {
        auto* key = proc.apvts.getParameter ("tuneKey");
        auto* scale = proc.apvts.getParameter ("tuneScale");
        key->setValueNotifyingHost (key->convertTo0to1 (9.0f));      // A
        scale->setValueNotifyingHost (scale->convertTo0to1 (2.0f));  // natural minor
        proc.setCurrentProgram (4);
        check (std::lround (proc.apvts.getRawParameterValue ("tuneKey")->load()) == 9
                   && std::lround (proc.apvts.getRawParameterValue ("tuneScale")->load()) == 2,
               "changing preset keeps key and scale");
        key->setValueNotifyingHost (key->convertTo0to1 (0.0f));
        scale->setValueNotifyingHost (scale->convertTo0to1 (0.0f));
    }

    for (int i = 0; i < vx::factoryPresetCount(); ++i)
    {
        char label[96];
        std::snprintf (label, sizeof (label), "%02d %s", i + 1, vx::factoryPresetName (i).toRawUTF8());
        renderPreset (proc, i, label);
    }

    // A preset really moves the values, not just the name.
    proc.setCurrentProgram (6);
    const float grit = proc.apvts.getRawParameterValue ("grit")->load();
    check (grit > 60.0f, "Grunge switches the Grit macro");

    // State round-trip.
    proc.setCurrentProgram (3);
    const float space = proc.apvts.getRawParameterValue ("space")->load();
    juce::MemoryBlock state;
    proc.getStateInformation (state);

    proc.setCurrentProgram (0);
    check (std::abs (proc.apvts.getRawParameterValue ("space")->load() - space) > 0.01f,
           "switching programs changes state");

    proc.setStateInformation (state.getData(), (int) state.getSize());
    check (proc.getCurrentProgram() == 3, "state restores the current program");
    check (std::abs (proc.apvts.getRawParameterValue ("space")->load() - space) < 0.01f,
           "state restores parameter values");

    proc.prepareToPlay (48000.0, 512);

    auto* editor = static_cast<vx::VoxlineEditor*> (proc.createEditor());
    check (editor != nullptr, "editor is created");

    if (editor != nullptr)
    {
        std::unique_ptr<vx::VoxlineEditor> owner (editor);
        editor->setVisible (true);
        editor->setSize (1180, 780);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (80);

        editor->showPresetForSnapshot (0);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (60);

        check (editor->getChainView().getSelectedModule() == vx::ui::ChainView::Pitch, "view starts on Pitch");

        for (int stage = 0; stage < vx::ui::ChainView::NumStages; ++stage)
        {
            editor->getChainView().selectModule (stage);
            const auto what = "selects " + vx::ui::ChainView::stageName (stage);
            check (editor->getChainView().getSelectedModule() == stage, what.toRawUTF8());
        }

        const auto snapshotDir = juce::SystemStats::getEnvironmentVariable ("VX_SNAPSHOT_DIR", {});

        if (snapshotDir.isNotEmpty())
        {
            juce::File dir (snapshotDir);
            dir.createDirectory();

            // The old two-page snapshots went with the two pages.
            for (auto* old : { "shape.png", "shape-grand.png", "detail-eq.png", "detail-reverb.png", "detail-tune.png" })
                dir.getChildFile (old).deleteFile();

            const struct { int preset; int stage; const char* file; } shots[] =
            {
                { 3, vx::ui::ChainView::Pitch,  "pitch-polished-pop.png" },
                { 0, vx::ui::ChainView::Eq,     "eq-close-intimate.png" },
                { 6, vx::ui::ChainView::Comp,   "comp-grunge.png" },
                { 4, vx::ui::ChainView::Reverb, "reverb-ethereal.png" },
                { 2, vx::ui::ChainView::Chorus, "chorus-chorus-stack.png" },
                { 1, vx::ui::ChainView::Delay,  "delay-grand-stadium.png" }
            };

            for (const auto& shot : shots)
            {
                editor->showPresetForSnapshot (shot.preset);
                editor->getChainView().selectModule (shot.stage);

                // Run some audio so the meters, tuner readout and GR bars are live.
                juce::AudioBuffer<float> buffer (2, 512);
                juce::MidiBuffer midi;
                TestVocal vocal;

                for (int block = 0; block < 60; ++block)
                {
                    vocal.fill (buffer.getWritePointer (0), buffer.getWritePointer (1), 512);
                    proc.processBlock (buffer, midi);
                }

                editor->getChainView().refresh();
                juce::MessageManager::getInstance()->runDispatchLoopUntil (60);

                const auto what = juce::String ("wrote ") + shot.file;
                check (savePng (*editor, dir.getChildFile (shot.file)), what.toRawUTF8());
            }

            std::printf ("snapshots: %s\n", dir.getFullPathName().toRawUTF8());
        }
        else
        {
            std::printf ("set VX_SNAPSHOT_DIR to write editor snapshots\n");
        }
    }

    std::printf ("\n%s\n", failures == 0 ? "SMOKE CHECKS PASSED" : "SMOKE CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}

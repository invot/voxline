// Console smoke harness: builds the processor and editor without a DAW, renders
// every factory preset, round-trips state, and writes both editor pages to PNG.

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"

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

        for (int block = 0; block < 188; ++block)
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
                }
            }
        }

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

    check (proc.getNumPrograms() == vx::factoryPresetCount(), "ten factory programs");
    check (vx::factoryPresetCount() == 10, "preset table has ten entries");

    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    check (proc.reportedLatency() > 0, "chain reports latency");
    check (proc.getLatencySamples() == proc.reportedLatency(), "host latency matches the chain");

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
        editor->setSize (1180, 800);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (80);

        editor->showPresetForSnapshot (0);
        editor->getTabBar().setSelected (0, true);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (60);

        check (editor->getDetailPage().getSelectedModule() == 0, "detail starts on Tune");

        const auto snapshotDir = juce::SystemStats::getEnvironmentVariable ("VX_SNAPSHOT_DIR", {});

        if (snapshotDir.isNotEmpty())
        {
            juce::File dir (snapshotDir);
            dir.createDirectory();

            editor->getShapePage().refresh();
            check (savePng (*editor, dir.getChildFile ("shape.png")), "wrote shape.png");

            editor->showPresetForSnapshot (1);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            check (savePng (*editor, dir.getChildFile ("shape-grand.png")), "wrote shape-grand.png");

            editor->getTabBar().setSelected (1, true);
            editor->getDetailPage().selectModule (1);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            check (savePng (*editor, dir.getChildFile ("detail-eq.png")), "wrote detail-eq.png");

            editor->getDetailPage().selectModule (8);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            check (savePng (*editor, dir.getChildFile ("detail-reverb.png")), "wrote detail-reverb.png");

            editor->getDetailPage().selectModule (0);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            check (savePng (*editor, dir.getChildFile ("detail-tune.png")), "wrote detail-tune.png");

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

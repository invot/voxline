#pragma once

#include "Parameters.h"
#include "core/Chain.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <vector>

namespace vx
{

class VoxlineEditor;

class VoxlineProcessor : public juce::AudioProcessor
{
public:
    VoxlineProcessor();
    ~VoxlineProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "voxline"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- editor helpers -------------------------------------------------
    ChainParams getParamsForUI() const { return buildParams(); }
    float inputMeter() const { return chain.inputMeter(); }
    float outputMeter() const { return chain.outputMeter(); }
    float comp1Gr() const { return chain.comp1Gr(); }
    float comp2Gr() const { return chain.comp2Gr(); }
    float limiterGr() const { return chain.limiterGr(); }
    float tuneCents() const { return chain.tuneCents(); }
    int reportedLatency() const { return chain.latencySamples(); }

    /** Momentary delay throw, held by the editor's button. */
    void setThrowHeld (bool held) { throwHeld.store (held); }
    bool isThrowHeld() const { return throwHeld.load(); }

private:
    ChainParams buildParams() const;

    std::array<std::atomic<float>*, (size_t) Param::Count> cached {};

    VocalChain chain;
    std::vector<float> monoScratch;
    float playheadBpm = 120.0f;
    std::atomic<bool> throwHeld { false };
    std::atomic<int> currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxlineProcessor)
};

} // namespace vx

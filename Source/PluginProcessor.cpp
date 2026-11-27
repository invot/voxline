#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

#include <algorithm>
#include <cmath>

namespace vx
{

VoxlineProcessor::VoxlineProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < (int) Param::Count; ++i)
        cached[(size_t) i] = apvts.getRawParameterValue (paramId ((Param) i));

    // Open on the close/intimate starting point, the way the reference presets
    // are meant to be auditioned.
    applyFactoryPreset (apvts, 0);
    currentProgram.store (0);
    apvts.state.setProperty ("program", 0, nullptr);
}

bool VoxlineProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    const bool inOk = in == juce::AudioChannelSet::mono()
                   || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono()
                    || out == juce::AudioChannelSet::stereo();

    return inOk && outOk && ! in.isDisabled() && ! out.isDisabled();
}

void VoxlineProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chain.prepare (sampleRate, samplesPerBlock);
    monoScratch.assign ((size_t) std::max (16, samplesPerBlock), 0.0f);
    setLatencySamples (chain.latencySamples());
}

void VoxlineProcessor::releaseResources()
{
    chain.reset();
}

ChainParams VoxlineProcessor::buildParams() const
{
    const auto v = [this] (Param p)
    {
        return cached[(size_t) p]->load (std::memory_order_relaxed);
    };

    const auto f = [&v] (Param p) { return v (p); };
    const auto on = [&v] (Param p) { return v (p) > 0.5f; };
    const auto pct = [&v] (Param p) { return v (p) * 0.01f; };

    ChainParams p;

    p.bypass = on (Param::Bypass);

    p.press = pct (Param::Press);
    p.tone = pct (Param::Tone);
    p.grit = pct (Param::Grit);
    p.width = pct (Param::Width);
    p.space = pct (Param::Space);
    p.levelDb = f (Param::Level);

    p.tune.on = on (Param::TuneOn);
    p.tune.amount = pct (Param::Tune);
    p.tune.speedMs = f (Param::TuneSpeed);
    p.tune.key = (int) std::lround (f (Param::TuneKey));
    p.tune.scale = (int) std::lround (f (Param::TuneScale));
    p.tune.formant = f (Param::Formant);
    p.tune.octaveMix = pct (Param::OctaveMix);

    p.eq.on = on (Param::EqOn);
    p.eq.hpf = f (Param::Hpf);
    p.eq.lp = f (Param::LpFreq);
    p.eq.lowFreq = f (Param::LowFreq);
    p.eq.lowGain = f (Param::LowGain);
    p.eq.bodyFreq = f (Param::BodyFreq);
    p.eq.bodyGain = f (Param::BodyGain);
    p.eq.bodyQ = f (Param::BodyQ);
    p.eq.presFreq = f (Param::PresFreq);
    p.eq.presGain = f (Param::PresGain);
    p.eq.presQ = f (Param::PresQ);
    p.eq.airFreq = f (Param::AirFreq);
    p.eq.airGain = f (Param::AirGain);

    p.deess.on = on (Param::DeessOn);
    p.deess.freq = f (Param::DeessFreq);
    p.deess.thresholdDb = f (Param::DeessThresh);
    p.deess.amountDb = f (Param::DeessAmount);

    p.comp1.on = on (Param::Comp1On);
    p.comp1.thresholdDb = f (Param::Comp1Thresh);
    p.comp1.ratio = f (Param::Comp1Ratio);
    p.comp1.attackMs = f (Param::Comp1Attack);
    p.comp1.releaseMs = f (Param::Comp1Release);
    p.comp1.mix = pct (Param::Comp1Mix);
    p.comp1.makeupDb = f (Param::Comp1Makeup);

    p.comp2.on = on (Param::Comp2On);
    p.comp2.thresholdDb = f (Param::Comp2Thresh);
    p.comp2.ratio = f (Param::Comp2Ratio);
    p.comp2.attackMs = f (Param::Comp2Attack);
    p.comp2.releaseMs = f (Param::Comp2Release);
    p.comp2.mix = pct (Param::Comp2Mix);
    p.comp2.makeupDb = f (Param::Comp2Makeup);

    p.sat.on = on (Param::SatOn);
    p.sat.mode = (int) std::lround (f (Param::SatMode));
    p.sat.drive = f (Param::SatDrive);
    p.sat.tone = pct (Param::SatTone);
    p.sat.mix = pct (Param::SatMix);
    p.sat.bits = (int) std::lround (f (Param::SatBits));

    p.motion.on = on (Param::MotionOn);
    p.motion.mode = (int) std::lround (f (Param::MotionMode));
    p.motion.rate = f (Param::MotionRate);
    p.motion.depth = pct (Param::MotionDepth);
    p.motion.feedback = pct (Param::MotionFeedback);
    p.motion.width = pct (Param::MotionWidth);
    p.motion.mix = pct (Param::MotionMix);

    p.delay.on = on (Param::DelayOn);
    p.delay.timeMs = f (Param::DelayTime);
    p.delay.sync = (int) std::lround (f (Param::DelaySync));
    p.delay.feedback = pct (Param::DelayFeedback);
    p.delay.tone = f (Param::DelayTone);
    p.delay.mix = pct (Param::DelayMix);
    p.delay.pingPong = on (Param::DelayPingPong);
    p.delay.duck = pct (Param::DelayDuck);
    p.delay.bpm = playheadBpm;

    p.verb.on = on (Param::VerbOn);
    p.verb.size = pct (Param::VerbSize);
    p.verb.preMs = f (Param::VerbPre);
    p.verb.damp = pct (Param::VerbDamp);
    p.verb.width = pct (Param::VerbWidth);
    p.verb.mix = pct (Param::VerbMix);
    p.verb.duck = pct (Param::VerbDuck);
    p.verb.hp = f (Param::VerbHP);
    p.verb.lp = f (Param::VerbLP);
    p.verb.shimmer = pct (Param::VerbShimmer);

    p.limiterOn = on (Param::LimiterOn);
    p.ceilingDb = f (Param::Ceiling);
    p.limiterReleaseMs = f (Param::LimiterRelease);

    p.bpm = playheadBpm;
    p.throwAmount = throwHeld.load() ? 1.0f : 0.0f;

    return p;
}

void VoxlineProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();

    if (n <= 0)
        return;

    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            if (const auto bpm = pos->getBpm())
                playheadBpm = (float) *bpm;
        }
    }

    if (channels >= 2)
    {
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);

        if (getTotalNumInputChannels() < 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, n);

        chain.process (l, r, n, buildParams());

        if (getTotalNumOutputChannels() == 1)
            for (int i = 0; i < n; ++i)
                l[i] = 0.5f * (l[i] + r[i]);

        for (int ch = 2; ch < channels; ++ch)
            buffer.clear (ch, 0, n);
    }
    else
    {
        auto* l = buffer.getWritePointer (0);

        if ((int) monoScratch.size() < n)
            monoScratch.resize ((size_t) n);

        std::copy (l, l + n, monoScratch.begin());
        chain.process (l, monoScratch.data(), n, buildParams());

        if (getTotalNumOutputChannels() == 1)
            for (int i = 0; i < n; ++i)
                l[i] = 0.5f * (l[i] + monoScratch[(size_t) i]);
    }
}

int VoxlineProcessor::getNumPrograms()
{
    return std::max (1, factoryPresetCount());
}

int VoxlineProcessor::getCurrentProgram()
{
    return currentProgram.load();
}

void VoxlineProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= factoryPresetCount())
        return;

    applyFactoryPreset (apvts, index);
    currentProgram.store (index);
    apvts.state.setProperty ("program", index, nullptr);
}

const juce::String VoxlineProcessor::getProgramName (int index)
{
    return factoryPresetName (index);
}

juce::AudioProcessorEditor* VoxlineProcessor::createEditor()
{
    return new VoxlineEditor (*this);
}

void VoxlineProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);

    juce::MemoryOutputStream out (destData, true);
    state.writeToStream (out);
}

void VoxlineProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);

    if (tree.isValid() && tree.hasType (apvts.state.getType()))
    {
        apvts.replaceState (tree);
        currentProgram.store ((int) tree.getProperty ("program", 0));
    }
}

} // namespace vx

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vx::VoxlineProcessor();
}

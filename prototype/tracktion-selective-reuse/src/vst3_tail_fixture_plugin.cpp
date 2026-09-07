#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>

namespace
{
constexpr int tailSamples = 1024;
constexpr float tailAmplitude = 0.25f;
}

class A2TailFixture final : public juce::AudioProcessor
{
public:
    A2TailFixture()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::mono(), true)
                                             .withOutput ("Output", juce::AudioChannelSet::mono(), true)) {}

    const juce::String getName() const override { return "A2 Tail Fixture"; }
    void prepareToPlay (double, int) override { remainingTailSamples = 0; sawSource = false; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto* channel = buffer.getWritePointer (0);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto input = channel[sample];
            if (input != 0.0f)
            {
                sawSource = true;
                remainingTailSamples = tailSamples;
                channel[sample] = input;
            }
            else if (sawSource && remainingTailSamples > 0)
            {
                channel[sample] = tailAmplitude;
                --remainingTailSamples;
            }
            else
            {
                channel[sample] = 0.0f;
            }
        }
    }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    double getTailLengthSeconds() const override { return static_cast<double> (tailSamples) / 48000.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

private:
    int remainingTailSamples = 0;
    bool sawSource = false;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new A2TailFixture(); }

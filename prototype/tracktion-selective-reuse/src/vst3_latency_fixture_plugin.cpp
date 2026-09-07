#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

#ifndef A2_LATENCY_SAMPLES
#error A2_LATENCY_SAMPLES must be defined for this fixture target
#endif

class A2LatencyFixture final : public juce::AudioProcessor
{
public:
    A2LatencyFixture()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::mono(), true)
                                             .withOutput ("Output", juce::AudioChannelSet::mono(), true))
    {
        setLatencySamples (A2_LATENCY_SAMPLES);
    }

    const juce::String getName() const override { return "A2 Latency Fixture"; }
    void prepareToPlay (double, int) override { delay.fill (0.0f); write = 0; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    { return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono()
          && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono(); }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto* channel = buffer.getWritePointer (0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto input = channel[i];
            channel[i] = delay[static_cast<std::size_t> (write)];
            delay[static_cast<std::size_t> (write)] = input;
            write = (write + 1) % A2_LATENCY_SAMPLES;
        }
    }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    double getTailLengthSeconds() const override { return 0.0; }
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
    std::array<float, A2_LATENCY_SAMPLES> delay {};
    int write = 0;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new A2LatencyFixture(); }

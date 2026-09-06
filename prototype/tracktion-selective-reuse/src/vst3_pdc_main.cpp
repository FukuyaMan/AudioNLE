#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vst3_pdc
{
constexpr std::int64_t event = 1024;
constexpr int blockSize = 128;
constexpr float epsilon = 0.000001f;

class Source final : public tracktion::graph::Node
{
public:
    explicit Source (float v) : value (v) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }
private:
    void process (ProcessContext& c) override
    {
        const auto offset = event - c.referenceSampleRange.getStart();
        choc::buffer::setAllFrames (c.buffers.audio, [this, offset] (auto frame) { return (std::int64_t) frame == offset ? value : 0.0f; });
        c.buffers.midi.clear();
    }
    float value;
};

std::unique_ptr<juce::AudioPluginInstance> loadPlugin (bool prepare)
{
    juce::VST3PluginFormatHeadless format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile (descriptions, juce::String (A2_VST3_FIXTURE_PATH));
    if (descriptions.isEmpty()) throw std::runtime_error ("VST3 known-path scan returned no description");
    juce::AudioPluginFormatManager manager;
    manager.addFormat (std::make_unique<juce::VST3PluginFormatHeadless>());
    juce::String error;
    auto instance = manager.createPluginInstance (*descriptions[0], 48000.0, blockSize, error);
    if (! instance) throw std::runtime_error ("VST3 createPluginInstance failed: " + error.toStdString());
    if (prepare)
    {
        instance->setRateAndBufferSizeDetails (48000.0, blockSize);
        instance->prepareToPlay (48000.0, blockSize);
    }
    return instance;
}

int measureDirectDelay (juce::AudioPluginInstance& plugin)
{
    int firstNonZero = -1;
    juce::MidiBuffer midi;
    for (int block = 0; block < 24; ++block)
    {
        juce::AudioBuffer<float> buffer (1, blockSize);
        buffer.clear();
        if (block == 0) buffer.setSample (0, 0, 1.0f);
        plugin.processBlock (buffer, midi);
        for (int i = 0; i < blockSize; ++i)
            if (std::abs (buffer.getSample (0, i)) > epsilon)
            {
                const auto position = block * blockSize + i;
                if (firstNonZero != -1) throw std::runtime_error ("fixture emitted more than one impulse");
                firstNonZero = position;
            }
    }
    return firstNonZero;
}

class PluginNode final : public tracktion::graph::Node
{
public:
    PluginNode (std::unique_ptr<tracktion::graph::Node> owned, std::unique_ptr<juce::AudioPluginInstance> pluginToUse)
        : inputOwner (std::move (owned)), input (inputOwner.get()), plugin (std::move (pluginToUse)) {}
    tracktion::graph::NodeProperties getNodeProperties() override
    { auto p = input->getNodeProperties(); p.latencyNumSamples += plugin->getLatencySamples(); return p; }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }
private:
    void process (ProcessContext& c) override
    {
        const auto in = input->getProcessedOutput();
        const auto frames = (int) c.buffers.audio.getNumFrames();
        juce::AudioBuffer<float> buffer (1, frames);
        for (int i = 0; i < frames; ++i) buffer.setSample (0, i, in.audio.getSample (0, i));
        juce::MidiBuffer midi;
        plugin->processBlock (buffer, midi);
        for (int i = 0; i < frames; ++i) c.buffers.audio.getSample (0, i) = buffer.getSample (0, i);
        c.buffers.midi.clear();
    }
    std::unique_ptr<tracktion::graph::Node> inputOwner;
    tracktion::graph::Node* input;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
};

std::vector<float> render (std::unique_ptr<tracktion::graph::Node> graph)
{
    tracktion::graph::SimpleNodePlayer player (std::move (graph), 48000.0, blockSize);
    std::vector<float> result;
    choc::buffer::ChannelArrayBuffer<float> buffer;
    buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi;
    for (std::int64_t pos = 0; pos < 4096; pos += blockSize)
    {
        auto view = buffer.getView(); view.clear(); midi.clear();
        player.process ({ blockSize, juce::Range<std::int64_t>::withStartAndLength (pos, blockSize), { view, midi } });
        for (int i = 0; i < blockSize; ++i) result.push_back (view.getSample (0, i));
    }
    return result;
}

void requireAt (const std::vector<float>& output, int position, float value, const char* name)
{
    if (std::abs (output.at ((size_t) position) - value) > epsilon)
        throw std::runtime_error (std::string (name) + " output mismatch");
    for (size_t i = 0; i < output.size(); ++i)
        if ((int) i != position && std::abs (output[i]) > epsilon)
            throw std::runtime_error (std::string (name) + " non-zero at unexpected sample");
}
}

int main()
{
    try
    {
        using namespace vst3_pdc;
        auto created = loadPlugin (false);
        const auto createdLatency = created->getLatencySamples();
        created->setRateAndBufferSizeDetails (48000.0, blockSize);
        created->prepareToPlay (48000.0, blockSize);
        const auto preparedLatency = created->getLatencySamples();
        const auto actualDelay = measureDirectDelay (*created);

        auto adapterPlugin = loadPlugin (true);
        auto adapter = std::make_unique<PluginNode> (std::make_unique<Source> (0.25f), std::move (adapterPlugin));
        const auto adapterLatency = adapter->getNodeProperties().latencyNumSamples;

        std::cout << "VST3 latency diagnostic create=" << createdLatency
                  << " prepare=" << preparedLatency
                  << " first-process=" << created->getLatencySamples()
                  << " actual-delay=" << actualDelay
                  << " adapter-after-prepare=" << adapterLatency << '\n';

        if (preparedLatency != 1024 || created->getLatencySamples() != 1024
            || actualDelay != 1024 || adapterLatency != 1024)
            throw std::runtime_error ("VST3 latency contract diagnostic mismatch");
        return 0;
    }
    catch (const std::exception& e) { std::cerr << "VST3 PDC FAIL: " << e.what() << '\n'; return 1; }
}

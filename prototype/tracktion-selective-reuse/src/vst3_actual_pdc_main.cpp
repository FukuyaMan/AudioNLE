#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace actual_vst3_pdc
{
using Sample = std::int64_t;
constexpr Sample eventSample = 1024;
constexpr int blockSize = 128;
constexpr float epsilon = 0.000001f;

struct PluginFixture
{
    const char* path;
    int latency;
};

const PluginFixture fixture256 { A2_VST3_FIXTURE_256_PATH, 256 };
const PluginFixture fixture768 { A2_VST3_FIXTURE_768_PATH, 768 };
const PluginFixture fixture1024 { A2_VST3_FIXTURE_1024_PATH, 1024 };
const PluginFixture fixture2048 { A2_VST3_FIXTURE_2048_PATH, 2048 };

struct ProcessorState
{
    std::string id;
    std::string layer;
    const PluginFixture* fixture = nullptr;
    int deterministicLatency = 0;
    float multiply = 1.0f;
    bool enabled = true;

    bool operator== (const ProcessorState&) const = default;
};

struct PathState
{
    std::string id;
    float impulse = 0.0f;
    std::vector<ProcessorState> processors;

    bool operator== (const PathState&) const = default;
};

struct DomainState
{
    Sample event = eventSample;
    std::vector<PathState> paths;

    bool operator== (const DomainState&) const = default;
};

class SourceNode final : public tracktion::graph::Node
{
public:
    SourceNode (Sample eventToUse, float impulseToUse) : event (eventToUse), impulse (impulseToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }
private:
    void process (ProcessContext& context) override
    {
        const auto offset = event - context.referenceSampleRange.getStart();
        choc::buffer::setAllFrames (context.buffers.audio, [this, offset] (auto frame)
        {
            return static_cast<Sample> (frame) == offset ? impulse : 0.0f;
        });
        context.buffers.midi.clear();
    }
    Sample event;
    float impulse;
};

std::unique_ptr<juce::AudioPluginInstance> loadPreparedPlugin (const PluginFixture& fixture)
{
    juce::VST3PluginFormatHeadless format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile (descriptions, juce::String (fixture.path));
    if (descriptions.isEmpty())
        throw std::runtime_error ("VST3 known-path scan returned no description");

    juce::AudioPluginFormatManager manager;
    manager.addFormat (std::make_unique<juce::VST3PluginFormatHeadless>());
    juce::String error;
    auto plugin = manager.createPluginInstance (*descriptions[0], 48000.0, blockSize, error);
    if (! plugin)
        throw std::runtime_error ("VST3 createPluginInstance failed: " + error.toStdString());
    plugin->setRateAndBufferSizeDetails (48000.0, blockSize);
    plugin->prepareToPlay (48000.0, blockSize);
    if (plugin->getLatencySamples() != fixture.latency)
        throw std::runtime_error ("hosted latency after prepare differs from fixture declaration");
    return plugin;
}

int measureActualDelay (const PluginFixture& fixture)
{
    auto plugin = loadPreparedPlugin (fixture);
    juce::MidiBuffer midi;
    int observed = -1;
    for (int block = 0; block < 32; ++block)
    {
        juce::AudioBuffer<float> buffer (1, blockSize);
        buffer.clear();
        if (block == 0)
            buffer.setSample (0, 0, 1.0f);
        plugin->processBlock (buffer, midi);
        for (int frame = 0; frame < blockSize; ++frame)
            if (std::abs (buffer.getSample (0, frame)) > epsilon)
            {
                if (observed != -1)
                    throw std::runtime_error ("fixture emitted more than one impulse");
                observed = block * blockSize + frame;
            }
    }
    if (observed != fixture.latency)
        throw std::runtime_error ("actual VST3 delay differs from fixture declaration");
    return observed;
}

class PluginNode final : public tracktion::graph::Node
{
public:
    PluginNode (std::unique_ptr<tracktion::graph::Node> inputToUse, const PluginFixture& fixture)
        : inputOwner (std::move (inputToUse)), input (inputOwner.get()), plugin (loadPreparedPlugin (fixture)) {}
    tracktion::graph::NodeProperties getNodeProperties() override
    {
        auto properties = input->getNodeProperties();
        properties.latencyNumSamples += plugin->getLatencySamples();
        return properties;
    }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }
private:
    void process (ProcessContext& context) override
    {
        const auto source = input->getProcessedOutput();
        const auto frames = static_cast<int> (context.buffers.audio.getNumFrames());
        juce::AudioBuffer<float> buffer (1, frames);
        for (int frame = 0; frame < frames; ++frame)
            buffer.setSample (0, frame, source.audio.getSample (0, frame));
        juce::MidiBuffer midi;
        plugin->processBlock (buffer, midi);
        for (int frame = 0; frame < frames; ++frame)
            context.buffers.audio.getSample (0, frame) = buffer.getSample (0, frame);
        context.buffers.midi.clear();
    }
    std::unique_ptr<tracktion::graph::Node> inputOwner;
    tracktion::graph::Node* input;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
};

class MultiplyNode final : public tracktion::graph::Node
{
public:
    MultiplyNode (std::unique_ptr<tracktion::graph::Node> inputToUse, float amountToUse)
        : inputOwner (std::move (inputToUse)), input (inputOwner.get()), amount (amountToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }
private:
    void process (ProcessContext& context) override
    {
        choc::buffer::copy (context.buffers.audio, input->getProcessedOutput().audio);
        for (int frame = 0; frame < context.buffers.audio.getNumFrames(); ++frame)
            context.buffers.audio.getSample (0, frame) *= amount;
        context.buffers.midi.clear();
    }
    std::unique_ptr<tracktion::graph::Node> inputOwner;
    tracktion::graph::Node* input;
    float amount;
};

struct RenderResult
{
    std::vector<float> samples;
    int rootLatency = 0;
    Sample firstNonZero = -1;
    Sample lastNonZero = -1;
};

std::unique_ptr<tracktion::graph::Node> buildPath (const DomainState& domain, const PathState& path)
{
    std::unique_ptr<tracktion::graph::Node> node = std::make_unique<SourceNode> (domain.event, path.impulse);
    for (const auto& processor : path.processors)
    {
        if (! processor.enabled)
            continue;
        if (processor.fixture != nullptr)
            node = std::make_unique<PluginNode> (std::move (node), *processor.fixture);
        else if (processor.deterministicLatency != 0)
            node = std::make_unique<tracktion::graph::LatencyNode> (std::move (node), processor.deterministicLatency);
        else if (processor.multiply != 1.0f)
            node = std::make_unique<MultiplyNode> (std::move (node), processor.multiply);
    }
    return node;
}

RenderResult render (const DomainState& domain)
{
    const auto before = domain;
    std::vector<std::unique_ptr<tracktion::graph::Node>> inputs;
    for (const auto& path : domain.paths)
        inputs.push_back (buildPath (domain, path));
    std::unique_ptr<tracktion::graph::Node> root = inputs.size() == 1
        ? std::move (inputs.front()) : std::make_unique<tracktion::graph::SummingNode> (std::move (inputs));
    RenderResult result;
    result.rootLatency = root->getNodeProperties().latencyNumSamples;
    tracktion::graph::SimpleNodePlayer player (std::move (root), 48000.0, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer;
    buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi;
    for (Sample position = 0; position < 8192; position += blockSize)
    {
        auto view = buffer.getView();
        view.clear();
        midi.clear();
        player.process ({ blockSize, juce::Range<Sample>::withStartAndLength (position, blockSize), { view, midi } });
        for (int frame = 0; frame < blockSize; ++frame)
            result.samples.push_back (view.getSample (0, frame));
    }
    for (std::size_t i = 0; i < result.samples.size(); ++i)
        if (std::abs (result.samples[i]) > epsilon)
        {
            const auto position = static_cast<Sample> (i);
            result.firstNonZero = result.firstNonZero == -1 ? position : result.firstNonZero;
            result.lastNonZero = position;
        }
    if (domain != before)
        throw std::runtime_error ("runtime mutated framework-free Domain state");
    return result;
}

ProcessorState vst3 (std::string id, const PluginFixture& fixture, std::string layer, bool enabled = true)
{ return { std::move (id), std::move (layer), &fixture, 0, 1.0f, enabled }; }
ProcessorState latency (std::string id, int samples, std::string layer)
{ return { std::move (id), std::move (layer), nullptr, samples, 1.0f, true }; }
ProcessorState multiply (std::string id, float amount)
{ return { std::move (id), "clip", nullptr, 0, amount, true }; }
DomainState domain (std::vector<PathState> paths) { return { eventSample, std::move (paths) }; }

void requireMix (const RenderResult& result, Sample expected, float amplitude, int latency, const char* label)
{
    if (result.rootLatency != latency || result.firstNonZero != expected || result.lastNonZero != expected
        || std::abs (result.samples.at (static_cast<size_t> (expected)) - amplitude) > epsilon)
        throw std::runtime_error (std::string (label) + " did not align exactly");
}

void testContractAndTwoPath()
{
    requireMix (render (domain ({ { "baseline", 0.25f, {} } })), eventSample, 0.25f, 0, "baseline");
    for (const auto* fixture : { &fixture256, &fixture1024, &fixture2048 })
    {
        const auto actual = measureActualDelay (*fixture);
        const auto single = render (domain ({ { "contract", 0.25f, { vst3 ("vst3", *fixture, "clip") } } }));
        requireMix (single, eventSample + fixture->latency, 0.25f, fixture->latency, "VST3 contract");
        const auto mixed = render (domain ({
            { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("vst3", *fixture, "clip") } }
        }));
        requireMix (mixed, eventSample + fixture->latency, 0.75f, fixture->latency, "two-path VST3 PDC");
        std::cout << "VST3-PDC value=" << fixture->latency << " declared=" << fixture->latency
                  << " hosted=" << fixture->latency << " actual=" << actual << " graph=" << mixed.rootLatency
                  << " observed=" << mixed.firstNonZero << " error=0\n";
    }
}

void testLayersAndOrder()
{
    requireMix (render (domain ({ { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("clip", fixture1024, "clip") } } })),
                2048, 0.75f, 1024, "clip layer");
    requireMix (render (domain ({ { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("track", fixture1024, "track") } } })),
                2048, 0.75f, 1024, "track layer");
    requireMix (render (domain ({
        { "a", 0.10f, {} },
        { "b", 0.20f, { latency ("clip-256", 256, "clip"), vst3 ("track-768", fixture768, "track") } },
        { "c", 0.40f, { vst3 ("clip-1024", fixture1024, "clip") } }
    })), 2048, 0.70f, 1024, "mixed layers");
    requireMix (render (domain ({ { "a", 0.25f, { vst3 ("vst3", fixture1024, "clip"), multiply ("multiply", 2.0f) } } })),
                2048, 0.50f, 1024, "VST3 then multiply");
    requireMix (render (domain ({ { "a", 0.25f, { multiply ("multiply", 2.0f), vst3 ("vst3", fixture1024, "clip") } } })),
                2048, 0.50f, 1024, "multiply then VST3");
    std::cout << "VST3-PDC layers=clip,track,mixed order=both observed=2048 error=0\n";
}

void testBypassChangeAndReconstruction()
{
    requireMix (render (domain ({ { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("bypass", fixture1024, "clip", false) } } })),
                1024, 0.75f, 0, "adapter bypass policy");
    const auto before = domain ({ { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("1024", fixture1024, "track") } } });
    const auto after = domain ({ { "a", 0.25f, {} }, { "b", 0.50f, { vst3 ("2048", fixture2048, "track") } } });
    requireMix (render (before), 2048, 0.75f, 1024, "stopped change before");
    requireMix (render (after), 3072, 0.75f, 2048, "stopped change after");
    const auto reconstruction = domain ({
        { "a", 0.10f, {} },
        { "b", 0.20f, { latency ("clip-256", 256, "clip"), vst3 ("track-768", fixture768, "track") } },
        { "c", 0.40f, { vst3 ("clip-1024", fixture1024, "clip") } }
    });
    const auto first = render (reconstruction);
    const auto second = render (reconstruction);
    if (first.samples != second.samples || first.rootLatency != second.rootLatency)
        throw std::runtime_error ("VST3 reconstruction changed output or graph latency");
    requireMix (second, 2048, 0.70f, 1024, "reconstruction");
    std::cout << "VST3-PDC bypass=0 stopped-change=1024->2048 reconstruction=equal error=0\n";
}
}

int main()
{
    try
    {
        actual_vst3_pdc::testContractAndTwoPath();
        actual_vst3_pdc::testLayersAndOrder();
        actual_vst3_pdc::testBypassChangeAndReconstruction();
        std::cout << "VST3-PDC PASS maximum-alignment-error=0\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "VST3-PDC FAIL: " << error.what() << '\n';
        return 1;
    }
}

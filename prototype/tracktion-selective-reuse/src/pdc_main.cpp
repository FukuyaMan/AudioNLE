#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pdc
{
using Sample = std::int64_t;
constexpr auto epsilon = 0.000001f;
constexpr auto eventSample = Sample { 1024 };
constexpr auto renderEnd = Sample { 8192 };

enum class ProcessorType { testLatency, multiply };

// Authoritative fixture state. Tracktion/JUCE values do not enter this boundary.
struct ProcessorState
{
    std::string id;
    ProcessorType type = ProcessorType::testLatency;
    int latencySamples = 0;
    float parameter = 1.0f;
    bool enabled = true;
    std::string layer;

    bool operator== (const ProcessorState&) const = default;
};

struct PathState
{
    std::string id;
    float impulseValue = 0.0f;
    std::vector<ProcessorState> processors;

    bool operator== (const PathState&) const = default;
};

struct DomainState
{
    Sample projectTimelineSampleRate = 48000;
    Sample timelineEvent = eventSample;
    std::vector<PathState> paths;

    bool operator== (const DomainState&) const = default;
};

struct Observation
{
    std::vector<std::string> processorOrder;
    std::vector<juce::Range<Sample>> requestedRanges;
    int rootReportedLatency = 0;
};

class DomainControlledSourceNode final : public tracktion::graph::Node
{
public:
    DomainControlledSourceNode (Sample event, float value, std::shared_ptr<Observation> observationToUse)
        : timelineEvent (event), impulseValue (value), observation (std::move (observationToUse)) {}

    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

private:
    void process (ProcessContext& context) override
    {
        observation->requestedRanges.push_back (context.referenceSampleRange);
        const auto offset = timelineEvent - context.referenceSampleRange.getStart();
        choc::buffer::setAllFrames (context.buffers.audio, [this, offset] (auto frame)
        {
            return static_cast<Sample> (frame) == offset ? impulseValue : 0.0f;
        });
        context.buffers.midi.clear();
    }

    Sample timelineEvent;
    float impulseValue;
    std::shared_ptr<Observation> observation;
};

class MultiplyNode final : public tracktion::graph::Node
{
public:
    MultiplyNode (std::unique_ptr<tracktion::graph::Node> inputToUse, ProcessorState stateToUse,
                  std::shared_ptr<Observation> observationToUse)
        : ownedInput (std::move (inputToUse)), input (ownedInput.get()), state (std::move (stateToUse)), observation (std::move (observationToUse))
    {
        if (input == nullptr || state.type != ProcessorType::multiply)
            throw std::invalid_argument ("MultiplyNode requires a Multiply Domain processor");
    }

    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }

private:
    void process (ProcessContext& context) override
    {
        const auto inputOutput = input->getProcessedOutput();
        choc::buffer::copy (context.buffers.audio, inputOutput.audio);
        context.buffers.midi.copyFrom (inputOutput.midi);
        observation->processorOrder.push_back (state.id);

        if (! state.enabled)
            return;

        for (auto channel = 0; channel < context.buffers.audio.getNumChannels(); ++channel)
            for (auto frame = 0; frame < context.buffers.audio.getNumFrames(); ++frame)
                context.buffers.audio.getSample (channel, frame) *= state.parameter;
    }

    std::unique_ptr<tracktion::graph::Node> ownedInput;
    tracktion::graph::Node* input = nullptr;
    ProcessorState state;
    std::shared_ptr<Observation> observation;
};

struct RenderOutput
{
    std::vector<float> samples;
    Observation observation;
    Sample firstNonZero = -1;
    Sample lastNonZero = -1;
};

class Runtime final
{
public:
    explicit Runtime (const DomainState& domain)
        : observation (std::make_shared<Observation>()),
          player (std::make_unique<tracktion::graph::SimpleNodePlayer> (buildGraph (domain), 48000.0, blockSize))
    {
        if (domain.projectTimelineSampleRate != 48000 || domain.timelineEvent != eventSample || domain.paths.empty())
            throw std::invalid_argument ("PDC fixture requires the fixed 48 kHz / event 1024 Domain state");
    }

    RenderOutput render (Sample start = 0, Sample end = renderEnd)
    {
        if (start < 0 || end <= start)
            throw std::invalid_argument ("PDC render range is invalid");

        observation->processorOrder.clear();
        observation->requestedRanges.clear();
        RenderOutput output;
        output.samples.reserve (static_cast<std::size_t> (end - start));
        choc::buffer::ChannelArrayBuffer<float> blockBuffer;
        blockBuffer.resize ({ 1, static_cast<choc::buffer::FrameCount> (blockSize) });
        tracktion::engine::MidiMessageArray midi;

        for (auto position = start; position < end;)
        {
            const auto frames = static_cast<int> (std::min<Sample> (blockSize, end - position));
            auto block = blockBuffer.getView().getStart (static_cast<choc::buffer::FrameCount> (frames));
            block.clear();
            midi.clear();
            player->process ({ static_cast<choc::buffer::FrameCount> (frames),
                               juce::Range<Sample>::withStartAndLength (position, frames), { block, midi } });
            for (int frame = 0; frame < frames; ++frame)
                output.samples.push_back (block.getSample (0, static_cast<choc::buffer::FrameCount> (frame)));
            position += frames;
        }

        output.observation = *observation;
        for (std::size_t i = 0; i < output.samples.size(); ++i)
            if (std::abs (output.samples[i]) > epsilon)
            {
                const auto absolute = start + static_cast<Sample> (i);
                if (output.firstNonZero == -1)
                    output.firstNonZero = absolute;
                output.lastNonZero = absolute;
            }
        return output;
    }

private:
    static constexpr int blockSize = 128;
    std::shared_ptr<Observation> observation;
    std::unique_ptr<tracktion::graph::SimpleNodePlayer> player;

    std::unique_ptr<tracktion::graph::Node> buildPath (const DomainState& domain, const PathState& path)
    {
        std::unique_ptr<tracktion::graph::Node> node = std::make_unique<DomainControlledSourceNode> (
            domain.timelineEvent, path.impulseValue, observation);
        for (const auto& processor : path.processors)
        {
            if (! processor.enabled)
                continue;
            if (processor.type == ProcessorType::testLatency)
            {
                if (processor.latencySamples < 0)
                    throw std::invalid_argument ("Reported latency cannot be negative");
                node = std::make_unique<tracktion::graph::LatencyNode> (std::move (node), processor.latencySamples);
            }
            else
            {
                node = std::make_unique<MultiplyNode> (std::move (node), processor, observation);
            }
        }
        return node;
    }

    std::unique_ptr<tracktion::graph::Node> buildGraph (const DomainState& domain)
    {
        std::vector<std::unique_ptr<tracktion::graph::Node>> inputs;
        for (const auto& path : domain.paths)
            inputs.push_back (buildPath (domain, path));
        std::unique_ptr<tracktion::graph::Node> graph = inputs.size() == 1
            ? std::move (inputs.front())
            : std::make_unique<tracktion::graph::SummingNode> (std::move (inputs));
        observation->rootReportedLatency = graph->getNodeProperties().latencyNumSamples;
        return graph;
    }
};

DomainState domainWithPaths (std::vector<PathState> paths)
{
    DomainState domain;
    domain.paths = std::move (paths);
    return domain;
}

ProcessorState latency (std::string id, int samples, std::string layer = "clip", bool enabled = true)
{
    return { std::move (id), ProcessorType::testLatency, samples, 1.0f, enabled, std::move (layer) };
}

ProcessorState multiply (std::string id, float amount, std::string layer = "clip")
{
    return { std::move (id), ProcessorType::multiply, 0, amount, true, std::move (layer) };
}

RenderOutput render (const DomainState& domain)
{
    const auto before = domain;
    Runtime runtime (domain);
    const auto output = runtime.render();
    if (domain != before)
        throw std::runtime_error ("Runtime mutated authoritative Domain state");
    return output;
}

float at (const RenderOutput& output, Sample absolute)
{
    return output.samples.at (static_cast<std::size_t> (absolute));
}

void requireNear (float actual, float expected, const char* label)
{
    if (std::abs (actual - expected) > epsilon)
        throw std::runtime_error (std::string (label) + " actual=" + std::to_string (actual)
                                  + " expected=" + std::to_string (expected));
}

void requireAlignedMix (const RenderOutput& output, Sample expectedPosition, float expectedAmplitude, const char* label)
{
    if (output.firstNonZero != expectedPosition || output.lastNonZero != expectedPosition)
        throw std::runtime_error (std::string (label) + " onset/last does not equal compensated Timeline sample");
    requireNear (at (output, expectedPosition), expectedAmplitude, label);
}

void testBaseline()
{
    const auto output = render (domainWithPaths ({ { "baseline", 0.25f, {} } }));
    requireAlignedMix (output, eventSample, 0.25f, "baseline");
    if (output.observation.rootReportedLatency != 0)
        throw std::runtime_error ("baseline graph reported latency is non-zero");
    std::cout << "PDC baseline event=1024 observed=1024 error=0\n";
}

void testLatencyNodeContract()
{
    const auto output = render (domainWithPaths ({
        { "latency-contract", 0.25f, { latency ("reported-1024", 1024) } }
    }));
    requireAlignedMix (output, 2048, 0.25f, "latency node actual delay");
    if (output.observation.rootReportedLatency != 1024)
        throw std::runtime_error ("LatencyNode reported latency differs from its actual delay fixture");
    std::cout << "PDC latency-contract reported=1024 actual=1024 observed=2048 error=0\n";
}

void testTwoPathAndValues()
{
    for (const auto samples : { 256, 1024, 2048 })
    {
        const auto output = render (domainWithPaths ({
            { "path-a", 0.25f, {} },
            { "path-b", 0.50f, { latency ("path-b-latency", samples) } }
        }));
        const auto expected = eventSample + samples;
        requireAlignedMix (output, expected, 0.75f, "two-path PDC");
        if (output.observation.rootReportedLatency != samples)
            throw std::runtime_error ("root reported latency differs from deterministic processor latency");
        std::cout << "PDC two-path latency=" << samples << " pathA=" << expected << " pathB-uncompensated="
                  << eventSample + samples << " compensated=" << expected << " error=0\n";
    }
}

void testTrackAndClipLayers()
{
    const auto track = render (domainWithPaths ({
        { "track-a", 0.25f, {} },
        { "track-b", 0.50f, { latency ("track-latency", 1024, "track") } }
    }));
    requireAlignedMix (track, 2048, 0.75f, "track-equivalent PDC");

    const auto clip = render (domainWithPaths ({
        { "clip-a", 0.25f, {} },
        { "clip-b", 0.50f, { latency ("clip-latency", 1024, "clip") } }
    }));
    requireAlignedMix (clip, 2048, 0.75f, "clip-equivalent PDC");
    std::cout << "PDC clip-and-track latency=1024 observed=2048 error=0\n";
}

void testMixedLayers()
{
    const auto output = render (domainWithPaths ({
        { "path-a", 0.10f, {} },
        { "path-b", 0.20f, { latency ("b-clip", 256, "clip"), latency ("b-track", 768, "track") } },
        { "path-c", 0.40f, { latency ("c-clip", 1024, "clip") } }
    }));
    requireAlignedMix (output, 2048, 0.70f, "mixed-layer PDC");
    if (output.observation.rootReportedLatency != 1024)
        throw std::runtime_error ("mixed-layer root reported latency differs from total path latency");
    std::cout << "PDC mixed-layer totals=0,1024,1024 observed=2048 error=0\n";
}

void testOrder()
{
    const auto latencyThenMultiply = render (domainWithPaths ({
        { "ordered", 0.25f, { latency ("latency-first", 1024), multiply ("multiply-second", 2.0f) } }
    }));
    const auto multiplyThenLatency = render (domainWithPaths ({
        { "ordered", 0.25f, { multiply ("multiply-first", 2.0f), latency ("latency-second", 1024) } }
    }));
    requireAlignedMix (latencyThenMultiply, 2048, 0.50f, "latency then multiply");
    requireAlignedMix (multiplyThenLatency, 2048, 0.50f, "multiply then latency");
    if (latencyThenMultiply.observation.processorOrder.empty() || multiplyThenLatency.observation.processorOrder.empty()
        || latencyThenMultiply.observation.processorOrder.front() != "multiply-second"
        || multiplyThenLatency.observation.processorOrder.front() != "multiply-first")
        throw std::runtime_error ("Domain processor order did not reach runtime wrapper");
    std::cout << "PDC order latency-multiply/multiply-latency timing=2048 amplitude=0.5 error=0\n";
}

void testLatencyChangeAndBypass()
{
    const auto before = domainWithPaths ({ { "a", 0.25f, {} }, { "b", 0.50f, { latency ("b", 1024) } } });
    const auto after = domainWithPaths ({ { "a", 0.25f, {} }, { "b", 0.50f, { latency ("b", 2048) } } });
    const auto first = render (before);
    const auto second = render (after);
    requireAlignedMix (first, 2048, 0.75f, "latency change before");
    requireAlignedMix (second, 3072, 0.75f, "latency change after");
    if (second.observation.rootReportedLatency != 2048)
        throw std::runtime_error ("changed latency did not propagate to root graph metadata");

    const auto bypass = render (domainWithPaths ({
        { "a", 0.25f, {} }, { "b", 0.50f, { latency ("disabled-latency", 1024, "clip", false) } }
    }));
    requireAlignedMix (bypass, eventSample, 0.75f, "disabled latency");
    if (bypass.observation.rootReportedLatency != 0)
        throw std::runtime_error ("disabled Domain processor still contributes latency");
    std::cout << "PDC latency-change 1024->2048 observed=2048->3072 error=0 bypass=1024 error=0\n";
}

void testReconstruction()
{
    const auto domain = domainWithPaths ({
        { "a", 0.25f, {} },
        { "b", 0.50f, { latency ("clip-latency", 256, "clip"), latency ("track-latency", 768, "track") } },
        { "c", 0.40f, { latency ("clip-latency-c", 1024, "clip") } }
    });
    const auto first = render (domain);
    const auto second = render (domain);
    if (first.samples != second.samples || first.observation.rootReportedLatency != second.observation.rootReportedLatency
        || first.observation.processorOrder != second.observation.processorOrder
        || first.observation.requestedRanges != second.observation.requestedRanges)
        throw std::runtime_error ("PDC reconstruction changed runtime observation");
    requireAlignedMix (second, 2048, 1.15f, "reconstruction");
    std::cout << "PDC reconstruction output/ranges/latency/order=equal error=0\n";
}
}

int main()
{
    try
    {
        const auto started = std::chrono::steady_clock::now();
        pdc::testBaseline();
        pdc::testLatencyNodeContract();
        pdc::testTwoPathAndValues();
        pdc::testTrackAndClipLayers();
        pdc::testMixedLayers();
        pdc::testOrder();
        pdc::testLatencyChangeAndBypass();
        pdc::testReconstruction();
        const auto elapsed = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - started).count();
        std::cout << "PDC PASS elapsed_ms=" << elapsed << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PDC FAIL: " << error.what() << '\n';
        return 1;
    }
}

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

namespace a2_2
{
using TimelineSample = std::int64_t;
constexpr auto epsilon = 0.000001f;

enum class SignalKind { constant, impulse };
enum class ProcessorKind { add, multiply };

// Authoritative fixture state. It deliberately contains neither Tracktion nor JUCE types.
struct SourceState
{
    std::string id;
    TimelineSample nativeSampleRate = 48000;
    SignalKind signalKind = SignalKind::constant;
    float value = 0.0f;
    TimelineSample eventSample = 0;

    bool operator== (const SourceState&) const = default;
};

struct ProcessorState
{
    std::string id;
    ProcessorKind kind = ProcessorKind::add;
    float parameter = 0.0f;
    bool enabled = true;

    bool operator== (const ProcessorState&) const = default;
};

struct LayerState
{
    std::string id;
    std::vector<ProcessorState> processors;

    bool operator== (const LayerState&) const = default;
};

struct DomainState
{
    TimelineSample projectSampleRate = 48000;
    std::vector<SourceState> sources;
    LayerState clipLayer { "clip" };
    LayerState trackLayer { "track" };
    LayerState masterLayer { "master" };

    bool operator== (const DomainState&) const = default;
};

struct RenderRange
{
    TimelineSample start = 0;
    TimelineSample end = 0;

    bool operator== (const RenderRange&) const = default;
};

struct RuntimeObservation
{
    std::vector<std::string> requestedProcessorOrder;
    std::vector<RenderRange> requestedTimelineRanges;
};

class DomainControlledSourceNode final : public tracktion::graph::Node
{
public:
    DomainControlledSourceNode (SourceState sourceToUse, std::shared_ptr<RuntimeObservation> observationToUse)
        : source (std::move (sourceToUse)), observation (std::move (observationToUse))
    {
    }

    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

protected:
    void process (ProcessContext& context) override
    {
        const auto range = RenderRange { context.referenceSampleRange.getStart(), context.referenceSampleRange.getEnd() };
        observation->requestedTimelineRanges.push_back (range);
        const auto eventOffset = source.eventSample - range.start;

        choc::buffer::setAllFrames (context.buffers.audio, [this, eventOffset] (auto frame)
        {
            if (source.signalKind == SignalKind::constant)
                return source.value;

            return static_cast<TimelineSample> (frame) == eventOffset ? source.value : 0.0f;
        });
        context.buffers.midi.clear();
    }

private:
    SourceState source;
    std::shared_ptr<RuntimeObservation> observation;
};

// This deterministic processor is test instrumentation, not a replacement graph engine.
// Its input ownership, process ordering, output buffers, and graph traversal are all public
// tracktion_graph responsibilities.
class DeterministicProcessorNode final : public tracktion::graph::Node
{
public:
    DeterministicProcessorNode (std::unique_ptr<tracktion::graph::Node> inputToUse,
                                ProcessorState processorToUse,
                                std::shared_ptr<RuntimeObservation> observationToUse)
        : ownedInput (std::move (inputToUse)), input (ownedInput.get()), processor (std::move (processorToUse)), observation (std::move (observationToUse))
    {
        if (input == nullptr)
            throw std::invalid_argument ("Processor Node requires an input Node");
    }

    tracktion::graph::NodeProperties getNodeProperties() override
    {
        auto properties = input->getNodeProperties();
        properties.latencyNumSamples += 0;
        return properties;
    }

    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }

protected:
    void process (ProcessContext& context) override
    {
        const auto inputOutput = input->getProcessedOutput();
        choc::buffer::copy (context.buffers.audio, inputOutput.audio);
        context.buffers.midi.copyFrom (inputOutput.midi);
        observation->requestedProcessorOrder.push_back (processor.id);

        if (! processor.enabled)
            return;

        for (auto channel = 0; channel < context.buffers.audio.getNumChannels(); ++channel)
            for (auto frame = 0; frame < context.buffers.audio.getNumFrames(); ++frame)
            {
                auto& sample = context.buffers.audio.getSample (channel, frame);
                sample = processor.kind == ProcessorKind::add ? sample + processor.parameter
                                                               : sample * processor.parameter;
            }
    }

private:
    std::unique_ptr<tracktion::graph::Node> ownedInput;
    tracktion::graph::Node* input = nullptr;
    ProcessorState processor;
    std::shared_ptr<RuntimeObservation> observation;
};

struct RenderOutput
{
    std::vector<float> samples;
    RuntimeObservation observation;
    TimelineSample firstNonZeroAbsolute = -1;
    TimelineSample alignmentError = -1;
};

class LowLevelRuntime final
{
public:
    explicit LowLevelRuntime (const DomainState& domain)
        : observation (std::make_shared<RuntimeObservation>()),
          player (std::make_unique<tracktion::graph::SimpleNodePlayer> (
              buildGraph (domain), static_cast<double> (domain.projectSampleRate), blockSize))
    {
        for (const auto& source : domain.sources)
            if (source.nativeSampleRate != domain.projectSampleRate)
                throw std::invalid_argument ("A2-2 intentionally performs no Source-to-Timeline conversion");
    }

    RenderOutput render (RenderRange range)
    {
        if (range.end <= range.start)
            throw std::invalid_argument ("Render range must be non-empty");

        observation->requestedProcessorOrder.clear();
        observation->requestedTimelineRanges.clear();
        RenderOutput output;
        output.samples.reserve (static_cast<std::size_t> (range.end - range.start));
        choc::buffer::ChannelArrayBuffer<float> blockBuffer;
        blockBuffer.resize ({ 1, static_cast<choc::buffer::FrameCount> (blockSize) });
        tracktion::engine::MidiMessageArray midi;

        for (auto position = range.start; position < range.end;)
        {
            const auto frames = static_cast<int> (std::min<TimelineSample> (blockSize, range.end - position));
            auto block = blockBuffer.getView().getStart (static_cast<choc::buffer::FrameCount> (frames));
            block.clear();
            midi.clear();
            const auto requested = juce::Range<TimelineSample>::withStartAndLength (position, frames);
            player->process ({ static_cast<choc::buffer::FrameCount> (frames), requested, { block, midi } });

            for (int frame = 0; frame < frames; ++frame)
                output.samples.push_back (block.getSample (0, static_cast<choc::buffer::FrameCount> (frame)));
            position += frames;
        }

        output.observation = *observation;
        for (std::size_t index = 0; index < output.samples.size(); ++index)
            if (std::abs (output.samples[index]) > epsilon)
            {
                output.firstNonZeroAbsolute = range.start + static_cast<TimelineSample> (index);
                break;
            }

        return output;
    }

private:
    static constexpr int blockSize = 128;
    std::shared_ptr<RuntimeObservation> observation;
    std::unique_ptr<tracktion::graph::SimpleNodePlayer> player;

    std::unique_ptr<tracktion::graph::Node> buildGraph (const DomainState& domain)
    {
        if (domain.sources.empty())
            throw std::invalid_argument ("A2-2 requires at least one Domain source");

        std::vector<std::unique_ptr<tracktion::graph::Node>> sourceNodes;
        for (const auto& source : domain.sources)
            sourceNodes.push_back (std::make_unique<DomainControlledSourceNode> (source, observation));

        std::unique_ptr<tracktion::graph::Node> graph;
        if (sourceNodes.size() == 1)
            graph = std::move (sourceNodes.front());
        else
            graph = std::make_unique<tracktion::graph::SummingNode> (std::move (sourceNodes));

        graph = appendLayer (std::move (graph), domain.clipLayer);
        graph = appendLayer (std::move (graph), domain.trackLayer);
        return appendLayer (std::move (graph), domain.masterLayer);
    }

    std::unique_ptr<tracktion::graph::Node> appendLayer (std::unique_ptr<tracktion::graph::Node> input,
                                                          const LayerState& layer)
    {
        for (const auto& processor : layer.processors)
            input = std::make_unique<DeterministicProcessorNode> (std::move (input), processor, observation);
        return input;
    }
};

DomainState oneSourceDomain (float value, SignalKind kind = SignalKind::constant, TimelineSample event = 0)
{
    DomainState domain;
    domain.sources = { { "source-a", 48000, kind, value, event } };
    return domain;
}

float sampleAt (const RenderOutput& output, TimelineSample rangeStart, TimelineSample absoluteSample)
{
    return output.samples.at (static_cast<std::size_t> (absoluteSample - rangeStart));
}

void requireNear (float observed, float expected, const char* label)
{
    if (std::abs (observed - expected) > epsilon)
        throw std::runtime_error (std::string (label) + " observed=" + std::to_string (observed)
                                  + " expected=" + std::to_string (expected));
}

void requireOrder (const RenderOutput& output, const std::vector<std::string>& expected)
{
    if (output.observation.requestedProcessorOrder.size() < expected.size()
        || ! std::equal (expected.begin(), expected.end(), output.observation.requestedProcessorOrder.begin()))
        throw std::runtime_error ("Runtime processor order differs from Domain order");
}

void requireTiming (const RenderOutput& output, TimelineSample event)
{
    if (output.firstNonZeroAbsolute != event)
        throw std::runtime_error ("Processor graph moved the event onset");
}

RenderOutput render (const DomainState& domain, RenderRange range)
{
    const auto original = domain;
    LowLevelRuntime runtime (domain);
    auto output = runtime.render (range);
    if (domain != original)
        throw std::runtime_error ("Runtime mutated framework-free Domain State");
    return output;
}

void testOrderedProcessing()
{
    auto forward = oneSourceDomain (0.25f);
    forward.clipLayer.processors = { { "clip-add", ProcessorKind::add, 0.25f },
                                     { "clip-multiply", ProcessorKind::multiply, 2.0f } };
    const auto forwardOutput = render (forward, { 0, 128 });
    requireNear (forwardOutput.samples.front(), 1.0f, "forward order");
    requireOrder (forwardOutput, { "clip-add", "clip-multiply" });

    auto reverse = oneSourceDomain (0.25f);
    reverse.clipLayer.processors = { { "clip-multiply", ProcessorKind::multiply, 2.0f },
                                     { "clip-add", ProcessorKind::add, 0.25f } };
    const auto reverseOutput = render (reverse, { 0, 128 });
    requireNear (reverseOutput.samples.front(), 0.75f, "reverse order");
    requireOrder (reverseOutput, { "clip-multiply", "clip-add" });
    if (forwardOutput.samples.front() == reverseOutput.samples.front())
        throw std::runtime_error ("Forward and reverse orders did not differ");

    std::cout << "A2-2 ordered forward=1 reverse=0.75 order=Domain-to-runtime\n";
}

void testTiming()
{
    auto domain = oneSourceDomain (0.25f, SignalKind::impulse, 1024);
    domain.clipLayer.processors = { { "clip-add-zero", ProcessorKind::add, 0.0f } };
    domain.trackLayer.processors = { { "track-multiply-one", ProcessorKind::multiply, 1.0f } };
    domain.masterLayer.processors = { { "master-add-zero", ProcessorKind::add, 0.0f } };
    const auto output = render (domain, { 0, 4096 });
    requireNear (sampleAt (output, 0, 1024), 0.25f, "timing impulse value");
    requireTiming (output, 1024);
    if (output.alignmentError != -1)
        throw std::runtime_error ("Unexpected stale timing state");
    requireOrder (output, { "clip-add-zero", "track-multiply-one", "master-add-zero" });
    std::cout << "A2-2 timing event=1024 observed=1024 error=0\n";
}

void testLayers()
{
    auto domain = oneSourceDomain (0.25f);
    domain.clipLayer.processors = { { "clip-add", ProcessorKind::add, 0.25f } };
    domain.trackLayer.processors = { { "track-multiply", ProcessorKind::multiply, 2.0f } };
    domain.masterLayer.processors = { { "master-add", ProcessorKind::add, 0.25f } };
    const auto output = render (domain, { 0, 128 });
    requireNear (output.samples.front(), 1.25f, "layered processing");
    requireOrder (output, { "clip-add", "track-multiply", "master-add" });
    std::cout << "A2-2 layers clip-add track-multiply master-add output=1.25\n";
}

void testTrackMix()
{
    DomainState domain;
    domain.sources = { { "source-a", 48000, SignalKind::constant, 0.25f, 0 },
                       { "source-b", 48000, SignalKind::constant, 0.5f, 0 } };
    domain.trackLayer.processors = { { "track-multiply", ProcessorKind::multiply, 2.0f } };
    const auto output = render (domain, { 0, 128 });
    requireNear (output.samples.front(), 1.5f, "public SummingNode Track Mix");
    requireOrder (output, { "track-multiply" });
    std::cout << "A2-2 track-mix public-SummingNode input=0.75 output=1.5\n";
}

void testReconstruction()
{
    auto domain = oneSourceDomain (0.25f, SignalKind::impulse, 1024);
    domain.clipLayer.processors = { { "clip-add-zero", ProcessorKind::add, 0.0f } };
    domain.trackLayer.processors = { { "track-multiply-one", ProcessorKind::multiply, 1.0f } };
    domain.masterLayer.processors = { { "master-add-zero", ProcessorKind::add, 0.0f } };
    const auto first = render (domain, { 0, 4096 });
    const auto second = render (domain, { 0, 4096 });
    if (first.samples != second.samples || first.observation.requestedProcessorOrder != second.observation.requestedProcessorOrder
        || first.observation.requestedTimelineRanges != second.observation.requestedTimelineRanges)
        throw std::runtime_error ("Destroy/rebuild changed processor output or graph observation");
    requireTiming (second, 1024);
    std::cout << "A2-2 reconstruction output/order/requested-ranges/timing=equal error=0\n";
}
}

int main()
{
    try
    {
        const auto started = std::chrono::steady_clock::now();
        a2_2::testOrderedProcessing();
        a2_2::testTiming();
        a2_2::testLayers();
        a2_2::testTrackMix();
        a2_2::testReconstruction();
        const auto elapsed = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - started).count();
        std::cout << "A2-2 PASS elapsed_ms=" << elapsed << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "A2-2 FAIL: " << error.what() << '\n';
        return 1;
    }
}

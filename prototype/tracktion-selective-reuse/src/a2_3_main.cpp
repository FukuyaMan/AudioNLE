#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace a2_3
{
using Sample = std::int64_t;
constexpr float epsilon = 0.000001f;

enum class ProcessorKind { multiply };

// Framework-free authoritative source data. Sparse impulses are sufficient to
// verify exact position mapping without selecting an audio-quality SRC algorithm.
struct SourceEvent
{
    Sample sourceSample = 0;
    float value = 0.0f;

    bool operator== (const SourceEvent&) const = default;
};

struct ProcessorState
{
    std::string id;
    ProcessorKind kind = ProcessorKind::multiply;
    float parameter = 1.0f;
    bool enabled = true;

    bool operator== (const ProcessorState&) const = default;
};

struct DomainState
{
    Sample projectTimelineSampleRate = 48000;
    Sample sourceSampleRate = 44100;
    Sample timelinePositionSamples = 0;
    Sample sourcePositionSamples = 0;
    Sample sourceDurationSamples = 44100;
    Sample timelineDurationSamples = 48000;
    std::vector<SourceEvent> sourceEvents;
    std::vector<ProcessorState> processing;

    bool operator== (const DomainState&) const = default;
};

struct RenderRange
{
    Sample start = 0;
    Sample end = 0;

    bool operator== (const RenderRange&) const = default;
};

struct MappedEvent
{
    Sample sourceSample = 0;
    Sample timelineSample = 0;
    float value = 0.0f;

    bool operator== (const MappedEvent&) const = default;
};

// Exact fixture policy: nearest integer, ties upward, using only checked integers.
Sample mapNearest (Sample index, Sample inputRate, Sample outputRate)
{
    if (index < 0 || inputRate <= 0 || outputRate <= 0)
        throw std::invalid_argument ("Mapping arguments must be non-negative samples and positive rates");
    if (index > (std::numeric_limits<Sample>::max() - inputRate / 2) / outputRate)
        throw std::overflow_error ("Mixed-rate fixture mapping multiplication would overflow int64_t");
    return (index * outputRate + inputRate / 2) / inputRate;
}

Sample mapSourceToTimeline (Sample sourceSample, const DomainState& domain)
{
    return domain.timelinePositionSamples
           + mapNearest (sourceSample - domain.sourcePositionSamples,
                         domain.sourceSampleRate, domain.projectTimelineSampleRate);
}

Sample mapTimelineToSource (Sample timelineSample, const DomainState& domain)
{
    return domain.sourcePositionSamples
           + mapNearest (timelineSample - domain.timelinePositionSamples,
                         domain.projectTimelineSampleRate, domain.sourceSampleRate);
}

struct RuntimeObservation
{
    std::vector<MappedEvent> mappedEvents;
    std::vector<RenderRange> requestedTimelineRanges;
    std::vector<std::string> processorOrder;

    bool operator== (const RuntimeObservation&) const = default;
};

class MappedSourceNode final : public tracktion::graph::Node
{
public:
    MappedSourceNode (DomainState domainToUse, std::shared_ptr<RuntimeObservation> observationToUse)
        : domain (std::move (domainToUse)), observation (std::move (observationToUse))
    {
    }

    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

protected:
    void process (ProcessContext& context) override
    {
        const auto range = RenderRange { context.referenceSampleRange.getStart(), context.referenceSampleRange.getEnd() };
        observation->requestedTimelineRanges.push_back (range);

        for (const auto& event : domain.sourceEvents)
        {
            const auto timelineSample = mapSourceToTimeline (event.sourceSample, domain);
            if (timelineSample < range.start || timelineSample >= range.end)
                continue;

            const auto frame = static_cast<choc::buffer::FrameCount> (timelineSample - range.start);
            context.buffers.audio.getSample (0, frame) += event.value;
            observation->mappedEvents.push_back ({ event.sourceSample, timelineSample, event.value });
        }
        context.buffers.midi.clear();
    }

private:
    DomainState domain;
    std::shared_ptr<RuntimeObservation> observation;
};

class DeterministicMultiplyNode final : public tracktion::graph::Node
{
public:
    DeterministicMultiplyNode (std::unique_ptr<tracktion::graph::Node> inputToUse,
                               ProcessorState processorToUse,
                               std::shared_ptr<RuntimeObservation> observationToUse)
        : ownedInput (std::move (inputToUse)), input (ownedInput.get()), processor (std::move (processorToUse)), observation (std::move (observationToUse))
    {
        if (input == nullptr || processor.kind != ProcessorKind::multiply)
            throw std::invalid_argument ("A2-3 requires a valid Multiply processor input");
    }

    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }

protected:
    void process (ProcessContext& context) override
    {
        const auto source = input->getProcessedOutput();
        choc::buffer::copy (context.buffers.audio, source.audio);
        context.buffers.midi.copyFrom (source.midi);
        observation->processorOrder.push_back (processor.id);

        if (! processor.enabled)
            return;

        for (auto channel = 0; channel < context.buffers.audio.getNumChannels(); ++channel)
            for (auto frame = 0; frame < context.buffers.audio.getNumFrames(); ++frame)
                context.buffers.audio.getSample (channel, frame) *= processor.parameter;
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
};

class LowLevelRuntime final
{
public:
    explicit LowLevelRuntime (const DomainState& domain)
        : observation (std::make_shared<RuntimeObservation>()),
          player (std::make_unique<tracktion::graph::SimpleNodePlayer> (
              buildGraph (domain), static_cast<double> (domain.projectTimelineSampleRate), blockSize))
    {
        validate (domain);
    }

    RenderOutput render (RenderRange range)
    {
        observation->mappedEvents.clear();
        observation->requestedTimelineRanges.clear();
        observation->processorOrder.clear();
        RenderOutput output;
        output.samples.reserve (static_cast<std::size_t> (range.end - range.start));
        choc::buffer::ChannelArrayBuffer<float> blockBuffer;
        blockBuffer.resize ({ 1, static_cast<choc::buffer::FrameCount> (blockSize) });
        tracktion::engine::MidiMessageArray midi;

        for (auto position = range.start; position < range.end;)
        {
            const auto frames = static_cast<int> (std::min<Sample> (blockSize, range.end - position));
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
        return output;
    }

private:
    static constexpr int blockSize = 128;
    std::shared_ptr<RuntimeObservation> observation;
    std::unique_ptr<tracktion::graph::SimpleNodePlayer> player;

    static void validate (const DomainState& domain)
    {
        if (domain.projectTimelineSampleRate != 48000 || domain.sourceSampleRate != 44100
            || domain.sourcePositionSamples < 0 || domain.timelinePositionSamples < 0
            || domain.sourceDurationSamples != 44100 || domain.timelineDurationSamples != 48000)
            throw std::invalid_argument ("A2-3 requires the explicit 44100-source/48000-Timeline one-second fixture");
        for (const auto& event : domain.sourceEvents)
            if (event.sourceSample < domain.sourcePositionSamples
                || event.sourceSample >= domain.sourcePositionSamples + domain.sourceDurationSamples)
                throw std::invalid_argument ("Source event lies outside the Domain source range");
    }

    std::unique_ptr<tracktion::graph::Node> buildGraph (const DomainState& domain)
    {
        validate (domain);
        std::unique_ptr<tracktion::graph::Node> graph = std::make_unique<MappedSourceNode> (domain, observation);
        for (const auto& processor : domain.processing)
            graph = std::make_unique<DeterministicMultiplyNode> (std::move (graph), processor, observation);
        return graph;
    }
};

DomainState mappingFixture()
{
    DomainState domain;
    domain.sourceEvents = {
        { 0, 0.10f }, { 1, 0.11f }, { 17, 0.12f }, { 101, 0.13f }, { 441, 0.14f },
        { 1000, 0.15f }, { 4410, 0.16f }, { 22050, 0.17f }, { 22051, 0.18f }, { 44099, 0.19f },
    };
    return domain;
}

float sampleAt (const RenderOutput& output, Sample sample)
{
    return output.samples.at (static_cast<std::size_t> (sample));
}

void require (bool condition, const char* message)
{
    if (! condition)
        throw std::runtime_error (message);
}

void requireNear (float observed, float expected, const char* message)
{
    if (std::abs (observed - expected) > epsilon)
        throw std::runtime_error (message);
}

RenderOutput render (const DomainState& domain)
{
    const auto before = domain;
    LowLevelRuntime runtime (domain);
    auto output = runtime.render ({ 0, domain.timelineDurationSamples });
    require (domain == before, "Runtime mutated authoritative Domain State");
    return output;
}

void testOneSecondAndAnchors()
{
    const auto domain = mappingFixture();
    require (mapSourceToTimeline (domain.sourceDurationSamples, domain) == domain.timelineDurationSamples,
             "One-second source duration does not map to one-second Timeline duration");
    const auto output = render (domain);
    const std::vector<Sample> expected { 0, 1, 19, 110, 480, 1088, 4800, 24000, 24001, 47999 };
    require (output.observation.mappedEvents.size() == domain.sourceEvents.size(), "Missing mapped source event");

    for (std::size_t index = 0; index < domain.sourceEvents.size(); ++index)
    {
        const auto& sourceEvent = domain.sourceEvents[index];
        const auto mapped = mapSourceToTimeline (sourceEvent.sourceSample, domain);
        require (mapped == expected[index], "Unexpected integer/rational Source-to-Timeline mapping");
        require (mapTimelineToSource (mapped, domain) == sourceEvent.sourceSample, "Canonical inverse mapping changed source anchor");
        requireNear (sampleAt (output, mapped), sourceEvent.value, "Mapped event value differs");
    }
    std::cout << "A2-3 one-second source_duration=44100 timeline_duration=48000 anchors=10\n";
}

void testRoundingRecord()
{
    const auto domain = mappingFixture();
    for (const auto source : { Sample { 1 }, Sample { 17 }, Sample { 101 }, Sample { 1000 }, Sample { 22051 } })
    {
        const auto numerator = source * domain.projectTimelineSampleRate;
        const auto mapped = mapSourceToTimeline (source, domain);
        std::cout << "A2-3 rounding source=" << source << " rational=" << numerator << '/' << domain.sourceSampleRate
                  << " timeline=" << mapped << '\n';
    }
}

void testDeterminism()
{
    const auto domain = mappingFixture();
    const auto reference = render (domain);
    for (int rebuild = 1; rebuild <= 10; ++rebuild)
    {
        const auto observed = render (domain);
        require (observed.samples == reference.samples, "Rebuild output drifted");
        require (observed.observation == reference.observation, "Rebuild mapping observation drifted");
    }
    std::cout << "A2-3 reconstruction count=10 drift=0\n";
}

void testProcessingCompatibility()
{
    auto domain = mappingFixture();
    domain.sourceEvents = { { 22050, 0.25f } };
    domain.processing = { { "mixed-rate-multiply", ProcessorKind::multiply, 2.0f } };
    const auto output = render (domain);
    const auto mapped = mapSourceToTimeline (22050, domain);
    require (mapped == 24000, "Expected mixed-rate processing anchor differs");
    requireNear (sampleAt (output, mapped), 0.5f, "Mixed-rate processed amplitude differs");
    require (output.observation.processorOrder.size() == 375, "Unexpected processing block count");
    require (std::all_of (output.observation.processorOrder.begin(), output.observation.processorOrder.end(),
                          [] (const auto& id) { return id == "mixed-rate-multiply"; }),
             "Runtime processing order differs from Domain order");
    std::cout << "A2-3 processing source=22050 timeline=24000 expected=0.5 observed=0.5 error=0\n";
}
}

int main()
{
    try
    {
        const auto started = std::chrono::steady_clock::now();
        a2_3::testOneSecondAndAnchors();
        a2_3::testRoundingRecord();
        a2_3::testDeterminism();
        a2_3::testProcessingCompatibility();
        const auto elapsed = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - started).count();
        std::cout << "A2-3 PASS elapsed_ms=" << elapsed << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "A2-3 FAIL: " << error.what() << '\n';
        return 1;
    }
}

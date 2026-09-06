#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace a2_1
{
using TimelineSample = std::int64_t;

// This is the authoritative, framework-free fixture. It contains no Tracktion or JUCE type.
struct SourceData
{
    std::string id;
    TimelineSample nativeSampleRate = 48000;
    std::vector<float> samples;

    bool operator== (const SourceData&) const = default;
};

struct DomainState
{
    TimelineSample projectSampleRate = 48000;
    SourceData source { "generated-mono-impulse", 48000, { 1.0f } };
    TimelineSample timelineEventSample = 1024;

    bool operator== (const DomainState&) const = default;
};

struct RenderRange
{
    TimelineSample start = 0;
    TimelineSample end = 0;

    bool operator== (const RenderRange&) const = default;
};

struct SourceObservation
{
    std::vector<RenderRange> requestedTimelineRanges;
};

class DomainControlledSourceNode final : public tracktion::graph::Node
{
public:
    DomainControlledSourceNode (TimelineSample eventSampleToUse,
                                std::vector<float> sourceDataToUse,
                                std::shared_ptr<SourceObservation> observationToUse)
        : eventSample (eventSampleToUse), sourceData (std::move (sourceDataToUse)), observation (std::move (observationToUse))
    {
    }

    tracktion::graph::NodeProperties getNodeProperties() override
    {
        return { true, false, 1, 0, 0 };
    }

    bool isReadyToProcess() override
    {
        return true;
    }

protected:
    void process (ProcessContext& context) override
    {
        const auto requested = RenderRange { context.referenceSampleRange.getStart(), context.referenceSampleRange.getEnd() };
        observation->requestedTimelineRanges.push_back (requested);

        const auto eventInThisBlock = eventSample >= requested.start && eventSample < requested.end;
        const auto eventOffset = eventInThisBlock ? eventSample - requested.start : -1;
        const auto sampleValue = sourceData.empty() ? 0.0f : sourceData.front();

        choc::buffer::setAllFrames (context.buffers.audio, [eventOffset, sampleValue] (auto frame)
        {
            return static_cast<TimelineSample> (frame) == eventOffset ? sampleValue : 0.0f;
        });
        context.buffers.midi.clear();
    }

private:
    TimelineSample eventSample;
    std::vector<float> sourceData;
    std::shared_ptr<SourceObservation> observation;
};

struct RuntimeOutput
{
    std::vector<float> samples;
    std::vector<RenderRange> requestedTimelineRanges;
    TimelineSample firstNonZeroAbsolute = -1;
    TimelineSample lastNonZeroAbsolute = -1;
    TimelineSample alignmentError = -1;
};

class LowLevelRuntime final
{
public:
    explicit LowLevelRuntime (const DomainState& domain)
        : domainEventSample (domain.timelineEventSample),
          observation (std::make_shared<SourceObservation>()),
          player (std::make_unique<tracktion::graph::SimpleNodePlayer> (
              std::make_unique<DomainControlledSourceNode> (domain.timelineEventSample, domain.source.samples, observation),
              static_cast<double> (domain.projectSampleRate), blockSize))
    {
        if (domain.projectSampleRate != domain.source.nativeSampleRate)
            throw std::runtime_error ("A2-1 fixture requires an explicit same-rate source; no Source/Timeline conversion is performed");
    }

    RuntimeOutput render (RenderRange range)
    {
        if (range.end <= range.start)
            throw std::runtime_error ("Render range must be non-empty");

        observation->requestedTimelineRanges.clear();
        RuntimeOutput output;
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

        output.requestedTimelineRanges = observation->requestedTimelineRanges;
        for (std::size_t index = 0; index < output.samples.size(); ++index)
            if (output.samples[index] != 0.0f)
            {
                const auto absolute = range.start + static_cast<TimelineSample> (index);
                if (output.firstNonZeroAbsolute == -1)
                    output.firstNonZeroAbsolute = absolute;
                output.lastNonZeroAbsolute = absolute;
            }

        if (output.firstNonZeroAbsolute != -1)
            output.alignmentError = output.firstNonZeroAbsolute - domainEventSample;

        return output;
    }

private:
    static constexpr int blockSize = 128;
    TimelineSample domainEventSample;
    std::shared_ptr<SourceObservation> observation;
    std::unique_ptr<tracktion::graph::SimpleNodePlayer> player;
};

struct CaseDefinition
{
    TimelineSample event = 0;
    RenderRange range;
};

struct CaseResult
{
    CaseDefinition definition;
    RuntimeOutput output;
    double elapsedMilliseconds = 0.0;
};

CaseResult runCase (CaseDefinition definition)
{
    DomainState domain;
    domain.timelineEventSample = definition.event;
    const auto domainBefore = domain;
    const auto started = std::chrono::steady_clock::now();
    LowLevelRuntime runtime (domain);
    auto output = runtime.render (definition.range);
    const auto ended = std::chrono::steady_clock::now();

    if (domain != domainBefore)
        throw std::runtime_error ("Runtime mutated the authoritative Domain State");
    if (output.firstNonZeroAbsolute != definition.event || output.lastNonZeroAbsolute != definition.event || output.alignmentError != 0)
        throw std::runtime_error ("Custom source alignment error was non-zero");
    if (output.requestedTimelineRanges.empty())
        throw std::runtime_error ("Custom source received no requested Timeline ranges");

    return { definition, std::move (output), std::chrono::duration<double, std::milli> (ended - started).count() };
}

void printResult (const char* label, const CaseResult& result)
{
    std::cout << label
              << " event=" << result.definition.event
              << " render=[" << result.definition.range.start << ',' << result.definition.range.end << ')'
              << " first=" << result.output.firstNonZeroAbsolute
              << " last=" << result.output.lastNonZeroAbsolute
              << " error=" << result.output.alignmentError
              << " requested=";

    for (const auto& requested : result.output.requestedTimelineRanges)
        std::cout << '[' << requested.start << ',' << requested.end << ')';

    std::cout << " elapsed_ms=" << result.elapsedMilliseconds << '\n';
}

void requireEqualOutput (const CaseResult& first, const CaseResult& second)
{
    if (first.output.samples != second.output.samples
        || first.output.requestedTimelineRanges != second.output.requestedTimelineRanges
        || first.output.firstNonZeroAbsolute != second.output.firstNonZeroAbsolute
        || first.output.lastNonZeroAbsolute != second.output.lastNonZeroAbsolute
        || first.output.alignmentError != second.output.alignmentError)
        throw std::runtime_error ("Reconstructed runtime changed the zero-error output or requested ranges");
}
}

int main()
{
    try
    {
        using namespace a2_1;
        const std::vector<CaseDefinition> cases {
            { 1,    { 0, 4096 } },
            { 100,  { 0, 4096 } },
            { 1024, { 0, 4096 } },
            { 2048, { 0, 4096 } },
            { 3000, { 0, 4096 } },
            { 1024, { 512, 4608 } },
            { 1024, { 1000, 5096 } },
        };

        std::vector<CaseResult> results;
        results.reserve (cases.size());
        for (const auto& definition : cases)
        {
            results.push_back (runCase (definition));
            printResult ("A2-1", results.back());
        }

        const auto reconstructed = runCase ({ 1024, { 0, 4096 } });
        requireEqualOutput (results[2], reconstructed);
        printResult ("A2-1 reconstruction", reconstructed);
        std::cout << "A2-1 PASS: custom low-level source node, headless SimpleNodePlayer, zero Timeline-sample error\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "A2-1 FAIL: " << error.what() << '\n';
        return 1;
    }
}

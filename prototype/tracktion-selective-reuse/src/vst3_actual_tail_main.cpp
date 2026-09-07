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

namespace actual_vst3_tail
{
using Sample = std::int64_t;
constexpr double projectRate = 48000.0;
constexpr int blockSize = 128;
constexpr Sample clipStart = 1000;
constexpr Sample sourceDuration = 480;
constexpr Sample sourceEnd = clipStart + sourceDuration;
constexpr Sample fixtureTailSamples = 1024;
constexpr Sample processingEnd = sourceEnd + fixtureTailSamples;
constexpr Sample moveDelta = 48000;
constexpr float sourceAmplitude = 0.5f;
constexpr float tailAmplitude = 0.25f;
constexpr float epsilon = 0.000001f;

enum class TailPolicy { reported, cutAtSourceEnd };

struct ClipState
{
    Sample start = clipStart;
    float amplitude = sourceAmplitude;
    TailPolicy tailPolicy = TailPolicy::reported;
    bool usesVst3 = true;
    bool operator== (const ClipState&) const = default;
};

struct DomainState
{
    std::vector<ClipState> clips;
    bool operator== (const DomainState&) const = default;
};

Sample secondsToTimelineSamples (double seconds)
{
    if (! std::isfinite (seconds) || seconds < 0.0)
        throw std::runtime_error ("fixture tail is not finite and non-negative");
    return static_cast<Sample> (std::llround (seconds * projectRate));
}

std::unique_ptr<juce::AudioPluginInstance> loadPreparedPlugin()
{
    juce::VST3PluginFormatHeadless format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile (descriptions, juce::String (A2_VST3_TAIL_FIXTURE_PATH));
    if (descriptions.isEmpty())
        throw std::runtime_error ("VST3 tail fixture known-path scan returned no description");

    juce::AudioPluginFormatManager manager;
    manager.addFormat (std::make_unique<juce::VST3PluginFormatHeadless>());
    juce::String error;
    auto plugin = manager.createPluginInstance (*descriptions[0], projectRate, blockSize, error);
    if (! plugin)
        throw std::runtime_error ("VST3 tail fixture creation failed: " + error.toStdString());
    plugin->setRateAndBufferSizeDetails (projectRate, blockSize);
    plugin->prepareToPlay (projectRate, blockSize);
    return plugin;
}

class SourceNode final : public tracktion::graph::Node
{
public:
    explicit SourceNode (ClipState clipToUse) : clip (clipToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

private:
    void process (ProcessContext& context) override
    {
        const auto start = clip.start;
        const auto end = start + sourceDuration;
        choc::buffer::setAllFrames (context.buffers.audio, [start, end, this, &context] (auto frame)
        {
            const auto position = context.referenceSampleRange.getStart() + static_cast<Sample> (frame);
            return position >= start && position < end ? clip.amplitude : 0.0f;
        });
        context.buffers.midi.clear();
    }

    ClipState clip;
};

struct PluginInstrumentation
{
    Sample sourceActiveInputSamples = 0;
    Sample postSourceZeroInputSamples = 0;
};

class PluginNode final : public tracktion::graph::Node
{
public:
    PluginNode (std::unique_ptr<tracktion::graph::Node> inputToUse, ClipState clipToUse,
                PluginInstrumentation& instrumentationToUse)
        : inputOwner (std::move (inputToUse)), input (inputOwner.get()), clip (clipToUse),
          instrumentation (instrumentationToUse), plugin (loadPreparedPlugin()) {}

    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }

private:
    void process (ProcessContext& context) override
    {
        const auto source = input->getProcessedOutput();
        const auto frames = static_cast<int> (context.buffers.audio.getNumFrames());
        juce::AudioBuffer<float> buffer (1, frames);
        const auto end = clip.start + sourceDuration;
        for (int frame = 0; frame < frames; ++frame)
        {
            const auto value = source.audio.getSample (0, frame);
            const auto position = context.referenceSampleRange.getStart() + frame;
            buffer.setSample (0, frame, value);
            if (position >= clip.start && position < end)
                ++instrumentation.sourceActiveInputSamples;
            else if (position >= end && std::abs (value) <= epsilon)
                ++instrumentation.postSourceZeroInputSamples;
        }

        if (clip.tailPolicy == TailPolicy::reported || context.referenceSampleRange.getStart() < end)
        {
            juce::MidiBuffer midi;
            plugin->processBlock (buffer, midi);
            if (clip.tailPolicy == TailPolicy::cutAtSourceEnd)
            {
                for (int frame = 0; frame < frames; ++frame)
                    if (context.referenceSampleRange.getStart() + frame >= end)
                        buffer.setSample (0, frame, 0.0f);
            }
        }
        else
        {
            buffer.clear();
        }

        for (int frame = 0; frame < frames; ++frame)
            context.buffers.audio.getSample (0, frame) = buffer.getSample (0, frame);
        context.buffers.midi.clear();
    }

    std::unique_ptr<tracktion::graph::Node> inputOwner;
    tracktion::graph::Node* input;
    ClipState clip;
    PluginInstrumentation& instrumentation;
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
    PluginInstrumentation instrumentation;
    Sample firstTail = -1;
    Sample lastNonZeroTail = -1;
    Sample actualTailSamples = 0;
};

RenderResult render (const DomainState& domain, bool downstreamMultiply = false)
{
    const auto before = domain;
    RenderResult result;
    std::vector<std::unique_ptr<tracktion::graph::Node>> nodes;
    for (const auto& clip : domain.clips)
    {
        std::unique_ptr<tracktion::graph::Node> node = std::make_unique<SourceNode> (clip);
        if (clip.usesVst3)
            node = std::make_unique<PluginNode> (std::move (node), clip, result.instrumentation);
        if (downstreamMultiply)
            node = std::make_unique<MultiplyNode> (std::move (node), 2.0f);
        nodes.push_back (std::move (node));
    }
    if (nodes.empty())
        return result;

    std::unique_ptr<tracktion::graph::Node> root = nodes.size() == 1
        ? std::move (nodes.front()) : std::make_unique<tracktion::graph::SummingNode> (std::move (nodes));
    tracktion::graph::SimpleNodePlayer player (std::move (root), projectRate, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer;
    buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi;
    const auto renderEnd = domain.clips.front().start + sourceDuration + fixtureTailSamples + blockSize;
    for (Sample position = 0; position < renderEnd; position += blockSize)
    {
        auto view = buffer.getView();
        view.clear();
        midi.clear();
        player.process ({ blockSize, juce::Range<Sample>::withStartAndLength (position, blockSize), { view, midi } });
        for (int frame = 0; frame < blockSize; ++frame)
            result.samples.push_back (view.getSample (0, frame));
    }

    const auto tailStart = domain.clips.front().start + sourceDuration;
    for (Sample position = tailStart; position < static_cast<Sample> (result.samples.size()); ++position)
    {
        if (std::abs (result.samples[static_cast<std::size_t> (position)]) > epsilon)
        {
            result.firstTail = result.firstTail == -1 ? position : result.firstTail;
            result.lastNonZeroTail = position;
        }
    }
    result.actualTailSamples = result.firstTail == -1 ? 0 : result.lastNonZeroTail - result.firstTail + 1;
    if (domain != before)
        throw std::runtime_error ("runtime mutated framework-free Domain state");
    return result;
}

void require (bool condition, const char* message)
{
    if (! condition)
        throw std::runtime_error (message);
}

void requireTail (const RenderResult& result, Sample first, Sample last, float amplitude, const char* label)
{
    require (result.firstTail == first && result.lastNonZeroTail == last, label);
    require (std::abs (result.samples.at (static_cast<std::size_t> (first)) - amplitude) <= epsilon, label);
}

void run()
{
    auto direct = loadPreparedPlugin();
    const auto reportedSeconds = direct->getTailLengthSeconds();
    const auto reportedSamples = secondsToTimelineSamples (reportedSeconds);
    require (reportedSamples == fixtureTailSamples, "host-visible VST3 tail report differs from fixture contract");

    const DomainState baseline { { {} } };
    const auto reported = render (baseline);
    requireTail (reported, sourceEnd, processingEnd - 1, tailAmplitude, "reported tail boundary mismatch");
    require (reported.actualTailSamples == fixtureTailSamples, "actual VST3 tail duration mismatch");
    require (reported.instrumentation.sourceActiveInputSamples == sourceDuration, "source-active input count mismatch");
    require (reported.instrumentation.postSourceZeroInputSamples >= fixtureTailSamples, "post-source silence was not fed to VST3");
    require (std::abs (reported.samples.at (static_cast<std::size_t> (processingEnd))) <= epsilon,
             "VST3 tail extends beyond Processing End");

    const auto downstream = render (baseline, true);
    requireTail (downstream, sourceEnd, processingEnd - 1, tailAmplitude * 2.0f, "downstream multiply did not process VST3 tail");

    ClipState laterSource { 1800, sourceAmplitude, TailPolicy::cutAtSourceEnd, false };
    const auto mixed = render ({ { {}, laterSource } });
    require (std::abs (mixed.samples.at (1800) - (tailAmplitude + sourceAmplitude)) <= epsilon,
             "SummingNode did not mix VST3 tail overlap");

    auto moved = baseline;
    moved.clips.front().start += moveDelta;
    const auto movedResult = render (moved);
    requireTail (movedResult, sourceEnd + moveDelta, processingEnd - 1 + moveDelta, tailAmplitude,
                 "move did not shift VST3 tail exactly");

    auto cut = baseline;
    cut.clips.front().tailPolicy = TailPolicy::cutAtSourceEnd;
    const auto cutResult = render (cut);
    require (cutResult.firstTail == -1 && cutResult.actualTailSamples == 0, "CutAtSourceEnd emitted VST3 tail");

    const auto first = render (baseline);
    const auto second = render (baseline);
    require (first.samples == second.samples && first.actualTailSamples == second.actualTailSamples,
             "destroy/rebuild VST3 tail output changed");
    require (render ({ }).samples.empty(), "delete left VST3 output or tail");

    std::cout << "VST3-TAIL reported-seconds=" << reportedSeconds
              << " reported-samples=" << reportedSamples
              << " source-end=" << sourceEnd
              << " processing-end=" << processingEnd
              << " first=" << reported.firstTail
              << " last=" << reported.lastNonZeroTail
              << " actual-samples=" << reported.actualTailSamples
              << " zero-input=" << reported.instrumentation.postSourceZeroInputSamples
              << " overlap=0.75 move=" << moveDelta << " cut=pass rebuild=equal\n";
}
}

int main()
{
    try
    {
        actual_vst3_tail::run();
        std::cout << "VST3-TAIL PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "VST3-TAIL FAIL: " << error.what() << '\n';
        return 1;
    }
}

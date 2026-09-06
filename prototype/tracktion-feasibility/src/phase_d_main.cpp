#include <tracktion_engine/tracktion_engine.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace phase_d
{
using SampleCount = std::int64_t;
using MediaId = std::string;

// This entire value model is framework-free and is the authoritative fixture state.
enum class ProcessorKind { add, multiply };

struct ProcessorState
{
    std::string id;
    ProcessorKind kind = ProcessorKind::add;
    float parameter = 0.0f;
    bool enabled = true;
    bool operator== (const ProcessorState&) const = default;
};

struct MediaState
{
    MediaId id;
    SampleCount sourceNativeSampleRate = 48000;
    std::string sourceLocator;
    bool operator== (const MediaState&) const = default;
};

struct ClipState
{
    std::string id;
    MediaId mediaId;
    SampleCount timelineStartSamples = 0;
    SampleCount timelineDurationSamples = 0;
    SampleCount sourceStartSamples = 0;
    SampleCount sourceDurationSamples = 0;
    std::vector<ProcessorState> processingStack;
    bool operator== (const ClipState&) const = default;
};

struct TrackState
{
    std::string id;
    double gainDb = 0.0;
    double pan = 0.0;
    std::vector<ProcessorState> processingStack;
    std::vector<ClipState> clips;
    bool operator== (const TrackState&) const = default;
};

struct MinimalDomainState
{
    SampleCount projectTimelineSampleRate = 48000;
    std::vector<MediaState> media;
    std::vector<TrackState> tracks;
    std::vector<ProcessorState> masterProcessingStack;
    bool operator== (const MinimalDomainState&) const = default;
};

class DeterministicProcessor final : public tracktion::engine::Plugin
{
public:
    DeterministicProcessor (tracktion::engine::PluginCreationInfo info, std::string domainId,
                            ProcessorKind processorKind, float processorParameter)
        : Plugin (info), id (std::move (domainId)), kind (processorKind), parameter (processorParameter)
    {
    }

    juce::String getName() const override { return "Phase D deterministic processor"; }
    juce::String getPluginType() override { return "audionle.phase_d.deterministic"; }
    juce::String getSelectableDescription() override { return getName(); }
    void initialise (const tracktion::engine::PluginInitialisationInfo&) override {}
    void deinitialise() override {}
    int getNumOutputChannelsGivenInputs (int inputs) override { return inputs; }
    BusLayout getBusses() const override { return BusLayout::singlePassThrough(); }
    bool producesAudioWhenNoAudioInput() override { return false; }

    void applyToBuffer (const tracktion::engine::PluginRenderContext& context) override
    {
        if (context.destBuffer == nullptr)
            return;
        for (int channel = 0; channel < context.destBuffer->getNumChannels(); ++channel)
        {
            auto* output = context.destBuffer->getWritePointer (channel, context.bufferStartSample);
            for (int sample = 0; sample < context.bufferNumSamples; ++sample)
                output[sample] = kind == ProcessorKind::add ? output[sample] + parameter : output[sample] * parameter;
        }
    }

    std::string id;
    ProcessorKind kind;
    float parameter;
};

class PhaseDEngineBehaviour final : public tracktion::engine::EngineBehaviour
{
public:
    tracktion::engine::Plugin::Ptr createCustomPlugin (tracktion::engine::PluginCreationInfo info) override
    {
        if (info.state[tracktion::engine::IDs::type].toString() != "audionle.phase_d.deterministic")
            return {};

        const auto kind = info.state["phaseDKind"].toString() == "multiply"
                            ? ProcessorKind::multiply : ProcessorKind::add;
        return new DeterministicProcessor (info, info.state["phaseDId"].toString().toStdString(), kind,
                                           static_cast<float> (info.state["phaseDParameter"]));
    }
};

class SourceRegistry
{
public:
    explicit SourceRegistry (std::map<MediaId, std::filesystem::path> values) : sources (std::move (values)) {}

    const std::filesystem::path& resolve (const MediaId& id) const
    {
        if (const auto it = sources.find (id); it != sources.end()) return it->second;
        throw std::runtime_error ("Missing source mapping: " + id);
    }

    juce::File resolveRuntimePath (const juce::String& locator) const
    {
        const auto requested = std::filesystem::path (locator.toStdString());
        for (const auto& [id, path] : sources)
            if (path.filename() == requested.filename()) return juce::File (path.string());
        return {};
    }

private:
    std::map<MediaId, std::filesystem::path> sources;
};

class FloatCapture final : public juce::AudioFormatWriter::ThreadedWriter::IncomingDataReceiver
{
public:
    void reset (int channels, double rate, std::int64_t totalSamples) override
    {
        sampleRate = rate;
        buffer.setSize (channels, static_cast<int> (totalSamples));
        buffer.clear();
    }
    void addBlock (std::int64_t sampleNumber, const juce::AudioBuffer<float>& data, int start, int count) override
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.copyFrom (channel, static_cast<int> (sampleNumber), data, channel, start, count);
    }
    juce::AudioBuffer<float> buffer;
    double sampleRate = 0.0;
};

struct Runtime
{
    Runtime() : juceInitialiser(), engine ("AudioNLE Tracktion feasibility Phase D", nullptr,
                                           std::make_unique<PhaseDEngineBehaviour>()) {}
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    tracktion::engine::Engine engine;
    std::unique_ptr<tracktion::engine::Edit> edit;
};

struct RuntimeProcessorObservation
{
    std::string id;
    ProcessorKind kind;
    float parameter;
    bool operator== (const RuntimeProcessorObservation&) const = default;
};

class Adapter
{
public:
    explicit Adapter (const SourceRegistry& registry) : sources (registry) {}

    std::unique_ptr<Runtime> construct (const MinimalDomainState& domain) const
    {
        validate (domain);
        auto runtime = std::make_unique<Runtime>();
        const auto editFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                  .getChildFile ("audionle-phase-d-runtime.tracktionedit");
        runtime->edit = tracktion::engine::createEmptyEdit (runtime->engine, editFile);
        runtime->edit->filePathResolver = [this] (const juce::String& locator) { return sources.resolveRuntimePath (locator); };
        runtime->edit->ensureNumberOfAudioTracks (1);
        runtime->edit->getMasterVolumePlugin()->setVolumeDb (0.0f);
        auto* track = tracktion::engine::getAudioTracks (*runtime->edit)[0];
        track->getVolumePlugin()->setVolumeDb (0.0f);
        track->getVolumePlugin()->setPan (0.0f);

        for (const auto& clip : domain.tracks.front().clips)
        {
            const tracktion::engine::ClipPosition position {
                { toPosition (clip.timelineStartSamples, domain.projectTimelineSampleRate),
                  toDuration (clip.timelineDurationSamples, domain.projectTimelineSampleRate) },
                toDuration (clip.sourceStartSamples, domain.projectTimelineSampleRate) };
            auto runtimeClip = track->insertWaveClip (clip.id, juce::File (sources.resolve (clip.mediaId).string()), position, false);
            if (runtimeClip == nullptr || runtimeClip->getPluginList() == nullptr)
                throw std::runtime_error ("Could not construct Clip Processing Stack");
            mapStack (*runtimeClip->getPluginList(), clip.processingStack);
        }
        mapStack (track->pluginList, domain.tracks.front().processingStack);
        mapStack (runtime->edit->getMasterPluginList(), domain.masterProcessingStack);
        return runtime;
    }

    std::vector<RuntimeProcessorObservation> observe (const tracktion::engine::PluginList& plugins) const
    {
        std::vector<RuntimeProcessorObservation> result;
        for (auto* plugin : plugins)
            if (const auto* deterministic = dynamic_cast<const DeterministicProcessor*> (plugin))
                result.push_back ({ deterministic->id, deterministic->kind, deterministic->parameter });
        return result;
    }

    FloatCapture render (Runtime& runtime, SampleCount samples, SampleCount sampleRate) const
    {
        juce::TemporaryFile output (".wav");
        tracktion::engine::Renderer::Parameters parameters (*runtime.edit);
        parameters.destFile = output.getFile();
        parameters.audioFormat = runtime.engine.getAudioFileFormatManager().getWavFormat();
        parameters.sampleRateForAudio = static_cast<double> (sampleRate);
        parameters.blockSizeForAudio = 128;
        parameters.bitDepth = 32;
        parameters.time = parameters.time.withLength (toDuration (samples, sampleRate));
        parameters.usePlugins = true;
        parameters.useMasterPlugins = true;
        FloatCapture capture;
        tracktion::engine::Renderer::RenderTask task ("Phase D headless render", parameters, nullptr, &capture);
        while (task.runJob() == juce::ThreadPoolJob::jobNeedsRunningAgain) {}
        if (task.errorMessage.isNotEmpty() || capture.buffer.getNumSamples() != samples)
            throw std::runtime_error ("Headless render did not produce requested float range");
        return capture;
    }

private:
    static juce::ValueTree pluginState (const ProcessorState& processor)
    {
        auto state = juce::ValueTree (tracktion::engine::IDs::PLUGIN);
        state.setProperty (tracktion::engine::IDs::type, "audionle.phase_d.deterministic", nullptr);
        state.setProperty ("phaseDId", juce::String (processor.id), nullptr);
        state.setProperty ("phaseDKind", processor.kind == ProcessorKind::add ? "add" : "multiply", nullptr);
        state.setProperty ("phaseDParameter", processor.parameter, nullptr);
        return state;
    }
    static void mapStack (tracktion::engine::PluginList& destination, const std::vector<ProcessorState>& stack)
    {
        for (const auto& processor : stack)
        {
            auto runtimeProcessor = destination.insertPlugin (pluginState (processor), -1);
            if (runtimeProcessor == nullptr)
                throw std::runtime_error ("Could not map deterministic Processor into runtime Stack");
            runtimeProcessor->setEnabled (processor.enabled);
        }
    }
    static tracktion::core::TimePosition toPosition (SampleCount samples, SampleCount rate)
    { return tracktion::core::TimePosition::fromSeconds (static_cast<double> (samples) / rate); }
    static tracktion::core::TimeDuration toDuration (SampleCount samples, SampleCount rate)
    { return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (samples) / rate); }
    static void validate (const MinimalDomainState& domain)
    {
        if (domain.projectTimelineSampleRate != 48000 || domain.media.size() != 1 || domain.tracks.size() != 1)
            throw std::runtime_error ("Phase D requires a single 48 kHz source and Track");
        for (const auto& clip : domain.tracks.front().clips)
            if (clip.timelineDurationSamples <= 0 || clip.sourceDurationSamples != clip.timelineDurationSamples)
                throw std::runtime_error ("Incomplete Clip fixture");
    }
    const SourceRegistry& sources;
};

void require (bool condition, const std::string& message) { if (! condition) throw std::runtime_error (message); }
void expectNear (float actual, float expected, const std::string& label)
{
    require (std::abs (actual - expected) <= 0.0002f, label + " expected " + std::to_string (expected)
             + ", observed " + std::to_string (actual));
}

juce::File createSource()
{
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("phase-d-quarter.wav");
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    require (stream != nullptr, "Could not create source WAV");
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.release(), 48000.0, 1, 32, {}, 0));
    require (writer != nullptr, "Could not create source WAV writer");
    juce::AudioBuffer<float> buffer (1, 256);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        buffer.setSample (0, sample, 0.25f);
    require (writer->writeFromAudioSampleBuffer (buffer, 0, 256), "Could not write source WAV");
    return file;
}

ProcessorState add (std::string id = "add") { return { std::move (id), ProcessorKind::add, 0.25f, true }; }
ProcessorState multiply (std::string id = "multiply") { return { std::move (id), ProcessorKind::multiply, 2.0f, true }; }
ClipState clip (std::string id, std::vector<ProcessorState> stack = {})
{ return { std::move (id), "quarter", 100, 64, 0, 64, std::move (stack) }; }
MinimalDomainState fixture (std::vector<ProcessorState> clipStack, std::vector<ProcessorState> trackStack,
                            std::vector<ProcessorState> masterStack, int clipCount = 1)
{
    std::vector<ClipState> clips;
    for (int i = 0; i < clipCount; ++i) clips.push_back (clip ("clip-" + std::to_string (i), clipStack));
    return { 48000, {{ "quarter", 48000, "phase-d-quarter.wav" }},
             {{ "track", 0.0, 0.0, std::move (trackStack), std::move (clips) }}, std::move (masterStack) };
}
float renderedSample (const FloatCapture& capture) { return capture.buffer.getSample (0, 120); }
void expectStack (const std::vector<RuntimeProcessorObservation>& actual, const std::vector<ProcessorState>& expected,
                  const std::string& layer)
{
    require (actual.size() == expected.size(), layer + " runtime processor count differs from Domain State");
    for (size_t i = 0; i < expected.size(); ++i)
        require (actual[i].id == expected[i].id && actual[i].kind == expected[i].kind
                     && std::abs (actual[i].parameter - expected[i].parameter) <= 0.000001f,
                 layer + " runtime processor order, identity, or parameter differs from Domain State");
}

struct RenderedRuntime { FloatCapture output; std::vector<RuntimeProcessorObservation> clip, track, master; };
RenderedRuntime renderAndObserve (const Adapter& adapter, const MinimalDomainState& domain)
{
    const auto before = domain;
    auto runtime = adapter.construct (domain);
    auto* track = tracktion::engine::getAudioTracks (*runtime->edit)[0];
    auto* runtimeClip = dynamic_cast<tracktion::engine::WaveAudioClip*> (track->getClips()[0]);
    require (runtimeClip != nullptr, "Could not observe runtime Clip");
    RenderedRuntime result { adapter.render (*runtime, 256, domain.projectTimelineSampleRate),
                             adapter.observe (*runtimeClip->getPluginList()), adapter.observe (track->pluginList),
                             adapter.observe (runtime->edit->getMasterPluginList()) };
    runtime.reset();
    require (domain == before, "Runtime construction/render mutated authoritative Domain State");
    return result;
}
} // namespace phase_d

int main()
{
    try
    {
        const auto source = phase_d::createSource();
        const phase_d::SourceRegistry registry {{ { "quarter", source.getFullPathName().toStdString() } }};
        const phase_d::Adapter adapter (registry);

        const auto clipForwardState = phase_d::fixture ({ phase_d::add(), phase_d::multiply() }, {}, {});
        const auto clipForward = phase_d::renderAndObserve (adapter, clipForwardState);
        phase_d::expectStack (clipForward.clip, clipForwardState.tracks[0].clips[0].processingStack, "Clip");
        phase_d::expectNear (phase_d::renderedSample (clipForward.output), 1.0f, "Clip Add then Multiply");

        const auto clipReverseState = phase_d::fixture ({ phase_d::multiply(), phase_d::add() }, {}, {});
        const auto clipReverse = phase_d::renderAndObserve (adapter, clipReverseState);
        phase_d::expectStack (clipReverse.clip, clipReverseState.tracks[0].clips[0].processingStack, "Clip reorder");
        phase_d::expectNear (phase_d::renderedSample (clipReverse.output), 0.75f, "Clip Multiply then Add");

        const auto trackState = phase_d::fixture ({}, { phase_d::add(), phase_d::multiply() }, {}, 2);
        const auto track = phase_d::renderAndObserve (adapter, trackState);
        phase_d::expectStack (track.track, trackState.tracks[0].processingStack, "Track");
        phase_d::expectNear (phase_d::renderedSample (track.output), 1.5f, "Track Add then Multiply after Track Mix");

        const auto masterState = phase_d::fixture ({}, {}, { phase_d::add(), phase_d::multiply() });
        const auto master = phase_d::renderAndObserve (adapter, masterState);
        phase_d::expectStack (master.master, masterState.masterProcessingStack, "Master");
        phase_d::expectNear (phase_d::renderedSample (master.output), 1.0f, "Master Add then Multiply");

        const auto layeredState = phase_d::fixture ({ phase_d::add ("clip-add") }, { phase_d::multiply ("track-multiply") },
                                                     { phase_d::add ("master-add") });
        const auto layered = phase_d::renderAndObserve (adapter, layeredState);
        phase_d::expectStack (layered.clip, layeredState.tracks[0].clips[0].processingStack, "Layered Clip");
        phase_d::expectStack (layered.track, layeredState.tracks[0].processingStack, "Layered Track");
        phase_d::expectStack (layered.master, layeredState.masterProcessingStack, "Layered Master");
        phase_d::expectNear (phase_d::renderedSample (layered.output), 1.25f, "Clip then Track then Master layer order");

        const auto reconstructed = phase_d::renderAndObserve (adapter, clipForwardState);
        phase_d::expectStack (reconstructed.clip, clipForwardState.tracks[0].clips[0].processingStack, "Reconstructed Clip");
        phase_d::expectNear (phase_d::renderedSample (reconstructed.output), phase_d::renderedSample (clipForward.output),
                             "Destroyed and reconstructed runtime");

        std::cout << "PHASE_D_PASS clip_forward=" << phase_d::renderedSample (clipForward.output)
                  << " clip_reordered=" << phase_d::renderedSample (clipReverse.output)
                  << " track=" << phase_d::renderedSample (track.output)
                  << " master=" << phase_d::renderedSample (master.output)
                  << " layered=" << phase_d::renderedSample (layered.output)
                  << " reconstruction=" << phase_d::renderedSample (reconstructed.output) << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PHASE_D_FAIL " << error.what() << '\n';
        return 1;
    }
}

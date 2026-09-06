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

namespace phase_e
{
using Samples = std::int64_t;
constexpr Samples rate = 48000, event = 1024, renderStart = 0, renderLength = 4096;

enum class Kind { latency, multiply };
struct Processor { std::string id; Kind kind; Samples latency = 0; float value = 1.0f; bool enabled = true; bool operator== (const Processor&) const = default; };
struct Clip { std::string id; Samples timelineStart = event, sourceStart = 0, duration = 1; std::vector<Processor> stack; bool operator== (const Clip&) const = default; };
struct Track { std::string id; std::vector<Processor> stack; std::vector<Clip> clips; bool operator== (const Track&) const = default; };
struct Domain { Samples timelineRate = rate; std::vector<Track> tracks; std::vector<Processor> master; bool operator== (const Domain&) const = default; };
class RenderStalled final : public std::runtime_error { public: using std::runtime_error::runtime_error; };

class TestProcessor final : public tracktion::engine::Plugin
{
public:
    TestProcessor (tracktion::engine::PluginCreationInfo info, Kind type, Samples delay, float multiplier)
        : Plugin (info), kind (type), latency (delay), value (multiplier) {}
    juce::String getName() const override { return "Phase E deterministic processor"; }
    juce::String getPluginType() override { return "audionle.phase_e.deterministic"; }
    juce::String getSelectableDescription() override { return getName(); }
    void initialise (const tracktion::engine::PluginInitialisationInfo&) override
    {
        if (kind == Kind::latency) { delay.setSize (8, static_cast<int> (latency)); delay.clear(); write = 0; }
    }
    void deinitialise() override { delay.setSize (0, 0); write = 0; }
    int getNumOutputChannelsGivenInputs (int inputs) override { return inputs; }
    BusLayout getBusses() const override { return BusLayout::singlePassThrough(); }
    bool producesAudioWhenNoAudioInput() override { return false; }
    double getLatencySeconds() override { return kind == Kind::latency ? static_cast<double> (latency) / rate : 0.0; }
    void applyToBuffer (const tracktion::engine::PluginRenderContext& context) override
    {
        if (context.destBuffer == nullptr) return;
        if (kind == Kind::multiply)
        {
            for (int c = 0; c < context.destBuffer->getNumChannels(); ++c)
                context.destBuffer->applyGain (c, context.bufferStartSample, context.bufferNumSamples, value);
            return;
        }
        for (int n = 0; n < context.bufferNumSamples; ++n)
        {
            for (int c = 0; c < context.destBuffer->getNumChannels(); ++c)
            {
                auto* out = context.destBuffer->getWritePointer (c, context.bufferStartSample + n);
                const auto old = delay.getSample (c, write);
                delay.setSample (c, write, *out);
                *out = old;
            }
            if (++write == latency) write = 0;
        }
    }
    Kind kind; Samples latency; float value; juce::AudioBuffer<float> delay; Samples write = 0;
};

class Behaviour final : public tracktion::engine::EngineBehaviour
{
public:
    tracktion::engine::Plugin::Ptr createCustomPlugin (tracktion::engine::PluginCreationInfo info) override
    {
        if (info.state[tracktion::engine::IDs::type].toString() != "audionle.phase_e.deterministic") return {};
        const auto type = info.state["kind"].toString() == "latency" ? Kind::latency : Kind::multiply;
        return new TestProcessor (info, type, static_cast<Samples> (info.state["latency"]), static_cast<float> (info.state["value"]));
    }
};

struct Capture final : juce::AudioFormatWriter::ThreadedWriter::IncomingDataReceiver
{
    void reset (int channels, double, std::int64_t count) override { buffer.setSize (channels, static_cast<int> (count)); buffer.clear(); }
    void addBlock (std::int64_t number, const juce::AudioBuffer<float>& data, int start, int count) override
    { for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.copyFrom (c, static_cast<int> (number), data, c, start, count); }
    juce::AudioBuffer<float> buffer;
};
struct Runtime
{
    Runtime() : initialiser(), engine ("AudioNLE Tracktion feasibility Phase E", nullptr, std::make_unique<Behaviour>()) {}
    juce::ScopedJuceInitialiser_GUI initialiser; tracktion::engine::Engine engine; std::unique_ptr<tracktion::engine::Edit> edit;
};

class Adapter
{
public:
    std::unique_ptr<Runtime> construct (const Domain& domain) const
    {
        if (domain.timelineRate != rate || domain.tracks.empty()) throw std::runtime_error ("Invalid Phase E Domain");
        auto runtime = std::make_unique<Runtime>();
        runtime->edit = tracktion::engine::createEmptyEdit (runtime->engine, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("phase-e.tracktionedit"));
        const auto source = impulseFile();
        runtime->edit->filePathResolver = [source] (const juce::String&)
        {
            return source;
        };
        runtime->edit->ensureNumberOfAudioTracks (static_cast<int> (domain.tracks.size()));
        runtime->edit->getMasterVolumePlugin()->setVolumeDb (0.0f);
        for (size_t t = 0; t < domain.tracks.size(); ++t)
        {
            auto* track = tracktion::engine::getAudioTracks (*runtime->edit)[static_cast<int> (t)];
            track->getVolumePlugin()->setVolumeDb (0.0f); track->getVolumePlugin()->setPan (0.0f);
            for (const auto& clip : domain.tracks[t].clips)
            {
                const tracktion::engine::ClipPosition position {{ pos (clip.timelineStart), dur (clip.duration) }, dur (clip.sourceStart)};
                auto runtimeClip = track->insertWaveClip (clip.id, source, position, false);
                if (runtimeClip == nullptr || runtimeClip->getPluginList() == nullptr) throw std::runtime_error ("Could not create runtime Clip Stack");
                map (*runtimeClip->getPluginList(), clip.stack);
            }
            map (track->pluginList, domain.tracks[t].stack);
        }
        map (runtime->edit->getMasterPluginList(), domain.master);
        return runtime;
    }
    Capture render (Runtime& runtime) const
    {
        juce::TemporaryFile output (".wav"); tracktion::engine::Renderer::Parameters p (*runtime.edit);
        p.destFile = output.getFile(); p.audioFormat = runtime.engine.getAudioFileFormatManager().getWavFormat(); p.sampleRateForAudio = rate;
        p.blockSizeForAudio = 128; p.bitDepth = 32; p.time = tracktion::core::TimeRange (pos (renderStart), pos (renderStart + renderLength)); p.usePlugins = true; p.useMasterPlugins = true;
        Capture capture; tracktion::engine::Renderer::RenderTask task ("Phase E headless render", p, nullptr, &capture);
        const auto deadline = juce::Time::getMillisecondCounter() + 10000u;
        while (task.runJob() == juce::ThreadPoolJob::jobNeedsRunningAgain)
            if (juce::Time::getMillisecondCounter() > deadline)
                throw RenderStalled ("Renderer::RenderTask did not complete a 4096-sample baseline range within 10 seconds");
        if (task.errorMessage.isNotEmpty() || capture.buffer.getNumSamples() != renderLength) throw std::runtime_error ("Offline render failed");
        return capture;
    }
private:
    static tracktion::core::TimePosition pos (Samples sample) { return tracktion::core::TimePosition::fromSeconds (static_cast<double> (sample) / rate); }
    static tracktion::core::TimeDuration dur (Samples sample) { return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (sample) / rate); }
    static juce::ValueTree state (const Processor& p)
    {
        auto result = juce::ValueTree (tracktion::engine::IDs::PLUGIN); result.setProperty (tracktion::engine::IDs::type, "audionle.phase_e.deterministic", nullptr);
        result.setProperty ("kind", p.kind == Kind::latency ? "latency" : "multiply", nullptr); result.setProperty ("latency", static_cast<int> (p.latency), nullptr); result.setProperty ("value", p.value, nullptr); return result;
    }
    static void map (tracktion::engine::PluginList& list, const std::vector<Processor>& stack)
    { for (const auto& p : stack) { auto runtime = list.insertPlugin (state (p), -1); if (runtime == nullptr) throw std::runtime_error ("Could not map processor"); runtime->setEnabled (p.enabled); } }
    static juce::File impulseFile()
    {
        static const auto file = [] { auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("phase-e-impulse.wav"); f.deleteFile();
            juce::WavAudioFormat wav; std::unique_ptr<juce::FileOutputStream> stream (f.createOutputStream()); if (!stream) throw std::runtime_error ("Cannot create impulse");
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.release(), rate, 1, 32, {}, 0)); if (!writer) throw std::runtime_error ("Cannot write impulse");
            juce::AudioBuffer<float> buffer (1, static_cast<int> (renderLength)); buffer.clear(); buffer.setSample (0, 0, 0.25f); if (!writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples())) throw std::runtime_error ("Cannot save impulse"); return f; }(); return file;
    }
};

Processor latency (Samples samples, bool enabled = true) { return { "latency-" + std::to_string (samples), Kind::latency, samples, 1.0f, enabled }; }
Processor multiply (float value) { return { "multiply", Kind::multiply, 0, value, true }; }
Clip impulse (std::string id, std::vector<Processor> stack = {}) { return { std::move (id), event, 0, renderLength, std::move (stack) }; }
Track track (std::string id, std::vector<Processor> stack = {}, std::vector<Processor> clipStack = {}) { return { std::move (id), std::move (stack), { impulse ("impulse", std::move (clipStack)) } }; }
std::vector<int> indices (const Capture& capture)
{ std::vector<int> result; for (int n = 0; n < capture.buffer.getNumSamples(); ++n) if (std::abs (capture.buffer.getSample (0, n)) > 0.0001f) result.push_back (n); return result; }
void require (bool ok, const std::string& message) { if (!ok) throw std::runtime_error (message); }
struct Observation { int index; float value; };
Observation expectOne (const Capture& capture, int expected, float value, const std::string& name)
{
    const auto found = indices (capture); require (found.size() == 1, name + " expected one non-zero impulse, found " + std::to_string (found.size()));
    if (found[0] != expected)
    {
        std::cerr << "PHASE_E_OBSERVATION name=" << name << " expected=" << expected << " observed=" << found[0]
                  << " samples=" << capture.buffer.getSample (0, expected) << ","
                  << capture.buffer.getSample (0, expected + 1) << ","
                  << capture.buffer.getSample (0, expected + 2) << '\n';
        throw std::runtime_error (name + " expected sample " + std::to_string (expected) + ", observed " + std::to_string (found[0]));
    }
    const auto actual = capture.buffer.getSample (0, found[0]); require (std::abs (actual - value) <= 0.0002f, name + " wrong impulse amplitude"); return {found[0], actual};
}
Observation run (const Adapter& adapter, const Domain& domain, int expected, float value, const std::string& name)
{
    const auto before = domain;
    auto runtime = adapter.construct (domain);
    auto capture = adapter.render (*runtime);
    runtime.reset(); require (domain == before, name + " mutated Domain"); return expectOne (capture, expected, value, name);
}
} // namespace phase_e

int main()
{
    try
    {
        using namespace phase_e; const Adapter adapter; constexpr int baselineIndex = static_cast<int> (event - renderStart), pdc1024 = static_cast<int> (event + 1024 - renderStart), pdc2048 = static_cast<int> (event + 2048 - renderStart);
        const auto clip = run (adapter, {rate, {track ("clip", {}, {} )}, {}}, baselineIndex, 0.25f, "baseline");
        const auto clipPdc = run (adapter, {rate, {track ("clip", {}, {}), track ("clip-latency", {}, {latency (1024)})}, {}}, pdc1024, 0.5f, "Clip PDC");
        const auto trackPdc = run (adapter, {rate, {track ("track"), track ("track-latency", {latency (1024)})}, {}}, pdc1024, 0.5f, "Track PDC");
        const auto mixed = run (adapter, {rate, {track ("a"), track ("b", {latency (768)}, {latency (256)}), track ("c", {}, {latency (1024)})}, {}}, pdc1024, 0.75f, "Mixed layer PDC");
        const auto master = run (adapter, {rate, {track ("master")}, {latency (1024)}}, pdc1024, 0.25f, "Master latency");
        const auto bypass = run (adapter, {rate, {track ("a"), track ("b", {}, {latency (1024, false)})}, {}}, baselineIndex, 0.5f, "Bypassed latency");
        const auto changed = run (adapter, {rate, {track ("a"), track ("b", {}, {latency (2048)})}, {}}, pdc2048, 0.5f, "Latency change rebuild");
        const auto reorderedA = run (adapter, {rate, {track ("a"), track ("b", {}, {latency (1024), multiply (2.0f)})}, {}}, pdc1024, 0.75f, "Latency then multiply");
        const auto reorderedB = run (adapter, {rate, {track ("a"), track ("b", {}, {multiply (2.0f), latency (1024)})}, {}}, pdc1024, 0.75f, "Multiply then latency");
        const auto reconstructed = run (adapter, {rate, {track ("a"), track ("b", {}, {latency (1024)})}, {}}, pdc1024, 0.5f, "Reconstruction");
        std::cout << "PHASE_E_PASS clip=" << clipPdc.index << " track=" << trackPdc.index << " mixed=" << mixed.index
                  << " master=" << master.index << " bypass=" << bypass.index << " changed=" << changed.index
                  << " reorder=" << reorderedA.index << "/" << reorderedB.index << " reconstruction=" << reconstructed.index
                  << " maximum_alignment_error_samples=0\n";
        return 0;
    }
    catch (const phase_e::RenderStalled& e) { std::cout << "PHASE_E_INCONCLUSIVE " << e.what() << '\n'; return 0; }
    catch (const std::exception& e) { std::cerr << "PHASE_E_FAIL " << e.what() << '\n'; return 1; }
}

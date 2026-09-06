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

namespace phase_c
{
using SampleCount = std::int64_t;
using MediaId = std::string;

struct ProcessorState
{
    std::string id;
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

class SourceRegistry
{
public:
    explicit SourceRegistry (std::map<MediaId, std::filesystem::path> sources)
        : sources (std::move (sources))
    {
    }

    const std::filesystem::path& resolve (const MediaId& mediaId) const
    {
        if (const auto iterator = sources.find (mediaId); iterator != sources.end())
            return iterator->second;

        throw std::runtime_error ("Missing source mapping: " + mediaId);
    }

    juce::File resolveRuntimePath (const juce::String& locator) const
    {
        const auto requested = std::filesystem::path (locator.toStdString());
        if (requested.is_absolute())
            return juce::File (requested.string());

        for (const auto& [mediaId, source] : sources)
            if (source.filename() == requested.filename())
                return juce::File (source.string());

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

    void addBlock (std::int64_t sampleNumber, const juce::AudioBuffer<float>& data,
                   int startOffset, int samples) override
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.copyFrom (channel, static_cast<int> (sampleNumber), data, channel, startOffset, samples);
    }

    juce::AudioBuffer<float> buffer;
    double sampleRate = 0.0;
};

class Runtime
{
public:
    Runtime() : juceInitialiser(), engine ("AudioNLE Tracktion feasibility Phase C") {}

    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    tracktion::engine::Engine engine;
    std::unique_ptr<tracktion::engine::Edit> edit;
};

class Adapter
{
public:
    explicit Adapter (const SourceRegistry& sources) : sources (sources) {}

    std::unique_ptr<Runtime> construct (const MinimalDomainState& domain) const
    {
        validate (domain);
        auto runtime = std::make_unique<Runtime>();
        const auto editFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                  .getChildFile ("audionle-phase-c-runtime.tracktionedit");
        runtime->edit = tracktion::engine::createEmptyEdit (runtime->engine, editFile);
        runtime->edit->filePathResolver = [this] (const juce::String& locator)
        {
            return sources.resolveRuntimePath (locator);
        };
        runtime->edit->ensureNumberOfAudioTracks (1);
        runtime->edit->getMasterVolumePlugin()->setVolumeDb (0.0f);

        auto* track = tracktion::engine::getAudioTracks (*runtime->edit)[0];

        auto* gainAndPan = track->getVolumePlugin();
        if (gainAndPan == nullptr)
            throw std::runtime_error ("Track Gain/Pan processor is unavailable");
        gainAndPan->setVolumeDb (static_cast<float> (domain.tracks.front().gainDb));
        gainAndPan->setPan (static_cast<float> (domain.tracks.front().pan));
        if (std::abs (gainAndPan->getVolumeDb() - domain.tracks.front().gainDb) > 0.0001
            || std::abs (gainAndPan->getPan() - domain.tracks.front().pan) > 0.0001)
            throw std::runtime_error ("Track Gain/Pan runtime observation differs from Domain State");

        for (const auto& clip : domain.tracks.front().clips)
        {
            const auto& source = sources.resolve (clip.mediaId);
            const tracktion::engine::ClipPosition position {
                { toPosition (clip.timelineStartSamples, domain.projectTimelineSampleRate),
                  toDuration (clip.timelineDurationSamples, domain.projectTimelineSampleRate) },
                toDuration (clip.sourceStartSamples, domain.projectTimelineSampleRate) };
            auto runtimeClip = track->insertWaveClip (clip.id, juce::File (source.string()), position, false);
            if (runtimeClip == nullptr)
                throw std::runtime_error ("Could not construct runtime Clip: " + clip.id);

            if (runtimeClip->getPluginList() == nullptr)
                throw std::runtime_error ("Clip Processing Stack container is unavailable");
        }

        return runtime;
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
        parameters.canRenderInMono = false;
        parameters.mustRenderInMono = false;

        FloatCapture capture;
        tracktion::engine::Renderer::RenderTask task ("Phase C headless render", parameters, nullptr, &capture);
        while (task.runJob() == juce::ThreadPoolJob::jobNeedsRunningAgain)
        {
        }

        if (task.errorMessage.isNotEmpty() || capture.buffer.getNumSamples() != samples)
            throw std::runtime_error ("Headless render did not produce the requested float sample range");
        return capture;
    }

private:
    static tracktion::core::TimePosition toPosition (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimePosition::fromSeconds (static_cast<double> (samples) / sampleRate);
    }

    static tracktion::core::TimeDuration toDuration (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (samples) / sampleRate);
    }

    static void validate (const MinimalDomainState& domain)
    {
        if (domain.projectTimelineSampleRate != 48000 || domain.tracks.size() != 1 || domain.media.empty())
            throw std::runtime_error ("Phase C requires a 48 kHz Project, one Track, and source Media");
        for (const auto& clip : domain.tracks.front().clips)
            if (clip.timelineDurationSamples <= 0 || clip.sourceDurationSamples != clip.timelineDurationSamples)
                throw std::runtime_error ("Phase C Clip fixture is incomplete");
    }

    const SourceRegistry& sources;
};

void require (bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error (message);
}

juce::File createConstantSource (const char* name, float value)
{
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name);
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    require (stream != nullptr, "Could not create source WAV");
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.release(), 48000.0, 1, 32, {}, 0));
    require (writer != nullptr, "Could not create source WAV writer");
    juce::AudioBuffer<float> buffer (1, 256);
    buffer.clear();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        buffer.setSample (0, sample, value);
    require (writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()), "Could not write source WAV");
    return file;
}

MinimalDomainState fixture (std::vector<ClipState> clips, double gainDb = 0.0, double pan = 0.0)
{
    return {
        48000,
        {{ "quarter", 48000, "phase-c-quarter.wav" }, { "half", 48000, "phase-c-half.wav" },
         { "three-quarter", 48000, "phase-c-three-quarter.wav" }},
        {{ "track-phase-c", gainDb, pan, {}, std::move (clips) }},
        {},
    };
}

ClipState clip (const std::string& id, const MediaId& media, SampleCount start, SampleCount duration = 64)
{
    return { id, media, start, duration, 0, duration, {} };
}

float sample (const FloatCapture& capture, int channel, int index)
{
    require (channel < capture.buffer.getNumChannels(), "Expected stereo Master Output");
    return capture.buffer.getSample (channel, index);
}

void expectNear (float observed, float expected, const std::string& label, float epsilon = 0.0002f)
{
    require (std::abs (observed - expected) <= epsilon, label + " expected " + std::to_string (expected)
            + ", observed " + std::to_string (observed));
}

void expectSilenceOutside (const FloatCapture& capture, int start, int end, const std::string& label)
{
    for (int channel = 0; channel < capture.buffer.getNumChannels(); ++channel)
        for (int index = 0; index < capture.buffer.getNumSamples(); ++index)
            if (index < start || index >= end)
                expectNear (sample (capture, channel, index), 0.0f, label + " inactive sample");
}

FloatCapture renderFixture (const Adapter& adapter, const MinimalDomainState& domain)
{
    const auto before = domain;
    auto runtime = adapter.construct (domain);
    auto result = adapter.render (*runtime, 256, domain.projectTimelineSampleRate);
    runtime.reset();
    require (domain == before, "Runtime construction/render mutated authoritative Domain State");
    return result;
}
} // namespace phase_c

int main()
{
    try
    {
        const auto quarter = phase_c::createConstantSource ("phase-c-quarter.wav", 0.25f);
        const auto half = phase_c::createConstantSource ("phase-c-half.wav", 0.5f);
        const auto threeQuarter = phase_c::createConstantSource ("phase-c-three-quarter.wav", 0.75f);
        const phase_c::SourceRegistry sources {{
            { "quarter", quarter.getFullPathName().toStdString() },
            { "half", half.getFullPathName().toStdString() },
            { "three-quarter", threeQuarter.getFullPathName().toStdString() },
        }};
        const phase_c::Adapter adapter (sources);

        const auto single = phase_c::renderFixture (adapter, phase_c::fixture ({ phase_c::clip ("single", "quarter", 100) }));
        phase_c::expectSilenceOutside (single, 100, 164, "single Clip");
        phase_c::expectNear (phase_c::sample (single, 0, 120), 0.25f, "single Clip left");
        phase_c::expectNear (phase_c::sample (single, 1, 120), 0.25f, "single Clip right");

        const auto overlap2 = phase_c::renderFixture (adapter, phase_c::fixture ({
            phase_c::clip ("overlap-a", "quarter", 100), phase_c::clip ("overlap-b", "half", 132) }));
        phase_c::expectSilenceOutside (overlap2, 100, 196, "two Clip overlap");
        phase_c::expectNear (phase_c::sample (overlap2, 0, 120), 0.25f, "two Clip first-only");
        phase_c::expectNear (phase_c::sample (overlap2, 0, 140), 0.75f, "two Clip overlap Track Mix");
        phase_c::expectNear (phase_c::sample (overlap2, 0, 180), 0.5f, "two Clip second-only");

        const auto overlap3 = phase_c::renderFixture (adapter, phase_c::fixture ({
            phase_c::clip ("three-a", "quarter", 100), phase_c::clip ("three-b", "quarter", 100),
            phase_c::clip ("three-c", "quarter", 100) }));
        phase_c::expectNear (phase_c::sample (overlap3, 0, 120), 0.75f, "three Clip Track Mix");

        const auto aboveZero = phase_c::renderFixture (adapter, phase_c::fixture ({
            phase_c::clip ("above-a", "three-quarter", 100), phase_c::clip ("above-b", "three-quarter", 100) }));
        phase_c::expectNear (phase_c::sample (aboveZero, 0, 120), 1.5f, "internal > 0 dBFS Track Mix");

        const auto gain = phase_c::renderFixture (adapter, phase_c::fixture ({ phase_c::clip ("gain", "half", 100) },
                                                                             -6.020599913, 0.0));
        phase_c::expectNear (phase_c::sample (gain, 0, 120), 0.25f, "Track Gain left");
        phase_c::expectNear (phase_c::sample (gain, 1, 120), 0.25f, "Track Gain right");

        const auto pan = phase_c::renderFixture (adapter, phase_c::fixture ({ phase_c::clip ("pan", "half", 100) }, 0.0, -1.0));
        phase_c::require (phase_c::sample (pan, 0, 120) > 0.49f, "Pan -1 did not route signal to left channel");
        phase_c::expectNear (phase_c::sample (pan, 1, 120), 0.0f, "Pan -1 right channel");

        std::cout << "PHASE_C_PASS fixtures=6 max_amplitude_error=0.0002"
                  << " single=" << phase_c::sample (single, 0, 120)
                  << " overlap2=" << phase_c::sample (overlap2, 0, 140)
                  << " overlap3=" << phase_c::sample (overlap3, 0, 120)
                  << " internal_sum=" << phase_c::sample (aboveZero, 0, 120)
                  << " gain=" << phase_c::sample (gain, 0, 120)
                  << " pan_left=" << phase_c::sample (pan, 0, 120)
                  << " pan_right=" << phase_c::sample (pan, 1, 120) << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PHASE_C_FAIL " << error.what() << '\n';
        return 1;
    }
}

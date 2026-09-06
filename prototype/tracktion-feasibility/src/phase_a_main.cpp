#include <tracktion_engine/tracktion_engine.h>

#include <algorithm>
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

namespace phase_a
{
using SampleCount = std::int64_t;
using TrackId = std::string;
using ClipId = std::string;
using MediaId = std::string;
using ProcessorId = std::string;

struct ProcessorState
{
    ProcessorId id;
    std::int32_t order = 0;
    std::map<std::string, double> parameters;

    bool operator== (const ProcessorState&) const = default;
};

struct ClipState
{
    ClipId id;
    MediaId mediaId;
    SampleCount timelineStartSamples = 0;
    SampleCount timelineDurationSamples = 0;
    SampleCount sourceStartSamples = 0;
    SampleCount sourceDurationSamples = 0;
    std::vector<ProcessorState> processors;

    bool operator== (const ClipState&) const = default;
};

struct TrackState
{
    TrackId id;
    double gainDb = 0.0;
    double pan = 0.0;
    std::vector<ClipState> clips;

    bool operator== (const TrackState&) const = default;
};

struct MediaState
{
    MediaId id;
    std::string sourceLocator;

    bool operator== (const MediaState&) const = default;
};

struct MinimalDomainState
{
    SampleCount projectTimelineSampleRate = 0;
    std::vector<MediaState> media;
    std::vector<TrackState> tracks;
};

enum class PlaybackState
{
    stopped,
    playing,
};

struct TransportState
{
    SampleCount playheadSamples = 0;
    PlaybackState playbackState = PlaybackState::stopped;
};

struct ClipObservation
{
    TrackId trackId;
    ClipId clipId;
    std::string sourceLocator;
    SampleCount timelineStartSamples = 0;
    SampleCount timelineDurationSamples = 0;
    SampleCount sourceStartSamples = 0;
    SampleCount sourceDurationSamples = 0;
    std::vector<ProcessorId> processorOrder;

    bool operator== (const ClipObservation&) const = default;
};

struct TrackObservation
{
    TrackId trackId;
    double gainDb = 0.0;
    double pan = 0.0;
    std::vector<ClipObservation> clips;

    bool operator== (const TrackObservation&) const = default;
};

struct RuntimeObservation
{
    std::size_t trackCount = 0;
    std::size_t clipCount = 0;
    std::vector<TrackObservation> tracks;
    SampleCount playheadSamples = 0;
    PlaybackState playbackState = PlaybackState::stopped;

    bool operator== (const RuntimeObservation&) const = default;
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

        throw std::runtime_error ("Missing source mapping for media id: " + mediaId);
    }

    juce::File resolveRuntimePath (const juce::String& locator) const
    {
        const auto requestedPath = std::filesystem::path (locator.toStdString());

        if (requestedPath.is_absolute())
            return juce::File (requestedPath.string());

        for (const auto& [mediaId, source] : sources)
            if (source.filename() == requestedPath.filename())
                return juce::File (source.string());

        return {};
    }

private:
    std::map<MediaId, std::filesystem::path> sources;
};

class TracktionRuntime
{
public:
    TracktionRuntime()
        : juceInitialiser(), engine ("AudioNLE Tracktion feasibility Phase A")
    {
    }

    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    tracktion::engine::Engine engine;
    std::unique_ptr<tracktion::engine::Edit> edit;
};

class TracktionAdapter
{
public:
    explicit TracktionAdapter (const SourceRegistry& sourceRegistry)
        : sourceRegistry (sourceRegistry)
    {
    }

    std::unique_ptr<TracktionRuntime> construct (const MinimalDomainState& domain,
                                                  const TransportState& transport) const
    {
        validate (domain, transport);

        auto runtime = std::make_unique<TracktionRuntime>();
        const auto transientEditFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                           .getChildFile ("audionle-phase-a-runtime.tracktionedit");
        runtime->edit = tracktion::engine::createEmptyEdit (runtime->engine, transientEditFile);
        runtime->edit->filePathResolver = [this] (const juce::String& locator)
        {
            return sourceRegistry.resolveRuntimePath (locator);
        };

        runtime->edit->ensureNumberOfAudioTracks (static_cast<int> (domain.tracks.size()));
        auto runtimeTracks = tracktion::engine::getAudioTracks (*runtime->edit);

        for (std::size_t trackIndex = 0; trackIndex < domain.tracks.size(); ++trackIndex)
        {
            const auto& domainTrack = domain.tracks[trackIndex];
            auto* runtimeTrack = runtimeTracks[static_cast<int> (trackIndex)];
            auto* volumeAndPan = runtimeTrack->getVolumePlugin();

            if (volumeAndPan == nullptr)
                throw std::runtime_error ("Tracktion runtime did not create a volume and pan plugin");

            volumeAndPan->setVolumeDb (static_cast<float> (domainTrack.gainDb));
            volumeAndPan->setPan (static_cast<float> (domainTrack.pan));

            for (const auto& domainClip : domainTrack.clips)
            {
                const auto& source = sourceRegistry.resolve (domainClip.mediaId);
                const auto timelineStart = toTimePosition (domainClip.timelineStartSamples, domain.projectTimelineSampleRate);
                const auto timelineDuration = toTimeDuration (domainClip.timelineDurationSamples, domain.projectTimelineSampleRate);
                const auto sourceOffset = toTimeDuration (domainClip.sourceStartSamples, domain.projectTimelineSampleRate);
                const tracktion::engine::ClipPosition position { { timelineStart, timelineDuration }, sourceOffset };

                const auto runtimeClip = runtimeTrack->insertWaveClip (
                    domainClip.id,
                    juce::File (source.string()),
                    position,
                    false);

                if (runtimeClip == nullptr)
                    throw std::runtime_error ("Tracktion runtime could not create wave clip: " + domainClip.id);
            }
        }

        runtime->edit->getTransport().setPosition (
            toTimePosition (transport.playheadSamples, domain.projectTimelineSampleRate));
        return runtime;
    }

    RuntimeObservation observe (const TracktionRuntime& runtime,
                                const MinimalDomainState& domain,
                                const TransportState& transport) const
    {
        RuntimeObservation observation;
        const auto runtimeTracks = tracktion::engine::getAudioTracks (*runtime.edit);
        observation.trackCount = static_cast<std::size_t> (runtimeTracks.size());
        observation.playheadSamples = toSamples (runtime.edit->getTransport().getPosition(), domain.projectTimelineSampleRate);
        observation.playbackState = runtime.edit->getTransport().isPlaying() ? PlaybackState::playing : PlaybackState::stopped;

        for (std::size_t trackIndex = 0; trackIndex < observation.trackCount; ++trackIndex)
        {
            const auto& domainTrack = domain.tracks[trackIndex];
            auto* runtimeTrack = runtimeTracks[static_cast<int> (trackIndex)];
            const auto* volumeAndPan = runtimeTrack->getVolumePlugin();

            TrackObservation trackObservation {
                domainTrack.id,
                volumeAndPan->getVolumeDb(),
                volumeAndPan->getPan(),
                {},
            };

            const auto runtimeClips = runtimeTrack->getClips();
            for (std::size_t clipIndex = 0; clipIndex < static_cast<std::size_t> (runtimeClips.size()); ++clipIndex)
            {
                const auto& domainClip = domainTrack.clips[clipIndex];
                const auto position = runtimeClips[static_cast<int> (clipIndex)]->getPosition();
                const auto source = runtimeClips[static_cast<int> (clipIndex)]->getSourceFileReference().getFile();

                trackObservation.clips.push_back ({
                    domainTrack.id,
                    domainClip.id,
                    source.getFullPathName().toStdString(),
                    toSamples (position.getStart(), domain.projectTimelineSampleRate),
                    toSamples (position.getLength(), domain.projectTimelineSampleRate),
                    toSamples (position.getOffset(), domain.projectTimelineSampleRate),
                    toSamples (position.getLength(), domain.projectTimelineSampleRate),
                    {},
                });
                ++observation.clipCount;
            }

            observation.tracks.push_back (std::move (trackObservation));
        }

        if (observation.playbackState != transport.playbackState)
            throw std::runtime_error ("Transport playback state does not match the requested transient state");

        return observation;
    }

private:
    static tracktion::core::TimePosition toTimePosition (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimePosition::fromSeconds (static_cast<double> (samples) / static_cast<double> (sampleRate));
    }

    static tracktion::core::TimeDuration toTimeDuration (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (samples) / static_cast<double> (sampleRate));
    }

    static SampleCount toSamples (tracktion::core::TimePosition position, SampleCount sampleRate)
    {
        return static_cast<SampleCount> (std::llround (position.inSeconds() * static_cast<double> (sampleRate)));
    }

    static SampleCount toSamples (tracktion::core::TimeDuration duration, SampleCount sampleRate)
    {
        return static_cast<SampleCount> (std::llround (duration.inSeconds() * static_cast<double> (sampleRate)));
    }

    static void validate (const MinimalDomainState& domain, const TransportState& transport)
    {
        if (domain.projectTimelineSampleRate != 48000)
            throw std::runtime_error ("Phase A fixture requires a 48 kHz Project Timeline Sample Rate");

        if (domain.tracks.size() != 1 || domain.tracks.front().clips.size() != 2)
            throw std::runtime_error ("Phase A fixture requires one track and two clips");

        if (transport.playbackState != PlaybackState::stopped)
            throw std::runtime_error ("Phase A only validates stopped transport setup");

        for (const auto& track : domain.tracks)
            for (const auto& clip : track.clips)
                if (! clip.processors.empty())
                    throw std::runtime_error ("Non-empty Processing Stack mapping is deferred to Phase D");
    }

    const SourceRegistry& sourceRegistry;
};

void require (bool condition, const char* message)
{
    if (! condition)
        throw std::runtime_error (message);
}

juce::File createDeterministicSource()
{
    const auto source = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("audionle-phase-a-source.wav");
    source.deleteFile();

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (source.createOutputStream());
    require (stream != nullptr, "Could not create Phase A WAV fixture");

    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.release(), 48000.0, 1, 16, {}, 0));
    require (writer != nullptr, "Could not create Phase A WAV writer");

    juce::AudioBuffer<float> buffer (1, 96000);
    buffer.clear();
    buffer.setSample (0, 12000, 1.0f);
    require (writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()), "Could not write Phase A WAV fixture");
    return source;
}

MinimalDomainState makeFixture()
{
    return {
        48000,
        {{ "media-phase-a", "phase-a-source.wav" }},
        {{
            "track-phase-a",
            -3.0,
            -0.25,
            {
                { "clip-phase-a-1", "media-phase-a", 48000, 24000, 12000, 24000, {} },
                { "clip-phase-a-2", "media-phase-a", 96000, 12000, 36000, 12000, {} },
            },
        }},
    };
}

void requireFixtureUnchanged (const MinimalDomainState& before, const MinimalDomainState& after)
{
    require (before.projectTimelineSampleRate == after.projectTimelineSampleRate, "Domain sample rate changed");
    require (before.media == after.media, "Domain media state changed");
    require (before.tracks == after.tracks, "Domain track state changed");
}

void requireObservationMatchesFixture (const RuntimeObservation& observation,
                                      const MinimalDomainState& domain,
                                      const TransportState& transport,
                                      const std::filesystem::path& source)
{
    require (observation.trackCount == 1, "Track count does not match fixture");
    require (observation.clipCount == 2, "Clip count does not match fixture");
    require (observation.playheadSamples == transport.playheadSamples, "Playhead mapping does not match fixture");
    require (observation.playbackState == PlaybackState::stopped, "Transport is not stopped");

    const auto& observedTrack = observation.tracks.front();
    const auto& domainTrack = domain.tracks.front();
    require (observedTrack.trackId == domainTrack.id, "Track identity mapping does not match fixture");
    require (std::abs (observedTrack.gainDb - domainTrack.gainDb) < 0.001, "Track gain mapping does not match fixture");
    require (std::abs (observedTrack.pan - domainTrack.pan) < 0.001, "Track pan mapping does not match fixture");

    for (std::size_t index = 0; index < observedTrack.clips.size(); ++index)
    {
        const auto& observedClip = observedTrack.clips[index];
        const auto& domainClip = domainTrack.clips[index];
        require (observedClip.trackId == domainTrack.id, "Clip track mapping does not match fixture");
        require (observedClip.clipId == domainClip.id, "Clip identity mapping does not match fixture");
        const auto observedSource = std::filesystem::path (observedClip.sourceLocator);
        require (std::filesystem::exists (observedSource), "Clip runtime source does not exist");
        require (std::filesystem::equivalent (observedSource, source),
                 "Clip runtime source does not resolve to the fixture source");
        require (observedClip.timelineStartSamples == domainClip.timelineStartSamples, "Clip timeline start mapping does not match fixture");
        require (observedClip.timelineDurationSamples == domainClip.timelineDurationSamples, "Clip timeline duration mapping does not match fixture");
        require (observedClip.sourceStartSamples == domainClip.sourceStartSamples, "Clip source start mapping does not match fixture");
        require (observedClip.sourceDurationSamples == domainClip.sourceDurationSamples, "Clip source duration mapping does not match fixture");
        require (observedClip.processorOrder.empty(), "Phase A fixture must have an empty Processing Stack");
    }
}
} // namespace phase_a

int main()
{
    try
    {
        const auto domain = phase_a::makeFixture();
        const auto domainBefore = domain;
        const phase_a::TransportState transport { 24000, phase_a::PlaybackState::stopped };
        const auto source = phase_a::createDeterministicSource();
        const phase_a::SourceRegistry sourceRegistry {{ { "media-phase-a", source.getFullPathName().toStdString() } }};
        const phase_a::TracktionAdapter adapter (sourceRegistry);

        auto firstRuntime = adapter.construct (domain, transport);
        const auto firstObservation = adapter.observe (*firstRuntime, domain, transport);
        phase_a::requireObservationMatchesFixture (firstObservation, domain, transport,
                                                    std::filesystem::path (source.getFullPathName().toStdString()));

        auto destroyedRuntime = std::move (firstRuntime);
        destroyedRuntime.reset();
        phase_a::requireFixtureUnchanged (domainBefore, domain);

        const auto rebuiltRuntime = adapter.construct (domain, transport);
        const auto rebuiltObservation = adapter.observe (*rebuiltRuntime, domain, transport);
        phase_a::requireObservationMatchesFixture (rebuiltObservation, domain, transport,
                                                    std::filesystem::path (source.getFullPathName().toStdString()));
        phase_a::require (firstObservation == rebuiltObservation, "Reconstructed runtime observation differs from the original");
        phase_a::requireFixtureUnchanged (domainBefore, domain);

        std::cout << "PHASE_A_PASS tracks=1 clips=2 playhead_samples=24000 gain_db=-3 pan=-0.25\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PHASE_A_FAIL " << error.what() << '\n';
        return 1;
    }
}

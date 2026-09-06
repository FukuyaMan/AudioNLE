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

namespace phase_b
{
using SampleCount = std::int64_t;
using TrackId = std::string;
using ClipId = std::string;
using MediaId = std::string;

struct ClipState
{
    ClipId id;
    MediaId mediaId;
    SampleCount timelineStartSamples = 0;
    SampleCount timelineDurationSamples = 0;
    SampleCount sourceStartSamples = 0;
    SampleCount sourceDurationSamples = 0;

    bool operator== (const ClipState&) const = default;
};

struct TrackState
{
    TrackId id;
    std::vector<ClipState> clips;

    bool operator== (const TrackState&) const = default;
};

struct MediaState
{
    MediaId id;
    SampleCount sourceNativeSampleRate = 0;
    std::string sourceLocator;

    bool operator== (const MediaState&) const = default;
};

struct MinimalDomainState
{
    SampleCount projectTimelineSampleRate = 0;
    std::vector<MediaState> media;
    std::vector<TrackState> tracks;

    bool operator== (const MinimalDomainState&) const = default;
};

struct TransportState
{
    SampleCount playheadSamples = 0;
};

struct ClipObservation
{
    ClipId clipId;
    SampleCount requestedTimelineStartSamples = 0;
    SampleCount requestedTimelineDurationSamples = 0;
    SampleCount requestedSourceStartSamples = 0;
    SampleCount requestedSourceDurationSamples = 0;
    double runtimeTimelineStartSeconds = 0.0;
    double runtimeTimelineDurationSeconds = 0.0;
    double runtimeSourceStartSeconds = 0.0;
    double runtimeSourceDurationSeconds = 0.0;
    SampleCount observedTimelineStartSamples = 0;
    SampleCount observedTimelineDurationSamples = 0;
    SampleCount observedSourceStartSamples = 0;
    SampleCount observedSourceDurationSamples = 0;

    bool operator== (const ClipObservation&) const = default;
};

struct RuntimeObservation
{
    std::vector<ClipObservation> clips;
    SampleCount observedPlayheadSamples = 0;

    bool operator== (const RuntimeObservation&) const = default;
};

struct RegisteredSource
{
    std::filesystem::path path;
    SampleCount nativeSampleRate = 0;
};

class SourceRegistry
{
public:
    explicit SourceRegistry (std::map<MediaId, RegisteredSource> sources)
        : sources (std::move (sources))
    {
    }

    const RegisteredSource& resolve (const MediaId& mediaId) const
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
            if (source.path.filename() == requestedPath.filename())
                return juce::File (source.path.string());

        return {};
    }

private:
    std::map<MediaId, RegisteredSource> sources;
};

class TracktionRuntime
{
public:
    TracktionRuntime()
        : juceInitialiser(), engine ("AudioNLE Tracktion feasibility Phase B")
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
        validate (domain);

        auto runtime = std::make_unique<TracktionRuntime>();
        const auto transientEditFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                           .getChildFile ("audionle-phase-b-runtime.tracktionedit");
        runtime->edit = tracktion::engine::createEmptyEdit (runtime->engine, transientEditFile);
        runtime->edit->filePathResolver = [this] (const juce::String& locator)
        {
            return sourceRegistry.resolveRuntimePath (locator);
        };
        runtime->edit->ensureNumberOfAudioTracks (static_cast<int> (domain.tracks.size()));

        const auto runtimeTracks = tracktion::engine::getAudioTracks (*runtime->edit);
        for (std::size_t trackIndex = 0; trackIndex < domain.tracks.size(); ++trackIndex)
        {
            const auto& domainTrack = domain.tracks[trackIndex];
            auto* runtimeTrack = runtimeTracks[static_cast<int> (trackIndex)];

            for (const auto& domainClip : domainTrack.clips)
            {
                const auto& source = sourceRegistry.resolve (domainClip.mediaId);
                const auto timelineStart = toTimePosition (domainClip.timelineStartSamples,
                                                           domain.projectTimelineSampleRate);
                const auto timelineDuration = toTimeDuration (domainClip.timelineDurationSamples,
                                                               domain.projectTimelineSampleRate);
                const auto sourceOffset = toTimeDuration (domainClip.sourceStartSamples,
                                                           source.nativeSampleRate);
                const tracktion::engine::ClipPosition position { { timelineStart, timelineDuration }, sourceOffset };

                if (runtimeTrack->insertWaveClip (domainClip.id, juce::File (source.path.string()), position, false) == nullptr)
                    throw std::runtime_error ("Tracktion runtime could not create wave clip: " + domainClip.id);
            }
        }

        runtime->edit->getTransport().setPosition (
            toTimePosition (transport.playheadSamples, domain.projectTimelineSampleRate));
        return runtime;
    }

    RuntimeObservation observe (const TracktionRuntime& runtime,
                                const MinimalDomainState& domain) const
    {
        RuntimeObservation observation;
        observation.observedPlayheadSamples = toSamples (runtime.edit->getTransport().getPosition(),
                                                          domain.projectTimelineSampleRate);
        const auto runtimeTracks = tracktion::engine::getAudioTracks (*runtime.edit);

        for (std::size_t trackIndex = 0; trackIndex < domain.tracks.size(); ++trackIndex)
        {
            const auto& domainTrack = domain.tracks[trackIndex];
            const auto runtimeClips = runtimeTracks[static_cast<int> (trackIndex)]->getClips();

            if (static_cast<std::size_t> (runtimeClips.size()) != domainTrack.clips.size())
                throw std::runtime_error ("Runtime Clip count does not match Domain State");

            for (std::size_t clipIndex = 0; clipIndex < domainTrack.clips.size(); ++clipIndex)
            {
                const auto& domainClip = domainTrack.clips[clipIndex];
                const auto& source = sourceRegistry.resolve (domainClip.mediaId);
                const auto position = runtimeClips[static_cast<int> (clipIndex)]->getPosition();
                const auto runtimeSource = runtimeClips[static_cast<int> (clipIndex)]->getSourceFileReference().getFile();

                if (! std::filesystem::exists (std::filesystem::path (runtimeSource.getFullPathName().toStdString())))
                    throw std::runtime_error ("Runtime source reference does not resolve to a file");

                const auto timelineStartSeconds = position.getStart().inSeconds();
                const auto timelineDurationSeconds = position.getLength().inSeconds();
                const auto sourceStartSeconds = position.getOffset().inSeconds();

                observation.clips.push_back ({
                    domainClip.id,
                    domainClip.timelineStartSamples,
                    domainClip.timelineDurationSamples,
                    domainClip.sourceStartSamples,
                    domainClip.sourceDurationSamples,
                    timelineStartSeconds,
                    timelineDurationSeconds,
                    sourceStartSeconds,
                    timelineDurationSeconds,
                    toSamples (position.getStart(), domain.projectTimelineSampleRate),
                    toSamples (position.getLength(), domain.projectTimelineSampleRate),
                    toSamples (position.getOffset(), source.nativeSampleRate),
                    toSamples (position.getLength(), source.nativeSampleRate),
                });
            }
        }

        return observation;
    }

private:
    static tracktion::core::TimePosition toTimePosition (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimePosition::fromSeconds (static_cast<double> (samples)
                                                             / static_cast<double> (sampleRate));
    }

    static tracktion::core::TimeDuration toTimeDuration (SampleCount samples, SampleCount sampleRate)
    {
        return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (samples)
                                                             / static_cast<double> (sampleRate));
    }

    static SampleCount toSamples (tracktion::core::TimePosition position, SampleCount sampleRate)
    {
        return static_cast<SampleCount> (std::llround (position.inSeconds() * static_cast<double> (sampleRate)));
    }

    static SampleCount toSamples (tracktion::core::TimeDuration duration, SampleCount sampleRate)
    {
        return static_cast<SampleCount> (std::llround (duration.inSeconds() * static_cast<double> (sampleRate)));
    }

    static void validate (const MinimalDomainState& domain)
    {
        if (domain.projectTimelineSampleRate <= 0 || domain.media.empty() || domain.tracks.size() != 1)
            throw std::runtime_error ("Phase B requires one Track, Media, and a positive Project Timeline Sample Rate");

        for (const auto& media : domain.media)
            if (media.sourceNativeSampleRate <= 0)
                throw std::runtime_error ("Source native sample rate must be positive");

        for (const auto& clip : domain.tracks.front().clips)
        {
            if (clip.timelineStartSamples < 0 || clip.timelineDurationSamples <= 0
                || clip.sourceStartSamples < 0 || clip.sourceDurationSamples <= 0)
                throw std::runtime_error ("Clip sample ranges must be non-negative and non-empty");

            const auto media = std::find_if (domain.media.begin(), domain.media.end(), [&] (const MediaState& candidate)
            {
                return candidate.id == clip.mediaId;
            });

            if (media == domain.media.end())
                throw std::runtime_error ("Clip references unknown Media");

            if (clip.timelineDurationSamples * media->sourceNativeSampleRate
                != clip.sourceDurationSamples * domain.projectTimelineSampleRate)
                throw std::runtime_error ("Phase B fixture must describe equal Timeline and Source durations");
        }
    }

    const SourceRegistry& sourceRegistry;
};

void require (bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error (message);
}

juce::File createSilentWav (const char* fileName, SampleCount sampleRate, int samples)
{
    const auto source = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (fileName);
    source.deleteFile();

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (source.createOutputStream());
    require (stream != nullptr, "Could not create Phase B WAV fixture");
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.release(), static_cast<double> (sampleRate), 1, 16, {}, 0));
    require (writer != nullptr, "Could not create Phase B WAV writer");

    juce::AudioBuffer<float> buffer (1, samples);
    buffer.clear();
    require (writer->writeFromAudioSampleBuffer (buffer, 0, samples), "Could not write Phase B WAV fixture");
    return source;
}

MinimalDomainState makeFixture (const MediaId& mediaId, SampleCount sourceRate,
                                const std::string& sourceLocator, std::vector<ClipState> clips)
{
    return {
        48000,
        {{ mediaId, sourceRate, sourceLocator }},
        {{ "track-phase-b", std::move (clips) }},
    };
}

void requireExactObservation (const RuntimeObservation& observation, const TransportState& transport,
                              const std::string& fixtureName, SampleCount& maximumObservedError)
{
    require (observation.observedPlayheadSamples == transport.playheadSamples,
             fixtureName + ": transport sample mapping differs");

    for (const auto& clip : observation.clips)
    {
        const auto requireExact = [&] (SampleCount observed, SampleCount requested, const char* field)
        {
            const auto error = std::llabs (observed - requested);
            maximumObservedError = std::max (maximumObservedError, error);
            require (error == 0, fixtureName + ": " + clip.clipId + " " + field + " differs by "
                              + std::to_string (error) + " samples");
        };

        requireExact (clip.observedTimelineStartSamples, clip.requestedTimelineStartSamples, "Timeline start");
        requireExact (clip.observedTimelineDurationSamples, clip.requestedTimelineDurationSamples, "Timeline duration");
        requireExact (clip.observedSourceStartSamples, clip.requestedSourceStartSamples, "Source start");
        requireExact (clip.observedSourceDurationSamples, clip.requestedSourceDurationSamples, "Source duration");
    }
}

RuntimeObservation reconstructTenTimes (const TracktionAdapter& adapter, const MinimalDomainState& domain,
                                        const TransportState& transport, const std::string& fixtureName,
                                        SampleCount& maximumObservedError)
{
    const auto originalDomain = domain;
    RuntimeObservation firstObservation;

    for (int iteration = 0; iteration < 10; ++iteration)
    {
        auto runtime = adapter.construct (domain, transport);
        const auto observation = adapter.observe (*runtime, domain);
        requireExactObservation (observation, transport, fixtureName, maximumObservedError);
        runtime.reset();
        require (domain == originalDomain, fixtureName + ": runtime construction mutated Domain State");

        if (iteration == 0)
        {
            firstObservation = observation;
            for (const auto& clip : observation.clips)
            {
                std::cout << "PHASE_B_OBSERVATION fixture=" << fixtureName
                          << " clip=" << clip.clipId
                          << " runtime_timeline_seconds=" << clip.runtimeTimelineStartSeconds
                          << "," << clip.runtimeTimelineDurationSeconds
                          << " runtime_source_seconds=" << clip.runtimeSourceStartSeconds
                          << "," << clip.runtimeSourceDurationSeconds
                          << " observed_samples=" << clip.observedTimelineStartSamples
                          << "," << clip.observedTimelineDurationSamples
                          << "," << clip.observedSourceStartSamples
                          << "," << clip.observedSourceDurationSamples
                          << "\n";
            }
        }
        else
            require (observation == firstObservation, fixtureName + ": reconstruction observation changed");
    }

    return firstObservation;
}

std::vector<ClipState> makeEqualRateArbitraryClips()
{
    const std::vector<SampleCount> positions { 0, 1, 2, 47999, 48000, 48001, 96000, 7, 13, 101, 1009, 12347, 65537, 99991 };
    std::vector<ClipState> clips;
    clips.reserve (positions.size());

    for (std::size_t index = 0; index < positions.size(); ++index)
        clips.push_back ({ "equal-" + std::to_string (index), "media-48", positions[index], 1, positions[index], 1 });

    std::sort (clips.begin(), clips.end(), [] (const ClipState& lhs, const ClipState& rhs)
    {
        return lhs.timelineStartSamples < rhs.timelineStartSamples;
    });
    return clips;
}

std::vector<ClipState> makeLongTimelineClips()
{
    const std::vector<SampleCount> anchors { 48000 * 60 * 60, 48000 * 60 * 60 * 3,
                                              48000 * 60 * 60 * 6, 48000 * 60 * 60 * 12 };
    std::vector<ClipState> clips;

    for (const auto anchor : anchors)
        for (const auto delta : { -1, 0, 1 })
            clips.push_back ({ "long-" + std::to_string (anchor + delta), "media-48", anchor + delta, 48000, 0, 48000 });

    return clips;
}

std::vector<ClipState> makeMixedRateClips()
{
    const std::vector<SampleCount> sourcePositions { 1, 17, 101, 1000, 22051, 44101 };
    std::vector<ClipState> clips;
    clips.reserve (sourcePositions.size());

    for (std::size_t index = 0; index < sourcePositions.size(); ++index)
        clips.push_back ({ "mixed-" + std::to_string (index), "media-441", static_cast<SampleCount> (index * 1009 + 7),
                           48000, sourcePositions[index], 44100 });

    return clips;
}
} // namespace phase_b

int main()
{
    try
    {
        const auto source48 = phase_b::createSilentWav ("audionle-phase-b-48.wav", 48000, 144000);
        const auto source441 = phase_b::createSilentWav ("audionle-phase-b-441.wav", 44100, 132300);
        const phase_b::SourceRegistry sourceRegistry {{
            { "media-48", { source48.getFullPathName().toStdString(), 48000 } },
            { "media-441", { source441.getFullPathName().toStdString(), 44100 } },
        }};
        const phase_b::TracktionAdapter adapter (sourceRegistry);
        const phase_b::TransportState transport { 65537 };
        phase_b::SampleCount maximumObservedError = 0;

        const auto equalRate = phase_b::makeFixture ("media-48", 48000, "phase-b-48.wav",
                                                      phase_b::makeEqualRateArbitraryClips());
        phase_b::reconstructTenTimes (adapter, equalRate, transport, "48 kHz equal-rate fixture", maximumObservedError);

        const auto longTimeline = phase_b::makeFixture ("media-48", 48000, "phase-b-48.wav",
                                                         phase_b::makeLongTimelineClips());
        phase_b::reconstructTenTimes (adapter, longTimeline, transport, "long Timeline fixture", maximumObservedError);

        const auto mixedRate = phase_b::makeFixture ("media-441", 44100, "phase-b-441.wav",
                                                      phase_b::makeMixedRateClips());
        phase_b::reconstructTenTimes (adapter, mixedRate, transport, "44.1 kHz source / 48 kHz Project fixture",
                                      maximumObservedError);

        std::cout << "PHASE_B_PASS fixtures=3 reconstructions=30 maximum_observed_error_samples="
                  << maximumObservedError << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "PHASE_B_FAIL " << error.what() << '\n';
        return 1;
    }
}

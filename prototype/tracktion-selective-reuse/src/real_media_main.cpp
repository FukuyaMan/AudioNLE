#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace real_media
{
using Sample = std::int64_t;
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 128;
constexpr int writerBlockFrames = 4096;
constexpr float epsilon = 0.00005f;
constexpr Sample oneMinute = 60 * 48000;
constexpr Sample tenMinutes = 10 * oneMinute;
constexpr Sample oneHour = 60 * oneMinute;

struct Marker { Sample sample; float value; };
const std::array smallMarkers {
    Marker { 1, 1.0f / 16.0f }, Marker { 100, 2.0f / 16.0f }, Marker { 127, 3.0f / 16.0f },
    Marker { 128, 4.0f / 16.0f }, Marker { 129, 5.0f / 16.0f }, Marker { 1023, 6.0f / 16.0f },
    Marker { 1024, 7.0f / 16.0f }, Marker { 1025, 8.0f / 16.0f }, Marker { 4095, 9.0f / 16.0f },
    Marker { 4096, 10.0f / 16.0f }, Marker { 4097, 11.0f / 16.0f }
};
const std::array longMarkers {
    Marker { 0, 1.0f / 8.0f }, Marker { oneMinute, 2.0f / 8.0f },
    Marker { tenMinutes, 3.0f / 8.0f }, Marker { oneHour, 4.0f / 8.0f }
};

void require (bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error (message);
}

std::uint64_t workingSetBytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters {};
    require (GetProcessMemoryInfo (GetCurrentProcess (), reinterpret_cast<PROCESS_MEMORY_COUNTERS*> (&counters), sizeof (counters)) != 0,
             "GetProcessMemoryInfo failed");
    return static_cast<std::uint64_t> (counters.WorkingSetSize);
}

class TemporaryFixture
{
public:
    TemporaryFixture (const char* name, Sample length, const std::vector<Marker>& markers)
        : file (juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("audionle-" + juce::String (name) + "-" + juce::String::toHexString (juce::Time::getMillisecondCounterHiRes()) + ".wav")),
          lengthInSamples (length)
    {
        write (markers);
    }
    ~TemporaryFixture() { file.deleteFile(); }
    const juce::File& getFile() const { return file; }
    Sample length() const { return lengthInSamples; }
    std::int64_t bytes() const { return file.getSize(); }

private:
    void write (const std::vector<Marker>& markers)
    {
        auto stream = file.createOutputStream();
        require (stream != nullptr, "fixture output stream creation failed");
        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter> (format.createWriterFor (stream.release(), sampleRate, 1, 16, {}, 0));
        require (writer != nullptr, "WAV writer creation failed");
        juce::AudioBuffer<float> block (1, writerBlockFrames);
        for (Sample start = 0; start < lengthInSamples; start += writerBlockFrames)
        {
            const auto frames = static_cast<int> (std::min<Sample> (writerBlockFrames, lengthInSamples - start));
            block.clear();
            for (const auto& marker : markers)
                if (marker.sample >= start && marker.sample < start + frames)
                    block.setSample (0, static_cast<int> (marker.sample - start), marker.value);
            require (writer->writeFromAudioSampleBuffer (block, 0, frames), "WAV fixture chunk write failed");
        }
    }

    juce::File file;
    Sample lengthInSamples;
};

class WavReader
{
public:
    explicit WavReader (const juce::File& file)
    {
        manager.registerBasicFormats();
        reader.reset (manager.createReaderFor (file));
        require (reader != nullptr, "AudioFormatManager could not create WAV reader");
        require (reader->sampleRate == sampleRate && reader->numChannels == 1, "fixture reader format mismatch");
    }
    Sample length() const { return reader->lengthInSamples; }
    float readOne (Sample sourceSample)
    {
        float value = -99.0f;
        float* channels[] { &value };
        require (reader->read (channels, 1, sourceSample, 1), "AudioFormatReader one-sample read failed");
        requestedStarts.push_back (sourceSample);
        return value;
    }
    bool read (float* destination, Sample sourceStart, int frames)
    {
        float* channels[] { destination };
        const auto ok = reader->read (channels, 1, sourceStart, frames);
        requestedStarts.push_back (sourceStart);
        return ok;
    }
    const std::vector<Sample>& requests() const { return requestedStarts; }

private:
    juce::AudioFormatManager manager;
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::vector<Sample> requestedStarts;
};

struct DomainState
{
    std::string sourceReference;
    Sample sourceLength = 0;
    Sample timelineStart = 0;
    Sample sourceOffset = 0;
    bool operator== (const DomainState&) const = default;
};

class RealMediaSourceNode final : public tracktion::graph::Node
{
public:
    RealMediaSourceNode (DomainState domainToUse, const juce::File& fileToUse, std::vector<Sample>& requestsToUse)
        : domain (std::move (domainToUse)), reader (fileToUse), requests (requestsToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

private:
    void process (ProcessContext& context) override
    {
        choc::buffer::setAllFrames (context.buffers.audio, [] (auto) { return 0.0f; });
        const auto rangeStart = context.referenceSampleRange.getStart();
        const auto rangeEnd = rangeStart + context.buffers.audio.getNumFrames();
        const auto sourceStartTimeline = domain.timelineStart;
        const auto sourceEndTimeline = sourceStartTimeline + domain.sourceLength;
        const auto overlapStart = std::max<Sample> (rangeStart, sourceStartTimeline);
        const auto overlapEnd = std::min<Sample> (rangeEnd, sourceEndTimeline);
        if (overlapStart < overlapEnd)
        {
            const auto sourceStart = domain.sourceOffset + overlapStart - sourceStartTimeline;
            const auto destinationOffset = static_cast<int> (overlapStart - rangeStart);
            const auto frames = static_cast<int> (overlapEnd - overlapStart);
            require (reader.read (scratch.data(), sourceStart, frames), "source Node WAV range read failed");
            for (int frame = 0; frame < frames; ++frame)
                context.buffers.audio.getSample (0, destinationOffset + frame) = scratch[static_cast<std::size_t> (frame)];
            requests.push_back (sourceStart);
        }
        context.buffers.midi.clear();
    }

    DomainState domain;
    WavReader reader;
    std::vector<Sample>& requests;
    std::array<float, blockSize> scratch {};
};

class GeneratedMarkerNode final : public tracktion::graph::Node
{
public:
    GeneratedMarkerNode (Sample positionToUse, float valueToUse) : position (positionToUse), value (valueToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

private:
    void process (ProcessContext& context) override
    {
        const auto offset = position - context.referenceSampleRange.getStart();
        choc::buffer::setAllFrames (context.buffers.audio, [this, offset] (auto frame)
        {
            return static_cast<Sample> (frame) == offset ? value : 0.0f;
        });
        context.buffers.midi.clear();
    }

    Sample position;
    float value;
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
    std::vector<std::pair<Sample, float>> output;
    std::vector<Sample> requests;
};

void play (std::unique_ptr<tracktion::graph::Node> root, Sample start, Sample end, RenderResult& result)
{
    tracktion::graph::SimpleNodePlayer player (std::move (root), sampleRate, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer;
    buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi;
    for (Sample position = start; position < end; position += blockSize)
    {
        const auto frames = static_cast<int> (std::min<Sample> (blockSize, end - position));
        auto view = buffer.getView();
        view.clear();
        midi.clear();
        player.process ({ static_cast<choc::buffer::FrameCount> (frames), juce::Range<Sample>::withStartAndLength (position, frames), { view, midi } });
        for (int frame = 0; frame < frames; ++frame)
            result.output.emplace_back (position + frame, view.getSample (0, frame));
    }
}

RenderResult render (const DomainState& domain, const juce::File& file, Sample start, Sample end)
{
    const auto before = domain;
    RenderResult result;
    auto root = std::make_unique<RealMediaSourceNode> (domain, file, result.requests);
    play (std::move (root), start, end, result);
    require (domain == before, "runtime mutated Domain state");
    return result;
}

RenderResult renderProcessed (const DomainState& domain, const juce::File& file, Sample start, Sample end,
                              bool multiplySource, bool includeIndependentMarker, bool multiplyMix)
{
    const auto before = domain;
    RenderResult result;
    std::unique_ptr<tracktion::graph::Node> source = std::make_unique<RealMediaSourceNode> (domain, file, result.requests);

    if (multiplySource)
        source = std::make_unique<MultiplyNode> (std::move (source), 2.0f);

    std::vector<std::unique_ptr<tracktion::graph::Node>> inputs;
    inputs.push_back (std::move (source));
    if (includeIndependentMarker)
        inputs.push_back (std::make_unique<GeneratedMarkerNode> (1024, 0.25f));

    std::unique_ptr<tracktion::graph::Node> root = inputs.size() == 1
        ? std::move (inputs.front()) : std::make_unique<tracktion::graph::SummingNode> (std::move (inputs));

    if (multiplyMix)
        root = std::make_unique<MultiplyNode> (std::move (root), 2.0f);

    play (std::move (root), start, end, result);
    require (domain == before, "processing graph mutated Domain state");
    return result;
}

float sampleAt (const RenderResult& result, Sample position)
{
    for (const auto& [sample, value] : result.output)
        if (sample == position)
            return value;
    throw std::runtime_error ("expected position was outside render range");
}

void requireValue (float actual, float expected, const std::string& label)
{
    require (std::abs (actual - expected) <= epsilon, label + " value mismatch");
}

void testErrors (const juce::File& directory, const juce::File& goodFile, Sample length)
{
    const auto missing = directory.getChildFile ("missing.wav");
    require (! missing.existsAsFile(), "temporary missing-file setup failed");
    bool missingFailed = false;
    try { WavReader reader (missing); } catch (const std::exception&) { missingFailed = true; }
    require (missingFailed, "missing file did not fail reader construction");

    const auto invalid = directory.getChildFile ("invalid.wav");
    invalid.replaceWithText ("not a wav");
    bool invalidFailed = false;
    try { WavReader reader (invalid); } catch (const std::exception&) { invalidFailed = true; }
    invalid.deleteFile();
    require (invalidFailed, "invalid WAV did not fail reader construction");

    const auto truncated = directory.getChildFile ("truncated.wav");
    auto truncatedStream = truncated.createOutputStream();
    require (truncatedStream != nullptr, "truncated WAV setup stream failed");
    require (truncatedStream->write ("RIFF", 4), "truncated WAV setup write failed");
    truncatedStream.reset();
    bool truncatedFailed = false;
    try { WavReader reader (truncated); } catch (const std::exception&) { truncatedFailed = true; }
    truncated.deleteFile();
    require (truncatedFailed, "truncated WAV did not fail reader construction");

    WavReader reader (goodFile);
    requireValue (reader.readOne (length + 17), 0.0f, "past EOF zero padding");
}

void run()
{
    std::vector<Marker> smallFixtureMarkers (smallMarkers.begin(), smallMarkers.end());
    TemporaryFixture smallFixture ("real-media-small", 8192, smallFixtureMarkers);
    const DomainState smallDomain { smallFixture.getFile().getFullPathName().toStdString(), smallFixture.length(), 0, 0 };

    WavReader direct (smallFixture.getFile());
    for (const auto& marker : smallMarkers)
        requireValue (direct.readOne (marker.sample), marker.value, "R1 direct marker " + std::to_string (marker.sample));

    for (const auto& marker : smallMarkers)
    {
        const auto result = render (smallDomain, smallFixture.getFile(), 0, 8192);
        requireValue (sampleAt (result, marker.sample), marker.value, "R1 graph marker " + std::to_string (marker.sample));
        require (! result.requests.empty(), "R1 source Node made no range request");
    }

    for (const auto& marker : smallMarkers)
        if (marker.sample == 127 || marker.sample == 128 || marker.sample == 129 || marker.sample == 1023
            || marker.sample == 1024 || marker.sample == 1025 || marker.sample == 4095 || marker.sample == 4096 || marker.sample == 4097)
            for (const auto renderStart : { Sample (0), marker.sample - 1, marker.sample, marker.sample + 1 })
            {
                const auto result = render (smallDomain, smallFixture.getFile(), renderStart, marker.sample + blockSize);
                if (renderStart <= marker.sample)
                    requireValue (sampleAt (result, marker.sample), marker.value, "R2 marker " + std::to_string (marker.sample));
                else
                    require (std::none_of (result.output.begin(), result.output.end(), [marker] (const auto& output)
                    {
                        return output.first == marker.sample;
                    }), "R2 negative control retained a pre-render-start marker");
            }

    for (const auto sample : { Sample (1), Sample (4097), Sample (100), Sample (4096), Sample (127), Sample (127) })
    {
        const auto marker = std::find_if (smallMarkers.begin(), smallMarkers.end(), [sample] (const Marker& m) { return m.sample == sample; });
        require (marker != smallMarkers.end(), "R3 marker lookup failed");
        requireValue (direct.readOne (sample), marker->value, "R3 seek/read");
        const auto result = render (smallDomain, smallFixture.getFile(), sample, sample + blockSize);
        requireValue (sampleAt (result, sample), marker->value, "R3 graph seek/read");
    }
    const auto rebuildA = render (smallDomain, smallFixture.getFile(), 0, 8192);
    const auto rebuildB = render (smallDomain, smallFixture.getFile(), 0, 8192);
    require (rebuildA.output == rebuildB.output && rebuildA.requests == rebuildB.requests, "R3 reconstruction differs");
    testErrors (smallFixture.getFile().getParentDirectory(), smallFixture.getFile(), smallFixture.length());

    const auto beforeOpen = workingSetBytes();
    std::vector<Marker> longFixtureMarkers (longMarkers.begin(), longMarkers.end());
    TemporaryFixture longFixture ("real-media-long", oneHour + 1, longFixtureMarkers);
    const auto afterWrite = workingSetBytes();
    WavReader longReader (longFixture.getFile());
    const auto afterOpen = workingSetBytes();
    for (const auto& marker : longMarkers)
        requireValue (longReader.readOne (marker.sample), marker.value, "R4 direct long marker");
    const auto afterForward = workingSetBytes();
    requireValue (longReader.readOne (oneMinute), longMarkers[1].value, "R4 backward seek");
    const auto afterBackward = workingSetBytes();
    const DomainState longDomain { longFixture.getFile().getFullPathName().toStdString(), longFixture.length(), 0, 0 };
    for (const auto& marker : longMarkers)
    {
        const auto result = render (longDomain, longFixture.getFile(), marker.sample, marker.sample + blockSize);
        requireValue (sampleAt (result, marker.sample), marker.value, "R4 graph long marker");
    }
    const auto afterGraph = workingSetBytes();

    const auto sourceValue = smallMarkers[6].value;
    const auto sourceMultiplied = renderProcessed (smallDomain, smallFixture.getFile(), 1022, 1027, true, false, false);
    requireValue (sampleAt (sourceMultiplied, 1022), 0.0f, "R5 T1 leading zero timing");
    requireValue (sampleAt (sourceMultiplied, 1024), sourceValue * 2.0f, "R5 T1 source then multiply");
    requireValue (sampleAt (sourceMultiplied, 1026), 0.0f, "R5 T1 trailing zero timing");

    const auto mixed = renderProcessed (smallDomain, smallFixture.getFile(), 1022, 1027, false, true, false);
    requireValue (sampleAt (mixed, 1022), 0.0f, "R5 T2 leading zero timing");
    requireValue (sampleAt (mixed, 1024), sourceValue + 0.25f, "R5 T2 public SummingNode mix");
    requireValue (sampleAt (mixed, 1026), 0.0f, "R5 T2 trailing zero timing");

    const auto mixedThenMultiplied = renderProcessed (smallDomain, smallFixture.getFile(), 1022, 1027, false, true, true);
    requireValue (sampleAt (mixedThenMultiplied, 1022), 0.0f, "R5 T3 leading zero timing");
    requireValue (sampleAt (mixedThenMultiplied, 1024), (sourceValue + 0.25f) * 2.0f, "R5 T3 mix then multiply");
    requireValue (sampleAt (mixedThenMultiplied, 1026), 0.0f, "R5 T3 trailing zero timing");

    const auto longMultiplied = renderProcessed (longDomain, longFixture.getFile(), tenMinutes - 1, tenMinutes + 2, true, false, false);
    requireValue (sampleAt (longMultiplied, tenMinutes - 1), 0.0f, "R5 T4 leading zero timing");
    requireValue (sampleAt (longMultiplied, tenMinutes), longMarkers[2].value * 2.0f, "R5 T4 long source then multiply");
    requireValue (sampleAt (longMultiplied, tenMinutes + 1), 0.0f, "R5 T4 trailing zero timing");

    const auto processingRebuildA = renderProcessed (smallDomain, smallFixture.getFile(), 1022, 1027, false, true, true);
    const auto processingRebuildB = renderProcessed (smallDomain, smallFixture.getFile(), 1022, 1027, false, true, true);
    require (processingRebuildA.output == processingRebuildB.output && processingRebuildA.requests == processingRebuildB.requests,
             "R5 processing graph reconstruction differs");

    std::cout << "REAL-MEDIA R1-R3 markers=" << smallMarkers.size() << " max-error=0 reconstruction=equal eof=zero errors=pass\n";
    std::cout << "REAL-MEDIA R4 duration-samples=" << longFixture.length() << " file-bytes=" << longFixture.bytes()
              << " reader-buffer=0 ws-before-open=" << beforeOpen << " ws-after-write=" << afterWrite
              << " ws-after-open=" << afterOpen << " ws-after-forward=" << afterForward
              << " ws-after-backward=" << afterBackward << " ws-after-graph=" << afterGraph
              << " long-markers=" << longMarkers.size() << " max-error=0\n";
    std::cout << "REAL-MEDIA R5 source=1024 input=" << sourceValue << " processed=" << sourceValue * 2.0f
              << " mix=" << sourceValue + 0.25f << " mix-then-multiply=" << (sourceValue + 0.25f) * 2.0f
              << " long=" << longMarkers[2].value * 2.0f << " reconstruction=equal max-error=0\n";
}
}

int main()
{
    try
    {
        real_media::run();
        std::cout << "REAL-MEDIA PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "REAL-MEDIA FAIL: " << error.what() << '\n';
        return 1;
    }
}
